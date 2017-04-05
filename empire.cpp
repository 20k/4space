#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

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
