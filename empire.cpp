#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include <set>
#include <unordered_set>
#include "ship_definitions.hpp"
#include <imgui/imgui.h>
#include "top_bar.hpp"
#include "util.hpp"
#include "procedural_text_generator.hpp"
#include "ui_util.hpp"
#include "../4space_server/networking.hpp"

empire::empire()
{
    culture_similarity = {randf_s(0.f, 1.f), randf_s(0.f, 1.f)};

    colour = randv<3, float>(0.3f, 1.f);

    procedural_text_generator generator;

    ship_prefix = generator.generate_ship_prefix();
}

int empire::team_gid = 0;

void empire::take_ownership(orbital* o)
{
    //if(o->type == orbital_info::ASTEROID && !o->is_resource_object)
    //    return;

    if(o->type == orbital_info::ASTEROID)
        return;

    for(auto& i : owned)
    {
        if(i == o)
            return;
    }

    o->parent_empire = this;
    o->viewed_by[this] = true;

    owned.push_back(o);
}

void empire::take_ownership_of_all(orbital_system* o)
{
    for(orbital* i : o->orbitals)
    {
        //if(i->type == orbital_info::ASTEROID && !i->is_resource_object)
        //    continue;

        if(i->type == orbital_info::ASTEROID)
            continue;

        take_ownership(i);
    }
}

void empire::release_ownership(orbital* o)
{
    for(auto it = owned.begin(); it != owned.end(); it++)
    {
        if(*it == o)
        {
            o->parent_empire = nullptr;

            owned.erase(it);
            return;
        }
    }
}

void empire::take_ownership(ship_manager* o)
{
    for(auto& i : owned_fleets)
    {
        if(i == o)
            return;
    }

    o->parent_empire = this;

    for(ship* s : o->ships)
    {
        s->team = team_id;

        if(s->original_owning_race == nullptr)
            s->original_owning_race = this;
    }

    owned_fleets.push_back(o);
}

void empire::release_ownership(ship_manager* o)
{
    for(auto it = owned_fleets.begin(); it != owned_fleets.end(); it++)
    {
        if(*it == o)
        {
            o->parent_empire = nullptr;

            owned_fleets.erase(it);
            return;
        }
    }
}

bool empire::owns(orbital* o)
{
    for(auto& i : owned)
    {
        if(i == o)
            return true;
    }

    return false;
}

void empire::generate_resource_from_owned(float step_s)
{
    float res[resource::COUNT] = {0};

    static float last_step_s = 1.f;

    //resource_manager diff_res;

    for(int i=0; i<backup_income.resources.size(); i++)
    {
        float cur_res = last_income.resources[i].amount;

        if(std::isnan(cur_res))
        {
            cur_res = 0;
        }

        //diff_res.resources[i].amount = (resources.resources[i].amount - backup_income.resources[i].amount);
        partial_resources.resources[i].amount += (resources.resources[i].amount - backup_income.resources[i].amount);
    }

    resource_time_accum += step_s;

    if(resource_time_accum > 0.2f)
    {
        backup_income_list.push_back(partial_resources);
        backup_dt_s.push_back(resource_time_accum);

        partial_resources = resource_manager();

        resource_time_accum = 0.f;
    }

    //backup_income_list.push_back(diff_res);
    //backup_dt_s.push_back(step_s);

    last_step_s = step_s;

    while(backup_income_list.size() > 20)
    {
        backup_income_list.pop_front();
        backup_dt_s.pop_front();
    }

    last_income = resource_manager();

    float max_time_s = 2.f;
    float dt_accum = 0;

    for(int kk=0; kk<backup_income_list.size() && dt_accum < max_time_s; kk++)
    {
        resource_manager& res_manage = backup_income_list[(int)backup_income_list.size() - 1 - kk];

        for(int i=0; i<res_manage.resources.size(); i++)
        {
            last_income.resources[i].amount += res_manage.resources[i].amount;
        }

        dt_accum += backup_dt_s[(int)backup_dt_s.size() - 1 - kk];
    }

    if(dt_accum > FLOAT_BOUND)
    {
        for(int i=0; i<last_income.resources.size(); i++)
        {
            last_income.resources[i].amount /= dt_accum;
        }
    }

    //printf("res_manage %f\n", last_income.resources[0].amount);

    backup_income = resources;

    for(orbital* o : owned)
    {
        if(!o->is_resource_object)
            continue;

        const resource_manager& manager = o->produced_resources_ps;

        for(int i=0; i<manager.resources.size(); i++)
        {
            res[i] += manager.resources[i].amount * step_s;
        }
    }

    for(int i=0; i<resource::COUNT; i++)
    {
        resources.resources[i].amount += res[i];
    }
}

bool empire::can_fully_dispense(resource::types type, float amount)
{
    return resources.resources[type].amount >= amount;
}


bool empire::can_fully_dispense(const std::map<resource::types, float>& res)
{
    for(auto& i : res)
    {
        if(resources.resources[i.first].amount < i.second)
            return false;
    }

    return true;
}

bool empire::can_fully_dispense(const resource_manager& res)
{
    for(int i=0; i<res.resources.size(); i++)
    {
        if(resources.resources[i].amount < res.resources[i].amount)
            return false;
    }

    return true;
}

void empire::dispense_resources(const std::map<resource::types, float>& res)
{
    for(auto& i : res)
    {
        dispense_resource(i.first, i.second);
    }
}

void empire::add_resource(const resource_manager& res)
{
    for(int i=0; i<res.resources.size(); i++)
    {
        resources.resources[i].amount += res.resources[i].amount;

        //last_income.resources[i].amount += res.resources[i].amount;
    }
}

void empire::add_resource(resource::types type, float amount)
{
    resources.resources[(int)type].amount += amount;

    //last_income.resources[(int)type].amount += amount;
}

float empire::get_relations_shift_of_adding_resources(const resource_manager& res)
{
    float max_total_shift = 1.f;

    float max_individual_shift = 0.3f;

    float accum = 0.f;

    for(int i=0; i<res.resources.size(); i++)
    {
        if(res.resources[i].amount <= 0.001f)
            continue;

        ///need to use empire worth calculation here instead of raw resources
        if(resources.resources[i].amount < 100.f)
        {
            accum += max_individual_shift * res.resources[i].amount / 100.f;
        }
        else
        {
            float v = 0.25f * max_individual_shift * res.resources[i].amount / resources.resources[i].amount;

            if(v >= max_individual_shift)
                v = max_individual_shift;

            accum += v;
        }
    }

    if(accum > max_total_shift)
        accum = max_total_shift;

    return accum;
}

void empire::add_research(const research& r)
{
    for(int i=0; i<r.categories.size(); i++)
    {
        research_tech_level.categories[i].amount += r.categories[i].amount;
    }
}

void empire::culture_shift(float culture_units, empire* destination)
{
    if(destination == nullptr)
        return;

    vec2f to_destination = (destination->culture_similarity - culture_similarity).norm() * culture_units;

    culture_similarity = culture_similarity + to_destination;

    //printf("%f %f culture\n", culture_similarity.x(), culture_similarity.y());
}

void empire::positive_relations(empire* e, float amount)
{
    if(e == nullptr)
        return;

    offset_relations(e, amount);

    relations_map[e].positivity += amount;
    e->relations_map[this].positivity += amount;
}

void empire::negative_relations(empire* e, float amount)
{
    if(e == nullptr)
        return;

    offset_relations(e, -amount);

    relations_map[e].hostility += amount;
    e->relations_map[this].hostility += amount;
}

void empire::offset_relations(empire* e, float amount)
{
    if(e == nullptr)
        return;

    relations_map[e].friendliness += amount;
    e->relations_map[this].friendliness += amount;

    ///only need to do relations diffs for empires we don't own on this host
    ///if we don't do this, we'll get double relations changes
    if(!owned_by_host)
    {
        relations_diff[e].friendliness += amount;

        force_send = true;

        //std::cout << amount << std::endl;
    }

    if(!e->owned_by_host)
    {
        e->relations_diff[this].friendliness += amount;

        e->force_send = true;
    }
}

void empire::dispense_resource(const resource_manager& res)
{
    for(int i=0; i<res.resources.size(); i++)
    {
        resources.resources[i].amount -= res.resources[i].amount;
    }
}

float empire::dispense_resource(resource::types type, float requested)
{
    float available = resources.resources[(int)type].amount;

    float real = requested;

    if(available < requested)
    {
        real = available;
    }

    auto& ramount = resources.resources[(int)type].amount;

    ramount -= real;

    if(ramount < 0)
        ramount = 0;

    return real;
}

std::map<resource::types, float> empire::dispense_resources_proportionally(const std::map<resource::types, float>& type, float take_fraction, float& frac_out)
{
    float min_frac = 1.f;

    for(auto& i : type)
    {
        float available = resources.resources[(int)i.first].amount;

        float requested = i.second;

        if(available < 0.001f)
        {
            min_frac = 0;
            continue;
        }

        if(requested < 0.001f)
        {
            continue;
        }

        ///so if available > requested for all, we'll request requested
        ///of available < requested, we'll get eg 0.5 / 1.f = 0.5, and all will request 50% of their request
        min_frac = std::min(min_frac, available / requested);
    }

    min_frac = std::min(min_frac, 1.f);

    std::map<resource::types, float> ret;

    for(auto& i : type)
    {
        ret[i.first] = dispense_resource(i.first, i.second * min_frac * take_fraction);
    }

    ///so eg if this is 0.5 but we requested 2,
    //frac_out = min_frac / take_fraction;

    //frac_out = clamp(frac_out, 0.f, 1.f);

    if(take_fraction < 0.0001f)
        min_frac = 0.f;

    frac_out = min_frac;

    return ret;
}

void empire::tick_system_claim()
{
    std::unordered_set<orbital_system*> systems;

    for(orbital* o : owned)
    {
        orbital_system* os = o->parent_system;

        systems.insert(os);
    }

    ///Ok. Change this from own all to own 1 if nobody else owns any
    for(auto& i : systems)
    {
        bool own_uncontested_planet = true;
        bool own_any_planet = false;

        int num_planets = 0;

        for(orbital* o : i->orbitals)
        {
            if(o->type == orbital_info::PLANET && o->parent_empire != this && o->parent_empire != nullptr)
            {
                own_uncontested_planet = false;
            }

            if(o->type == orbital_info::PLANET)
            {
                num_planets++;
            }

            if(o->type == orbital_info::PLANET && o->parent_empire == this)
            {
                own_any_planet = true;
            }
        }

        if(!own_any_planet && i->get_base()->parent_empire == this && num_planets > 0)
        {
            i->get_base()->parent_empire = nullptr;
        }

        if(!own_uncontested_planet || !own_any_planet)
            continue;

        for(orbital* o : i->orbitals)
        {
            /*if((o->type == orbital_info::ASTEROID && o->is_resource_object) || o->type == orbital_info::MOON)
            {
                take_ownership(o);
            }*/

            if(o->type == orbital_info::MOON && o->parent != nullptr && o->parent->parent_empire == this)
            {
                take_ownership(o);
            }

            if(o->type == orbital_info::STAR)
            {
                take_ownership(o);
            }
        }
    }
}

void empire::tick(float step_s)
{
    research_tech_level.tick(step_s);

    for(auto& i : relations_map)
    {
        if(!is_allied(i.first))
            continue;

        float culture_shift_ps = 1/10000.f;

        culture_shift(culture_shift_ps * step_s, i.first);
    }
}

void empire::tick_ai(all_battles_manager& all_battles, system_manager& system_manage)
{
    //if(!has_ai)
    //    return;

    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        sm->ai_controller.tick_fleet(sm, o, all_battles, system_manage, has_ai);
    }
}

///evaluate enemy empire strengths
///launch attacks on weaker hostile empires
///expand and colonise space
///alliance of alliance system needs work.. somehow
///maybe ai should come to the aid of allies?
///maybe declaring war on someone automatically declares war on their allies?
void empire::tick_high_level_ai(float dt_s, fleet_manager& fm, system_manager& sm)
{
    if(!has_ai)
        return;

    ai_empire_controller.tick(accumulated_dt_s, fm, sm, this);
}

void empire::draw_ui()
{
    research_tech_level.draw_ui(this);
}

float empire::empire_culture_distance(empire* s)
{
    if(s == nullptr)
    {
        return 0.5f;
    }

    return (s->culture_similarity - culture_similarity).length();
}

float empire::get_research_level(research_info::types type)
{
    return research_tech_level.categories[(int)type].amount;
}

float empire::get_military_strength()
{
    float accum = 0;

    for(ship_manager* sm : owned_fleets)
    {
        accum += sm->get_tech_adjusted_military_power();
    }

    return accum;
}

float empire::available_scanning_power_on(orbital* passed_other)
{
    if(passed_other == nullptr)
        return 0.f;

    if(passed_other->parent_empire == this)
        return 1.f;

    orbital_system* os = passed_other->parent_system;

    float max_scanning_power = 0.f;

    for(orbital* o : os->orbitals)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* found_sm = (ship_manager*)o->data;

        if(found_sm->parent_empire != this && !is_allied(found_sm->parent_empire))
            continue;

        for(ship* s : found_sm->ships)
        {
            max_scanning_power = std::max(max_scanning_power, s->get_scanning_power_on(passed_other));
        }
    }

    return max_scanning_power;
}

/*float empire::available_scanning_power_on(ship* s, system_manager& system_manage)
{
    if(s == nullptr)
        return 0.f;

    ship_manager* sm = s->owned_by;

    if(sm == nullptr)
        return 1.f;

    if(sm->parent_empire == this)
        return 1.f;

    orbital_system* os = system_manage.get_by_element(sm);

    ///if os == nullptr, no orbital is associated
    ///ie bug bug bug

    float max_scanning_power = 0.f;

    for(orbital* o : os->orbitals)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* found_sm = (ship_manager*)o->data;

        if(found_sm->parent_empire != this && !is_allied(found_sm->parent_empire))
            continue;

        for(ship* ns : found_sm->ships)
        {
            max_scanning_power = std::max(max_scanning_power, ns->get_scanning_power_on_ship(s));
        }
    }

    return max_scanning_power;
}

float empire::available_scanning_power_on(ship* s, orbital* o)
{
    if(s == nullptr)
        return 0.f;

    ship_manager* sm = s->owned_by;

    if(sm == nullptr)
        return 1.f;

    if(sm->parent_empire == this)
        return 1.f;

    orbital_system* os = o->parent_system;

    ///if os == nullptr, no orbital is associated
    ///ie bug bug bug

    float max_scanning_power = 0.f;

    for(orbital* o : os->orbitals)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* found_sm = (ship_manager*)o->data;

        if(found_sm->parent_empire != this && !is_allied(found_sm->parent_empire))
            continue;

        for(ship* ns : found_sm->ships)
        {
            max_scanning_power = std::max(max_scanning_power, ns->get_scanning_power_on_ship(s));
        }
    }

    return max_scanning_power;
}*/

///available scanning power on ship does this already, we're basically duplicating stuff
/*float empire::available_scanning_power_on(ship_manager* sm, system_manager& system_manage)
{
    if(sm == nullptr)
        return 1.f;

    float max_v = 0.f;

    for(ship* s : sm->ships)
    {
        max_v = std::max(max_v, available_scanning_power_on(s, system_manage));
    }

    return max_v;
}*/

void empire::become_hostile(empire* e)
{
    if(is_hostile(e))
        return;

    relations_map[e].hostile = true;
    e->relations_map[this].hostile = true;

    if(!e->owned_by_host)
    {
        e->relations_diff[this].hostile = true;
    }

    if(!owned_by_host)
    {
        relations_diff[e].hostile = true;
    }

    negative_relations(e, 0.5f);

    trade_space_access(e, false);

    unally(e);

    offset_relations(e, get_relation_constraint_offset_upper(e, relations_info::unhostility_threshold));
    e->offset_relations(this, e->get_relation_constraint_offset_upper(this, relations_info::unhostility_threshold));
}

void empire::become_unhostile(empire* e)
{
    if(!is_hostile(e))
        return;

    relations_map[e].hostile = false;
    e->relations_map[this].hostile = false;

    if(!e->owned_by_host)
    {
        e->relations_diff[this].hostile = false;
    }

    if(!owned_by_host)
    {
        relations_diff[e].hostile = false;
    }

    positive_relations(e, 0.5f);

    offset_relations(e, get_relation_constraint_offset_lower(e, relations_info::hostility_threshold));
    e->offset_relations(this, e->get_relation_constraint_offset_lower(this, relations_info::hostility_threshold));
}

void empire::trade_space_access(empire* e, bool status)
{
    relations_map[e].have_passage_rights = status;
    e->relations_map[this].have_passage_rights = status;

    if(!e->owned_by_host)
    {
        e->relations_diff[this].have_passage_rights = status;
    }

    if(!owned_by_host)
    {
        relations_diff[e].have_passage_rights = status;
    }
}

///this doesn't work well for MP
void empire::trade_vision(empire* e)
{
    for(orbital* o : e->owned)
    {
        //o->ever_viewed = true;

        o->viewed_by[this] = true;
    }

    for(orbital* o : owned)
    {
       // o->ever_viewed = true;

       o->viewed_by[e] = true;
    }
}

void empire::ally(empire* e)
{
    if(is_allied(e))
        return;

    if(is_hostile(e))
        become_unhostile(e);

    relations_map[e].allied = true;
    e->relations_map[this].allied = true;

    if(!e->owned_by_host)
    {
        e->relations_diff[this].allied = true;
    }

    if(!owned_by_host)
    {
        relations_diff[e].allied = true;
    }

    positive_relations(e, 0.5f);

    offset_relations(e, get_relation_constraint_offset_lower(e, relations_info::unally_threshold));
    e->offset_relations(this, e->get_relation_constraint_offset_lower(this, relations_info::unally_threshold));

    for(orbital* o : e->owned)
    {
        //o->ever_viewed = true;

        o->viewed_by[this] = true;
    }

    for(orbital* o : owned)
    {
       // o->ever_viewed = true;

       o->viewed_by[e] = true;
    }

    trade_space_access(e, true);
}

void empire::unally(empire* e)
{
    if(!is_allied(e))
        return;

    relations_map[e].allied = false;
    e->relations_map[this].allied = false;

    if(!e->owned_by_host)
    {
        e->relations_diff[this].allied = false;
    }

    if(!owned_by_host)
    {
        relations_diff[e].allied = false;
    }

    negative_relations(e, 0.5f);

    trade_space_access(e, false);

    offset_relations(e, get_relation_constraint_offset_upper(e, relations_info::ally_threshold));
    e->offset_relations(this, e->get_relation_constraint_offset_upper(this, relations_info::ally_threshold));
}

bool empire::is_allied(empire* e)
{
    if(e == nullptr)
        return false;

    if(e == this)
        return false;

    return relations_map[e].allied;
}

bool empire::is_hostile(empire* e)
{
    if(e == nullptr)
        return false;

    if(e == this)
        return false;

    return relations_map[e].hostile;
}

bool empire::can_make_peace(empire* e)
{
    if(e == nullptr)
        return false;

    return relations_map[e].friendliness >= 0.5f;
}

bool empire::can_traverse_space(empire* e)
{
    return e->relations_map[this].have_passage_rights;
}

float empire::get_culture_modified_friendliness(empire* e)
{
    if(e == nullptr)
        return 0.f;

    float culture_dist = empire_culture_distance(e);

    float current_friendliness = (culture_dist - 0.5f) + relations_map[e].friendliness;

    return current_friendliness;
}

bool empire::could_invade(empire* e)
{
    return is_hostile(e) && (get_military_strength() > (e->get_military_strength() * 1.1f));
}

void empire::update_vision_of_allies(orbital* o)
{
    for(auto& relations : relations_map)
    {
        //if(is_allied(relations.first))
        if(relations.second.allied)
        {
            o->viewed_by[relations.first] = true;
            o->currently_viewed_by[relations.first] = true;
        }
    }
}

/*float empire::constrain_relations(empire* e, float val, float lower, float upper) const
{
    if(e == nullptr)
        return val;

    float friendliness = get_culture_modified_friendliness(e);

    if(val < lower)
    {
        friendliness += lower - val + relations_info::fudge;
    }

    if(val > upper)
    {
        friendliness += upper - val - relations_info::fudge;
    }

    return friendliness;
}*/

float empire::get_relation_constraint_offset_upper(empire* e, float upper)
{
    float friendliness = get_culture_modified_friendliness(e);

    if(friendliness > upper)
    {
        return upper - friendliness - relations_info::fudge;
    }

    return 0.f;
}

float empire::get_relation_constraint_offset_lower(empire* e, float lower)
{
    float friendliness = get_culture_modified_friendliness(e);

    if(friendliness < lower)
    {
        return lower - friendliness + relations_info::fudge;
    }

    return 0.f;
}

std::string empire::get_relations_string(empire* e)
{
    if(e == nullptr)
        return "Unknown";

    if(is_allied(e))
    {
        return "Allied";
    }
    else if(!is_hostile(e))
    {
        float friendliness = get_culture_modified_friendliness(e);

        if(friendliness >= 0.9f)
        {
            return "Very Friendly";
        }
        else if(friendliness >= 0.7)
        {
            return "Friendly";
        }
        else if(friendliness < 0.3f)
        {
            return "Unfriendly";
        }
        else if(friendliness < 0.1f)
        {
            return "Extreme dislike";
        }
        else
        {
           return "Neutral";
        }
    }
    else
    {
        return "Hostile";
    }
}

vec3f empire::get_relations_colour(empire* e, bool use_alt_owned_colour)
{
    if(e == nullptr)
        return relations_info::hidden_col;

    if(e == this)
    {
        if(!use_alt_owned_colour)
            return relations_info::base_col;
        else
            return relations_info::alt_owned_col;
    }

    vec3f found_col = relations_info::hostile_col;

    if(e != nullptr && !is_hostile(e))
    {
        found_col = relations_info::neutral_col;
    }

    ///if the orbital has no parent!
    if(e != nullptr && is_allied(e))
    {
        found_col = relations_info::friendly_col;
    }

    return found_col;
}

std::string empire::get_short_relations_str(empire* e)
{
    if(e == nullptr)
    {
        return "Unknown";
    }

    if(e == this)
    {
        return "Owned";
    }

    if(is_hostile(e))
    {
        return "Hostile";
    }

    if(is_allied(e))
    {
        return "Allied";
    }

    return "Neutral";
}

std::string empire::get_single_digit_relations_str(empire* e)
{
    if(e == nullptr)
    {
        return "?";
    }

    if(e == this)
    {
        return "O";
    }

    if(is_hostile(e))
    {
        return "H";
    }

    if(is_allied(e))
    {
        return "A";
    }

    return "N";
}

void empire::negative_interaction(empire* e)
{
    e->relations_map[this].hostility += 1.f;
    relations_map[e].hostility += 1.f;
}

void empire::positive_interaction(empire* e)
{
    e->relations_map[this].positivity += 1.f;
    relations_map[e].positivity += 1.f;
}

void empire::propagage_relationship_modification_from_damaging_ship(empire* damaged)
{
    float relationship_change = 0.05f;

    if(relations_map[damaged].friendliness > -1 && damaged->relations_map[this].friendliness > -1)
    {
        negative_relations(damaged, relationship_change);
    }

    float enemy_of_enemy_conversion = 0.1f;

    for(auto& rel : damaged->relations_map)
    {
        empire* e = rel.first;
        faction_relations& relations = rel.second;

        if(e->is_hostile(damaged) && e != this)
        {
            e->positive_relations(this, relationship_change * enemy_of_enemy_conversion);
        }
    }
}

bool empire::can_colonise(orbital* o)
{
    if(owns(o))
        return false;

    ///? Is parent empire an acceptable proxy for colonising
    ///but then, the whole point of colonising is ownership claiming. Can expand later?
    if(o->parent_empire != nullptr)
        return false;

    return true;
}

void empire::tick_cleanup_colonising()
{
    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        vec2f pos = o->absolute_pos;

        for(ship* s : sm->ships)
        {
            if(s->colonising)
            {
                if(s->colonise_target->parent_empire != nullptr)
                {
                    s->colonising = false;
                    s->colonise_target = nullptr;
                    continue;
                }

                vec2f target = s->colonise_target->absolute_pos;

                float len = (target - pos).length();

                float max_dist = 20.f;

                if(len < max_dist)
                {
                    s->cleanup = true;

                    take_ownership(s->colonise_target);
                }
                else
                {
                    ///we're pushing this to the front, bear in mind this may cancel later queue items down the line
                    o->command_queue.transfer(s->colonise_target->absolute_pos, o, o->parent_system, false, false, true, false);
                }
            }
        }
    }
}

void empire::tick_decolonisation()
{
    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        sm->decolonising = false;

        if(sm->any_in_combat())
            continue;

        if(sm->all_derelict())
            continue;

        ///prevent ships that have chickened out of combat
        ///from being able to decolonise planets
        if(!sm->can_engage())
            continue;

        vec2f my_pos = o->absolute_pos;

        orbital_system* parent = o->parent_system;

        ///acceptable to be n^2 now as this is only important objects
        for(orbital* other : parent->orbitals)
        {
            if(other->parent_empire == nullptr || other->parent_empire == this)
                continue;

            if(!is_hostile(other->parent_empire))
                continue;

            if(other->type != orbital_info::PLANET)
                continue;

            vec2f their_pos = other->absolute_pos;

            float dist = (their_pos - my_pos).length();

            if(dist < 40)
            {
                ///reset in o->tick, no ordering dependencies
                other->being_decolonised = true;

                sm->decolonising = true;
            }
        }
    }
}

void empire::tick_relation_ship_occupancy_loss(float step_s, system_manager& system_manage)
{
    std::map<empire*, std::map<orbital_system*, bool>> infringing_table;

    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        orbital_system* sys = o->parent_system;

        empire* system_owner = sys->get_base()->parent_empire;

        if(system_owner == nullptr)
            continue;

        if(can_traverse_space(system_owner))
            continue;

        if(infringing_table[o->parent_empire][sys])
            continue;

        infringing_table[o->parent_empire][sys] = true;

        float relation_loss_ps_per_system_infringing = 0.001f;

        float relation_loss = relation_loss_ps_per_system_infringing * step_s;

        system_owner->negative_relations(this, relation_loss);
    }
}

///will need popup alerts for this
void empire::tick_relation_alliance_changes(empire* player_empire)
{
    for(auto& i : relations_map)
    {
        empire* e = i.first;
        faction_relations& rel = i.second;

        float current_friendliness = get_culture_modified_friendliness(e);

        if(e->is_allied(this) && current_friendliness < relations_info::unally_threshold)
        {
            e->unally(this);

            current_friendliness = get_culture_modified_friendliness(e);
        }

        if(!e->is_hostile(this) && current_friendliness < relations_info::hostility_threshold)
        {
            e->become_hostile(this);

            current_friendliness = get_culture_modified_friendliness(e);
        }

        if(e == player_empire)
            continue;

        if(!is_allied(e) && current_friendliness >= relations_info::ally_threshold)
        {
            ally(e);

            current_friendliness = get_culture_modified_friendliness(e);
        }

        if(is_hostile(e) && current_friendliness > relations_info::unhostility_threshold)
        {
            become_unhostile(e);

            current_friendliness = get_culture_modified_friendliness(e);
        }
    }
}

void empire::tick_relation_border_friction(float dt_s, system_manager& system_manage)
{
    std::unordered_set<empire*> borders_touching;

    for(orbital_system* my_sys : calculated_owned_systems)
    {
        vec2f my_pos = my_sys->universe_pos * system_manage.universe_scale;

        for(orbital_system* sys : system_manage.systems)
        {
            if(sys == my_sys)
                continue;

            empire* owner = sys->get_base()->parent_empire;

            if(owner == nullptr)
                continue;

            if(owner == this)
                continue;

            vec2f their_pos = sys->universe_pos * system_manage.universe_scale;

            float dist = (their_pos - my_pos).length();

            float fudge = 1.2f;

            if(dist < system_manage.border_universe_rad * 2 * fudge)
            {
                borders_touching.insert(owner);
            }
        }
    }

    for(empire* touching_empire : borders_touching)
    {
        float relation = get_culture_modified_friendliness(touching_empire);

        ///if relations are very poor, degrade very slowly
        ///in the future, make this a constant offset rather than a degredation

        float loss_bound = 0.2f;
        float loss_speed_ps = 0.0005f;

        if(relation < loss_bound)
        {
            float relation_loss = loss_speed_ps * dt_s;

            negative_relations(touching_empire, relation_loss);
        }
    }
}

void empire::tick_calculate_owned_systems()
{
    std::unordered_set<orbital_system*> systems;

    for(orbital* o : owned)
    {
        if(o->type == orbital_info::STAR && o->parent_empire == this)
            systems.insert(o->parent_system);
    }

    calculated_owned_systems = systems;
}

bool empire::has_vision(orbital_system* os)
{
    empire* them = os->get_base()->parent_empire;

    if(is_allied(them))
        return true;

    if(them == this)
        return true;

    /*for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        if(o->parent_system == os)
            return true;
    }*/

    for(orbital* o : os->orbitals)
    {
        if(o->parent_empire == this)
            return true;

        if(is_allied(o->parent_empire))
            return true;
    }

    return false;
}

bool empire::has_direct_vision(orbital_system* os)
{
    empire* them = os->get_base()->parent_empire;

    if(them == this)
        return true;

    for(orbital* o : os->orbitals)
    {
        if(o->parent_empire == this)
            return true;
    }

    return false;
}

std::vector<orbital_system*> empire::get_unowned_system_with_my_fleets_in()
{
    std::set<orbital_system*> systems;

    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        if(sm->all_derelict())
            continue;

        if(o->parent_system->is_owned())
            continue;

        systems.insert(o->parent_system);
    }

    std::vector<orbital_system*> ret;

    for(auto& i : systems)
    {
        ret.push_back(i);
    }

    return ret;
}

void empire::tick_invasion_timer(float step_s, system_manager& system_manage, fleet_manager& fleet_manage)
{
    pirate_invasion_timer_s += step_s;

    if(pirate_invasion_timer_s < max_pirate_invasion_elapsed_time_s)
        return;

    pirate_invasion_timer_s = 0;

    std::vector<orbital_system*> effectively_owned = get_unowned_system_with_my_fleets_in();

    if(effectively_owned.size() == 0)
        return;

    std::set<orbital_system*> nearby;

    for(orbital_system* os : effectively_owned)
    {
        auto near_sys = system_manage.get_nearest_n(os, 10);

        int cnt = 0;

        for(orbital_system* i : near_sys)
        {
            if(!i->is_owned())
                continue;

            nearby.insert(i);

            cnt++;

            if(cnt > 2)
            {
                break;
            }
        }
    }

    std::vector<orbital_system*> lin_near;

    for(auto& i : nearby)
    {
        lin_near.push_back(i);
    }

    std::vector<std::pair<orbital*, ship_manager*>> fleet_nums;

    int fleets = nearby.size();

    for(int i=0; i<fleets; i++)
    {
        ship_manager* fleet1 = fleet_manage.make_new();

        for(int j = 0; j < 2; j++)
        {
            ship* test_ship = fleet1->make_new_from(this, make_default());
            //test_ship->name = "SS " + std::to_string(i) + std::to_string(j);

            std::map<ship_component_element, float> res;

            res[ship_component_element::WARP_POWER] = 100000.f;

            test_ship->distribute_resources(res);
        }

        orbital* ofleet = effectively_owned[0]->make_new(orbital_info::FLEET, 5.f);

        ofleet->orbital_angle = i*M_PI/13.f;
        ofleet->orbital_length = 200.f;
        ofleet->parent = effectively_owned[0]->get_base();
        ofleet->data = fleet1;

        take_ownership(ofleet);
        take_ownership(fleet1);

        fleet_nums.push_back({ofleet, fleet1});
    }

    for(int i=0; i<fleets; i++)
    {
        auto fl = fleet_nums[i];

        orbital* o = fl.first;
        ship_manager* sm = fl.second;

        sm->force_warp(lin_near[i], o->parent_system, o);
    }

    //printf("INVADE\n");
}

void net_faction_relations::do_serialise(serialise& s, bool ser)
{
    s.handle_serialise(friendliness, ser);
    s.handle_serialise(hostile, ser);
    s.handle_serialise(allied, ser);
    s.handle_serialise(have_passage_rights, ser);
}

void empire::do_serialise(serialise& s, bool ser)
{
    decltype(is_claimed) claimed = false;

    decltype(potential_owner) received_potential = -1;

    auto net_relations_diff = relations_diff;

    if(serialise_data_helper::send_mode == 1)
    {
        //s.handle_serialise(relations_diff, ser);
        //s.handle_serialise(resource_time_accum, ser);
        //s.handle_serialise(partial_resources, ser);
        s.handle_serialise(frame_counter, ser);
        s.handle_serialise(accumulated_dt_s, ser);
        s.handle_serialise(desired_empire_size, ser);
        s.handle_serialise(fleet_name_counter, ser);
        s.handle_serialise(toggle_systems_ui, ser);
        s.handle_serialise(is_derelict, ser);
        s.handle_serialise(toggle_ui, ser);
        s.handle_serialise(is_pirate, ser);
        s.handle_serialise(pirate_invasion_timer_s, ser);
        s.handle_serialise(parent, ser);

        ///BACKUP_DT_S
        ///BACKUP_INCOME_LIST
        ///LAST_INCOME
        ///BACKUP_INCOME

        s.handle_serialise(relations_map, ser);
        s.handle_serialise(owned_fleets, ser);
        s.handle_serialise(owned, ser);
        s.handle_serialise(ship_prefix, ser);
        s.handle_serialise(name, ser);

        s.handle_serialise(resources, ser);
        s.handle_serialise(research_tech_level, ser);

        s.handle_serialise(culture_similarity, ser);
        s.handle_serialise(given_colour, ser);
        s.handle_serialise(colour, ser);
        s.handle_serialise(has_ai, ser);

        s.handle_serialise(is_player, ser);
        s.handle_serialise(is_claimed, ser);

        s.handle_serialise(potential_owner, ser);
        s.handle_serialise(original_owner, ser);

        ///AI EMPIRE CONTROLLER
    }

    if(serialise_data_helper::send_mode == 0)
    {
        //s.handle_serialise(frame_counter, ser);
        //s.handle_serialise(accumulated_dt_s, ser);
        s.handle_serialise(desired_empire_size, ser);
        s.handle_serialise(fleet_name_counter, ser);
        //s.handle_serialise(toggle_systems_ui, ser);
        //s.handle_serialise(is_derelict, ser);
        //s.handle_serialise(toggle_ui, ser);
        //s.handle_serialise(is_pirate, ser);
        s.handle_serialise(pirate_invasion_timer_s, ser);
        s.handle_serialise(parent, ser);

        ///BACKUP_DT_S
        ///BACKUP_INCOME_LIST
        ///LAST_INCOME
        ///BACKUP_INCOME

        ///relations need to be defined in terms of relation changes
        ///ie we take a diff and network it if we need to affect our relations with another player
        ///although... if diplomacy with players is manual, this may not matter
        s.handle_serialise(relations_map, ser);
        s.handle_serialise(owned_fleets, ser);
        s.handle_serialise(owned, ser);
        //s.handle_serialise(ship_prefix, ser);
        //s.handle_serialise(name, ser);

        s.handle_serialise(resources, ser);
        s.handle_serialise(research_tech_level, ser);

        s.handle_serialise(culture_similarity, ser);
        //s.handle_serialise(given_colour, ser);
        //s.handle_serialise(colour, ser);
        s.handle_serialise(has_ai, ser);

        s.handle_serialise(is_player, ser);
        s.handle_serialise(claimed, ser);

        bool found_claim = net_claim;

        s.handle_serialise(found_claim, ser);

        if(found_claim)
        {
            if(ser == true)
            {
                s.handle_serialise(potential_owner, ser);

                net_claim = false;

                std::cout << "yay" << std::endl;
            }
            else if(ser == false)
            {
                claim_attempts++;

                std::cout << "hiya " << claim_attempts << std::endl;

                s.handle_serialise(received_potential, ser);

                if(claim_attempts > 1)
                {
                    //network_take_ownership(original_owner);
                    potential_owner = -1;
                    is_claimed = false;
                    claim_attempts = 0;
                    claim_dirty = true;
                }
                else if(claim_attempts == 1)
                {
                    //network_take_ownership(received_potential);
                    potential_owner = received_potential;
                    is_claimed = true;
                    claim_dirty = true;
                }
            }
        }
    }

    if(serialise_data_helper::send_mode == 3)
    {
        s.handle_serialise(net_relations_diff, ser);



        if(ser == true)
        {
            relations_diff.clear();

            force_send = false;

            //std::cout << "serd " << std::endl;
        }
        else
        {
            for(auto& i : net_relations_diff)
            {
                relations_map[i.first].friendliness += i.second.friendliness;

                if(i.second.hostile)
                {
                    relations_map[i.first].hostile = *i.second.hostile;
                }

                if(i.second.allied)
                {
                    relations_map[i.first].allied = *i.second.allied;
                }

                if(i.second.have_passage_rights)
                {
                    relations_map[i.first].have_passage_rights = *i.second.have_passage_rights;
                }
            }

            if(net_relations_diff.size() > 0)
            {
                //std::cout << "yaydfdfdfs\n";
            }

            //std::cout << "test" << std::endl;
        }
    }
}

///the problem is, we can't claim this for someone else as we don't know its serialisation id
void empire::network_take_ownership(network_state& net_state, serialise_host_type& host, bool modify_ai_status)
{
    if(modify_ai_status)
    {
        if(host == net_state.my_id)
        {
            has_ai = false;
            is_player = true;
        }
        else
        {
            has_ai = true;
            is_player = false;
        }
    }

    net_state.claim_for(this, host);

    for(orbital* o : owned)
    {
        net_state.claim_for(o, host);

        if(o->type == orbital_info::FLEET)
        {
            net_state.claim_for(o->data, host);

            for(ship* s : o->data->ships)
            {
                net_state.claim_for(s, host);

                ///not a pointer type
                for(component& c : s->entity_list)
                {
                    net_state.claim_for(s, host);

                    if(host == net_state.my_id)
                        c.make_dirty();
                }
                if(host == net_state.my_id)
                    s->make_dirty();
            }

        }

        if(host == net_state.my_id)
            o->make_dirty();
    }

    if(host == net_state.my_id)
        make_dirty();
}

void empire::try_network_take_ownership(network_state& net_state)
{
    if(is_claimed || potential_owner != -1 || claim_attempts > 0)
        return;

    ///HOST_ID DANGER
    ///this is fine and completely intentional, but code using host_id needs to be
    ///really aware of what it is, what it means, and how it interacts with pointers
    original_owner = host_id;

    potential_owner = net_state.my_id;

    claim_clock.restart();

    net_claim = true;

    std::cout << "net_claim" << std::endl;

    claim_attempts++;

    //network_take_ownership(net_state, net_state.my_id);

    is_claimed = true;
}

/*bool empire::tick_network_take_ownership(network_state& net_state)
{
    if(is_claimed)
        return;

    float time_s = 5;

    float elapsed_s = claim_clock.getElapsedTime().asMilliseconds() / 1000.f;

    if(elapsed_s >= time_s)
    {
        if(potential_owner != net_state.my_id)
            return;
    }
}*/

void empire::tick_network_take_ownership(network_state& net_state)
{
    if(net_claim)
        return;

    if(claim_dirty)
    {
        if(!is_claimed)
        {
            network_take_ownership(net_state, original_owner);
            std::cout << "transfer ownershipbb\n";
        }
        else
        {
            network_take_ownership(net_state, potential_owner);
            std::cout << "transfer ownershiperer\n";
        }
    }

    if(!net_state.owns(this) && potential_owner == net_state.my_id)
    {
        network_take_ownership(net_state, net_state.my_id);
        std::cout << "transfer ownershipqqq\n";
    }

    claim_dirty = false;
}

void empire_manager::tick_network_take_ownership(network_state& net_state)
{
    for(empire* e : empires)
    {
        e->tick_network_take_ownership(net_state);
    }
}

empire* empire_manager::make_new()
{
    empire* e = new empire;

    procedural_text_generator generator;

    e->parent = this;
    e->name = generator.generate_empire_name();

    empires.push_back(e);

    e->make_dirty();

    return e;
}

void empire_manager::generate_resources_from_owned(float diff_s)
{
    for(empire* e : empires)
    {
        e->generate_resource_from_owned(diff_s);
    }
}

void empire_manager::notify_removal(orbital* o)
{
    for(auto& i : empires)
    {
        i->release_ownership(o);
    }
}

void empire_manager::notify_removal(ship_manager* s)
{
    for(auto& i : empires)
    {
        i->release_ownership(s);
    }
}

void empire_manager::tick_all(float step_s, all_battles_manager& all_battles, system_manager& system_manage, fleet_manager& fleet_manage, empire* player_empire)
{
    int e_id = 0;

    for(empire* emp : empires)
    {
        emp->tick_calculate_owned_systems();
        emp->tick(step_s);
        emp->tick_system_claim();

        if(emp->owned_by_host)
            emp->tick_ai(all_battles, system_manage);

        emp->accumulated_dt_s += step_s;

        if(e_id == frame_counter && emp->owned_by_host)
        {
            emp->tick_high_level_ai(step_s, fleet_manage, system_manage);
        }

        emp->tick_relation_ship_occupancy_loss(step_s, system_manage);
        emp->tick_relation_alliance_changes(player_empire);
        emp->tick_relation_border_friction(step_s, system_manage);

        if(emp->has_ai && !emp->is_pirate && emp->owned_by_host)
        {
            e_id++;
        }
    }

    for(empire* emp : pirate_empires)
    {
        emp->tick_invasion_timer(step_s, system_manage, fleet_manage);
    }

    frame_counter++;

    if(e_id != 0)
        frame_counter %= e_id;

    tick_name_fleets();
}

void empire_manager::tick_cleanup_colonising()
{
    for(empire* e : empires)
    {
        e->tick_cleanup_colonising();
    }
}

void empire_manager::tick_decolonisation()
{
    for(empire* e : empires)
    {
        e->tick_decolonisation();
    }
}

void empire_manager::tick_name_fleets()
{
    for(empire* e : empires)
    {
        for(orbital* o : e->owned)
        {
            if(o->type != orbital_info::FLEET)
                continue;

            if(o->name != "")
                continue;

            procedural_text_generator generator;

            o->name = generator.generate_fleet_name(o);
        }
    }
}

void claim_system(empire* e, orbital_system* os, fleet_manager& fleet_manage)
{
    /*for(int i=0; i<10; i++)
    {
    ship_manager* fleet1 = fleet_manage.make_new();

    ship* test_ship = fleet1->make_new_from(e, make_colony_ship());

    //ship* test_ship = fleet1->make_new_from(e, make_default());
    //test_ship->name = "SS Still Todo";

    orbital* ofleet = os->make_new(orbital_info::FLEET, 5.f);

    ofleet->orbital_angle = M_PI/13.f;
    ofleet->orbital_length = 200.f;
    ofleet->parent = os->get_base();
    ofleet->data = fleet1;

    ship_manager* fleet2 = fleet_manage.make_new();
    ship* test_ship2 = fleet2->make_new_from(e, make_default());

    orbital* ofleet2 = os->make_new(orbital_info::FLEET, 5.f);

    ofleet2->orbital_angle = M_PI/13.f;
    ofleet2->orbital_length = 400.f;
    ofleet2->parent = os->get_base();
    ofleet2->data = fleet2;


    e->take_ownership(ofleet);
    e->take_ownership(fleet1);

    e->take_ownership(ofleet2);
    e->take_ownership(fleet2);

    }*/

    e->take_ownership_of_all(os);

    /*for(int i=0; i<5; i++)
    {
        orbital* test_o = os->make_fleet(fleet_manage, 100.f, 5.f, e);

        ship_manager* sm = (ship_manager*)test_o->data;

        ship* s = sm->make_new_from(e, make_mining_ship().duplicate());
    }*/
}

empire* empire_manager::birth_empire(system_manager& system_manage, fleet_manager& fleet_manage, orbital_system* os, int system_size)
{
    if(os->is_owned())
        return nullptr;

    empire* e = make_new();

    //e->name = "Rando";
    e->has_ai = true;

    claim_system(e, os, fleet_manage);

    std::vector<orbital_system*> systems = system_manage.systems;

    ///thanks c++. This time not even 100% sarcastically
    ///don't need to multiply by universe scale because space is relative dude
    std::sort(systems.begin(), systems.end(),

    [&](const orbital_system* a, const orbital_system* b) -> bool
    {
        return (a->universe_pos - os->universe_pos).length() < (b->universe_pos - os->universe_pos).length();
    });

    int num = 1;

    for(orbital_system* sys : systems)
    {
        if(sys == os)
            continue;

        if(sys->is_owned())
            continue;

        bool any_planets = false;

        for(orbital* o : sys->orbitals)
        {
            if(o->type == orbital_info::PLANET)
            {
                any_planets = true;
                break;
            }
        }

        if(!any_planets)
            continue;

        if(num >= system_size)
            break;

        claim_system(e, sys, fleet_manage);

        num++;
    }

    initial_spawn_reference[e] = os;

    e->desired_empire_size = system_size * 2;

    return e;
}

///give em random tech?
empire* empire_manager::birth_empire_without_system_ownership(fleet_manager& fleet_manage, orbital_system* os, int fleets, int ships_per_fleet)
{
    if(os->is_owned())
        return nullptr;

    empire* e = make_new();

    pirate_empires.push_back(e);

    e->name = "Pirates";
    e->has_ai = true;
    e->is_pirate = true;

    e->resources.resources[resource::IRON].amount = 5000.f;
    e->resources.resources[resource::COPPER].amount = 5000.f;
    e->resources.resources[resource::TITANIUM].amount = 5000.f;
    e->resources.resources[resource::URANIUM].amount = 5000.f;
    e->resources.resources[resource::RESEARCH].amount = 8000.f;
    e->resources.resources[resource::HYDROGEN].amount = 8000.f;
    //e->resources.resources[resource::OXYGEN].amount = 8000.f;

    for(int i=0; i<fleets; i++)
    {
        ship_manager* fleet1 = fleet_manage.make_new();

        for(int kk=0; kk < ships_per_fleet; kk++)
        {
            ship* test_ship = fleet1->make_new_from(e, make_default());
            //test_ship->name = "SS Still Todo " + std::to_string(i) + std::to_string(kk);
        }

        orbital* ofleet = os->make_new(orbital_info::FLEET, 5.f);

        ofleet->orbital_angle = i * M_PI/13.f;
        ofleet->orbital_length = 200.f;
        ofleet->parent = os->get_base();
        ofleet->data = fleet1;

        e->take_ownership(ofleet);
        e->take_ownership(fleet1);
    }

    return e;
}

void empire_manager::setup_relations()
{
    for(empire* e : empires)
    {
        if(!e->is_pirate)
            continue;

        for(empire* emp : empires)
        {
            if(emp == e)
                continue;

            if(emp->is_pirate)
            {
                emp->ally(e);
            }
            else
            {
                emp->become_hostile(e);
            }
        }
    }
}

void empire_manager::birth_empires_random(fleet_manager& fleet_manage, system_manager& system_manage, float sys_frac)
{
    float max_empire_size = 10.f;
    float min_empire_size = 1.f;

    float avg = (max_empire_size + min_empire_size)/2.f;

    int num = ceil(sys_frac * system_manage.systems.size() / avg);

    std::minstd_rand random;
    random.seed(rand());

    auto shuffled = system_manage.systems;

    std::shuffle(shuffled.begin(), shuffled.end(), random);

    shuffled.resize(num);

    for(int i=0; i<num; i++)
    {
        orbital_system* os = system_manage.systems[i];

        if(os->is_owned())
            continue;

        birth_empire(system_manage, fleet_manage, os, randf_s(min_empire_size, max_empire_size));
    }

    assign_colours_non_randomly();
}

void empire_manager::birth_empires_without_ownership(fleet_manager& fleet_manage, system_manager& system_manage)
{
    for(orbital_system* sys : system_manage.systems)
    {
        if(sys->is_owned())
            continue;

        if(randf_s(0.f, 1.f) < 0.8f)
            continue;

        birth_empire_without_system_ownership(fleet_manage, sys, randf_s(1.f, 4.f), randf_s(1.f, 3.f));
    }

    setup_relations();
}

void empire_manager::draw_diplomacy_ui(empire* viewer_empire, system_manager& system_manage)
{
    if(!top_bar::active[top_bar_info::DIPLOMACY])
        return;

    ImGui::BeginOverride("Diplomacy", &top_bar::active[top_bar_info::DIPLOMACY], IMGUI_WINDOW_FLAGS);

    ImGui::clamp_window_to_screen();

    ///if we've seen a system or one of their ships
    //for(auto& i : viewer_empire->relations_map)

    for(empire* e : empires)
    {
        faction_relations& relations = viewer_empire->relations_map[e];

        if(e->is_pirate)
            continue;

        if(e == viewer_empire)
            continue;

        if(e->is_derelict)
            continue;

        float current_friendliness = viewer_empire->get_culture_modified_friendliness(e);

        std::string rel_str = to_string_with_enforced_variable_dp(current_friendliness);

        std::string empire_name = e->name;

        std::string pad = "-";

        if(e->toggle_ui)
        {
            pad = "+";
        }

        ImGui::Text((pad + empire_name + pad).c_str());

        if(ImGui::IsItemClicked_Registered())
        {
            e->toggle_ui = !e->toggle_ui;
        }

        ImGui::SameLine();

        ImGui::Text(("" + rel_str).c_str());

        std::string relations_str = viewer_empire->get_relations_string(e);

        ImGui::SameLine();

        ImGui::Text(("(" + relations_str + ")").c_str());

        if(!e->toggle_ui)
            continue;

        ImGui::Indent();

        std::string rpad = "-";

        if(offer_resources_ui)
            rpad = "+";

        ImGui::Text((rpad + "Offer Resources" + rpad).c_str());

        if(ImGui::IsItemClicked_Registered())
        {
            offer_resources_ui = !offer_resources_ui;

            offering_resources = e;
        }

        ImGui::Text("Trade Starcharts");

        if(current_friendliness < 0.9f)
        {
            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Need at least 0.9 relationship to trade starcharts");
            }
        }
        else if(ImGui::IsItemClicked_Registered())
        {
            e->trade_vision(viewer_empire);
        }

        if(!viewer_empire->can_traverse_space(e))
        {
            ImGui::Text("Trade Territory Access");

            if(current_friendliness < 1.2f)
            {
                if(ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Need at least 1.2 relationship to trade territory access");
                }
            }
            else if(ImGui::IsItemClicked_Registered())
            {
                viewer_empire->trade_space_access(e, true);
            }
        }
        else
        {
            ImGui::Text("Have Territory Access");
        }

        if(!viewer_empire->is_allied(e))
        {
            ImGui::Text("(Ally)");

            if(current_friendliness < 1)
            {
                if(ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Need relation > 1");
                }
            }
            else if(ImGui::IsItemClicked_Registered())
            {
                viewer_empire->ally(e);
            }
        }
        else
        {
            ImGui::Text("(Break Alliance)");

            if(ImGui::IsItemClicked_Registered())
            {
                confirm_break_alliance = true;
            }

            if(confirm_break_alliance)
            {
                ImGui::Text("(Are you sure?)");

                if(ImGui::IsItemClicked_Registered())
                {
                    viewer_empire->unally(e);

                    confirm_break_alliance = false;
                }
            }
        }

        if(viewer_empire->is_hostile(e))
        {
            ImGui::Text("(Make Peace)");

            if(current_friendliness < 0.5f)
            {
                if(ImGui::IsItemHovered())
                    ImGui::SetTooltip("Need relation > 0.5");
            }
            else if(ImGui::IsItemClicked_Registered())
            {
                viewer_empire->become_unhostile(e);
            }
        }
        else
        {
            ImGui::Text("(Declare War)");

            if(ImGui::IsItemClicked_Registered())
            {
                confirm_declare_war = true;
            }

            if(confirm_declare_war)
            {
                ImGui::Text("(Are you sure?)");

                if(ImGui::IsItemClicked_Registered())
                {
                    if(viewer_empire->is_allied(e))
                    {
                        viewer_empire->unally(e);
                    }

                    viewer_empire->become_hostile(e);

                    confirm_declare_war = false;
                }
            }
        }


        //ImGui::Text(rel_str.c_str());

        for(int i=0; i<e->owned.size(); i++)
        {
            orbital* o = e->owned[i];

            if(o->type == orbital_info::PLANET)
            {
                ImGui::Text(("System: " + o->parent_system->get_base()->name).c_str());

                if(ImGui::IsItemClicked_Registered())
                {
                    system_manage.set_viewed_system(o->parent_system);
                }

                break;
            }
        }

        ImGui::Unindent();
    }

    ImGui::End();
}

void empire_manager::draw_resource_donation_ui(empire* viewer_empire)
{
    if(!offer_resources_ui)
        return;

    if(offering_resources == nullptr)
        return;

    ImGui::BeginOverride("Offer Resources", &offer_resources_ui, IMGUI_WINDOW_FLAGS);

    ImGui::clamp_window_to_screen();

    if(ImGui::IsWindowHovered())
    {
        ImGui::SetTooltip("Click and drag, shift for faster, alt for slower");
    }

    static float drag_res[resource::COUNT] = {0};

    for(int i=0; i<(int)resource::COUNT; i++)
    {
        ImGui::DragFloat(resource::short_names[i].c_str(), &drag_res[i], 1.f, 0.f, viewer_empire->resources.resources[i].amount);

        offering.resources[i].amount = drag_res[i];
    }

    ImGui::Text("(Offer Resource)");

    if(ImGui::IsItemClicked_Registered())
    {
        giving_are_you_sure = true;
    }

    if(giving_are_you_sure)
    {
        ImGui::Text("(Are you sure?)");

        if(ImGui::IsItemClicked_Registered())
        {
            giving_are_you_sure = false;

            if(viewer_empire->can_fully_dispense(offering))
            {
                viewer_empire->dispense_resource(offering);

                float relations_mod = offering_resources->get_relations_shift_of_adding_resources(offering);

                offering_resources->positive_relations(viewer_empire, relations_mod);

                ///get relations difference based on res
                offering_resources->add_resource(offering);
            }
        }
    }

    ImGui::End();
}

vec3f hsv_to_rgb(vec3f in)
{
    float H = in.x();
    float S = in.y();
    float V = in.z();

    S = clamp(S, 0.f, 1.f);
    V = clamp(V, 0.f, 1.f);

    if(H < 0)
        H = fabs(H);

    float C = V * S;

    float hd = H / d2r(60.f);

    float mod_H = fmod(hd, 2.f) - 1;

    float X = C * (1.f - fabs(mod_H));

    vec3f t1;

    if(hd >= 0)
        t1 = {C, X, 0};

    if(hd > 1)
        t1 = {X, C, 0};

    if(hd > 2)
        t1 = {0, C, X};

    if(hd > 3)
        t1 = {0, X, C};

    if(hd > 4)
        t1 = {X, 0, C};

    if(hd > 5)
        t1 = {C, 0, X};

    float m = V - C;

    //printf("%f %f %f\n", m, hd, H);

    return t1 + (vec3f){m,m,m};
}

void empire_manager::assign_colours_non_randomly()
{
    if(empires.size() == 0)
        return;

    float num = empires.size();

    float S = 0.99f;
    float V = 0.995f;

    float current_colour_angle = 0.01f;

    std::deque<vec3f> available_colours;

    for(int i=0; i<empires.size(); i++)
    {
        float angle = (i / (num + 1.f)) * 2 * M_PI;

        available_colours.push_back({angle + 0.01f + randf_s(-M_PI/4, M_PI/4), S, V});
    }

    for(empire* e : empires)
    {
        if(e->given_colour)
            continue;

        if(e->is_pirate)
            continue;

        float max_dist = FLT_MAX;
        empire* found_e = nullptr;

        for(empire* other : empires)
        {
            if(other->is_pirate)
                continue;

            if(other->given_colour)
                continue;

            if(other == e)
                continue;

            float dist = get_spawn_empire_distance(e, other);

            if(dist < max_dist)
            {
                max_dist = dist;
                found_e = other;
            }
        }

        if(found_e == nullptr)
            continue;

        found_e->given_colour = true;
        e->given_colour = true;

        vec3f hsv_1 = available_colours.front();

        available_colours.pop_front();

        int id = available_colours.size() / 2;

        id = clamp(id, 0, available_colours.size()-1);

        vec3f hsv_2 = available_colours[id];

        available_colours.erase(available_colours.begin() + id);

        e->colour = hsv_to_rgb(hsv_1);
        e->temporary_hsv = hsv_1;

        found_e->colour = hsv_to_rgb(hsv_2);
        found_e->temporary_hsv = hsv_2;

        //e->colour = pow(e->colour, 1/2.2);
        //found_e->colour = pow(found_e->colour, 1/2.2);
    }
}

float empire_manager::get_spawn_empire_distance(empire* e1, empire* e2)
{
    orbital_system* my_os = initial_spawn_reference[e1];

    if(my_os == nullptr)
        return 0.f;

    vec2f my_pos = my_os->universe_pos;

    orbital_system* their_os = initial_spawn_reference[e2];

    if(their_os == nullptr)
        return 0.f;

    vec2f their_pos = their_os->universe_pos;

    return (their_pos - my_pos).length();
}

void empire_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(frame_counter, ser);
        s.handle_serialise(giving_are_you_sure, ser);
        ///OFFERING
        s.handle_serialise(giving_resources_ui_clicked, ser);
        s.handle_serialise(offer_resources_ui, ser);
        s.handle_serialise(offering_resources, ser);
        s.handle_serialise(confirm_declare_war, ser);
        s.handle_serialise(confirm_break_alliance, ser);
        s.handle_serialise(unknown_empire, ser); ///???
        s.handle_serialise(initial_spawn_reference, ser);
        s.handle_serialise(pirate_empires, ser);
        s.handle_serialise(empires, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        //s.handle_serialise(frame_counter, ser);
        //s.handle_serialise(giving_are_you_sure, ser);
        ///OFFERING
        //s.handle_serialise(giving_resources_ui_clicked, ser);
        //s.handle_serialise(offer_resources_ui, ser);
        //s.handle_serialise(offering_resources, ser);
        //s.handle_serialise(confirm_declare_war, ser);
        //s.handle_serialise(confirm_break_alliance, ser);
        //s.handle_serialise(unknown_empire, ser); ///???
        //s.handle_serialise(initial_spawn_reference, ser);
        //s.handle_serialise(pirate_empires, ser);
        s.handle_serialise(empires, ser);
    }

    handled_by_client = true;
}

void empire_manager::erase_all()
{
    for(empire* e : empires)
    {
        delete e;
    }

    empires.clear();
}
