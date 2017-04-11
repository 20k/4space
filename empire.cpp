#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include <set>
#include <unordered_set>
#include "ship_definitions.hpp"

empire::empire()
{
    culture_similarity = {randf_s(0.f, 1.f), randf_s(0.f, 1.f)};

    colour = randv<3, float>(0.3f, 1.f);
}

int empire::team_gid = 0;

void empire::take_ownership(orbital* o)
{
    for(auto& i : owned)
    {
        if(i == o)
            return;
    }

    o->parent_empire = this;

    owned.push_back(o);
}

void empire::take_ownership_of_all(orbital_system* o)
{
    for(orbital* i : o->orbitals)
    {
        if(i->type == orbital_info::ASTEROID && !i->is_resource_object)
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
    std::vector<float> res;
    res.resize(resource::COUNT);

    for(orbital* o : owned)
    {
        if(!o->is_resource_object)
            continue;

        resource_manager manager = o->produced_resources_ps;

        for(int i=0; i<manager.resources.size(); i++)
        {
            res[i] += manager.resources[i].amount * step_s;
        }
    }

    for(int i=0; i<res.size(); i++)
    {
        resources.resources[i].amount += res[i];
    }
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

void empire::dispense_resources(const std::map<resource::types, float>& res)
{
    for(auto& i : res)
    {
        dispense_resource(i.first, i.second);
    }
}

void empire::add_resource(resource::types type, float amount)
{
    resources.resources[(int)type].amount += amount;
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

float empire::dispense_resource(resource::types type, float requested)
{
    float available = resources.resources[(int)type].amount;

    float real = requested;

    if(available < requested)
    {
        real = available;
    }

    float& ramount = resources.resources[(int)type].amount;

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


    for(auto& i : systems)
    {
        bool own_all_planets = true;

        for(orbital* o : i->orbitals)
        {
            if(o->type == orbital_info::PLANET && o->parent_empire != this)
            {
                own_all_planets = false;
            }
        }

        if(!own_all_planets)
            continue;

        for(orbital* o : i->orbitals)
        {
            if((o->type == orbital_info::ASTEROID && o->is_resource_object) || o->type == orbital_info::MOON)
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
    if(!has_ai)
        return;

    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        sm->ai_controller.tick_fleet(sm, o, all_battles, system_manage);
    }
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

float empire::available_scanning_power_on(orbital* passed_other)
{
    if(passed_other->parent_empire == this)
        return 1.f;

    orbital_system* os = passed_other->parent_system;

    float max_scanning_power = 0.f;

    for(orbital* o : os->orbitals)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* found_sm = (ship_manager*)o->data;

        if(found_sm->parent_empire != this)
            continue;

        for(ship* s : found_sm->ships)
        {
            max_scanning_power = std::max(max_scanning_power, s->get_scanning_power_on(passed_other));
        }
    }

    return max_scanning_power;
}

float empire::available_scanning_power_on(ship* s, system_manager& system_manage)
{
    ship_manager* sm = s->owned_by;

    if(sm->parent_empire == this)
        return 1.f;

    orbital_system* os = system_manage.get_by_element(sm);

    float max_scanning_power = 0.f;

    for(orbital* o : os->orbitals)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* found_sm = (ship_manager*)o->data;

        if(found_sm->parent_empire != this)
            continue;

        for(ship* ns : found_sm->ships)
        {
            max_scanning_power = std::max(max_scanning_power, ns->get_scanning_power_on_ship(s));
        }
    }

    return max_scanning_power;
}

float empire::available_scanning_power_on(ship_manager* sm, system_manager& system_manage)
{
    float max_v = 0.f;

    for(ship* s : sm->ships)
    {
        max_v = std::max(max_v, available_scanning_power_on(s, system_manage));
    }

    return max_v;
}

void empire::become_hostile(empire* e)
{
    relations_map[e].hostile = true;
    e->relations_map[this].hostile = true;

    unally(e);
}

void empire::become_unhostile(empire* e)
{
    relations_map[e].hostile = false;
    e->relations_map[this].hostile = false;
}

void empire::ally(empire* e)
{
    relations_map[e].allied = true;
    e->relations_map[this].allied = true;

    become_unhostile(e);

    for(orbital* o : e->owned)
    {
        o->ever_viewed = true;
    }

    for(orbital* o : owned)
    {
        o->ever_viewed = true;
    }
}

void empire::unally(empire* e)
{
    relations_map[e].allied = false;
    e->relations_map[this].allied = false;
}

bool empire::is_allied(empire* e)
{
    return relations_map[e].allied;
}

bool empire::is_hostile(empire* e)
{
    return relations_map[e].hostile;
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
                    o->transfer(s->colonise_target->absolute_pos);
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

        if(sm->any_in_combat())
            continue;

        if(sm->all_derelict())
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
            }
        }
    }
}

bool empire::has_vision(orbital_system* os)
{
    empire* them = os->get_base()->parent_empire;

    if(is_allied(them))
        return true;

    if(them == this)
        return true;

    for(orbital* o : owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        if(o->parent_system == os)
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

void empire::tick_invasion_timer(float diff_s, system_manager& system_manage, fleet_manager& fleet_manage)
{
    pirate_invasion_timer_s += diff_s;

    if(pirate_invasion_timer_s < max_pirate_invasion_elapsed_time_s)
        return;

    pirate_invasion_timer_s = 0;

    std::vector<orbital_system*> effectively_owned = get_unowned_system_with_my_fleets_in();

    if(effectively_owned.size() == 0)
        return;

    std::set<orbital_system*> nearby;

    for(orbital_system* os : effectively_owned)
    {
        auto near = system_manage.get_nearest_n(os, 10);

        int cnt = 0;

        for(orbital_system* i : near)
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
            ship* test_ship = fleet1->make_new_from(team_id, make_default());
            test_ship->name = "SS " + std::to_string(i) + std::to_string(j);

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

    printf("INVADE\n");
}

empire* empire_manager::make_new()
{
    empire* e = new empire;

    e->parent = this;

    empires.push_back(e);

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

void empire_manager::tick_all(float step_s, all_battles_manager& all_battles, system_manager& system_manage, fleet_manager& fleet_manage)
{
    for(empire* emp : empires)
    {
        emp->tick(step_s);
        emp->tick_system_claim();
        emp->tick_ai(all_battles, system_manage);
    }

    for(empire* emp : pirate_empires)
    {
        emp->tick_invasion_timer(step_s, system_manage, fleet_manage);
    }
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

void claim_system(empire* e, orbital_system* os, fleet_manager& fleet_manage)
{
    ship_manager* fleet1 = fleet_manage.make_new();

    ship* test_ship = fleet1->make_new_from(e->team_id, make_default());
    test_ship->name = "SS Still Todo";

    orbital* ofleet = os->make_new(orbital_info::FLEET, 5.f);

    ofleet->orbital_angle = M_PI/13.f;
    ofleet->orbital_length = 200.f;
    ofleet->parent = os->get_base();
    ofleet->data = fleet1;

    for(orbital* o : os->orbitals)
    {
        if(o->type == orbital_info::FLEET)
            continue;

        if(o->parent_empire != nullptr)
            continue;

        e->take_ownership(o);
    }

    e->take_ownership(ofleet);
    e->take_ownership(fleet1);

}

empire* empire_manager::birth_empire(system_manager& system_manage, fleet_manager& fleet_manage, orbital_system* os, int system_size)
{
    if(os->is_owned())
        return nullptr;

    empire* e = make_new();

    e->name = "Rando";
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

        if(num >= system_size)
            break;

        claim_system(e, sys, fleet_manage);

        num++;
    }

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
    e->resources.resources[resource::OXYGEN].amount = 8000.f;

    for(int i=0; i<fleets; i++)
    {
        ship_manager* fleet1 = fleet_manage.make_new();

        for(int kk=0; kk < ships_per_fleet; kk++)
        {
            ship* test_ship = fleet1->make_new_from(e->team_id, make_default());
            test_ship->name = "SS Still Todo " + std::to_string(i) + std::to_string(kk);
        }

        orbital* ofleet = os->make_new(orbital_info::FLEET, 5.f);

        ofleet->orbital_angle = i * M_PI/13.f;
        ofleet->orbital_length = 200.f;
        ofleet->parent = os->get_base();
        ofleet->data = fleet1;

        e->take_ownership(ofleet);
        e->take_ownership(fleet1);
    }

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

    return e;
}

void empire_manager::birth_empires_random(fleet_manager& fleet_manage, system_manager& system_manage)
{
    for(int i=0; i<system_manage.systems.size(); i++)
    {
        orbital_system* os = system_manage.systems[i];

        if(os->is_owned())
            continue;

        if(randf_s(0.f, 1.f) < 0.9f)
            continue;

        birth_empire(system_manage, fleet_manage, os, randf_s(1.f, 10.f));
    }
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
}
