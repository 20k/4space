#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

empire::empire()
{
    culture_similarity = {randf_s(0.f, 1.f), randf_s(0.f, 1.f)};
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
    for(auto& i : o->orbitals)
    {
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

void empire::ally(empire* e)
{
    relations_map[e].allied = true;
    e->relations_map[this].allied = true;
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

empire* empire_manager::make_new()
{
    empire* e = new empire;

    empires.push_back(e);

    return e;
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

void empire_manager::tick_all(float step_s)
{
    for(empire* emp : empires)
    {
        emp->tick(step_s);
    }
}
