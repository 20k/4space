#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

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
