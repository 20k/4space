#include "ai_empire.hpp"
#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

struct orbital_system_descriptor
{
    int position = -1;

    orbital_system* os = nullptr;

    //float weighted_resource_importance = 0.f;

    ///how valuable are the resources in this system to us
    bool is_owned_by_me = false;
    bool is_owned = false;
    bool is_allied = false;
    bool contains_hostiles = false;

    float resource_rating = 0.f;
    float distance_rating = 0.f;

    int resource_ranking = 9999999;
    int distance_ranking = 9999999;

    float empire_threat_rating = 0.f;
    int empire_threat_ranking = 9999999;

    float hostiles_threat_rating = 0.f;
    int hostiles_threat_ranking = 9999999;

    float friendly_threat_rating = 0.f;
    float my_threat_rating = 0.f;
};

std::vector<orbital_system_descriptor> process_orbitals(system_manager& sm, empire* e)
{
    vec2f my_avg_pos = {0,0};

    for(orbital_system* os : e->calculated_owned_systems)
    {
        my_avg_pos += os->universe_pos / (float)e->calculated_owned_systems.size();
    }

    int fnum = 0;

    if(e->calculated_owned_systems.size() == 0)
    {
        for(orbital* o : e->owned)
        {
            if(o->type != orbital_info::FLEET)
                continue;

            fnum++;
            my_avg_pos += o->parent_system->universe_pos;
        }

        if(fnum > 0)
            my_avg_pos = my_avg_pos / (float)fnum;
    }

    const std::vector<orbital_system*>& systems = sm.systems;
    std::vector<orbital_system_descriptor> descriptor;

    int id = 0;

    for(orbital_system* os : systems)
    {
        orbital* base = os->get_base();

        if(!base->viewed_by[e])
            continue;

        orbital_system_descriptor desc;
        desc.os = os;

        if(base->parent_empire == e)
        {
            desc.is_owned_by_me = true;
        }

        if(base->parent_empire != nullptr)
        {
            desc.is_owned = true;
        }

        if(base->parent_empire && e->is_allied(base->parent_empire))
        {
            desc.is_allied = true;
        }

        desc.position = id;

        if(!desc.is_owned_by_me)
        {
            resource_manager potential_resources = os->get_potential_resources();

            float rarity = potential_resources.get_weighted_rarity();

            desc.resource_rating = rarity;
        }

        desc.distance_rating = (os->universe_pos - my_avg_pos).length();

        for(orbital* o : os->orbitals)
        {
            if(o->type != orbital_info::FLEET)
                continue;

            ship_manager* sm = (ship_manager*)o->data;

            if(e->is_hostile(o->parent_empire))
            {
                if(sm->all_derelict())
                    continue;

                float available_scanning_power = e->available_scanning_power_on(o);

                if(!sm->any_in_combat() && available_scanning_power <= ship_info::accessory_information_obfuscation_level)
                    continue;

                desc.contains_hostiles = true;

                ///WARNING NEED TO WORK IN STEALTH
                desc.hostiles_threat_rating += sm->get_tech_adjusted_military_power();
            }

            if(e->is_allied(o->parent_empire))
            {
                desc.friendly_threat_rating += sm->get_tech_adjusted_military_power();
            }

            if(o->parent_empire == e)
            {
                desc.my_threat_rating += sm->get_tech_adjusted_military_power();
            }
        }

        id++;
    }

    return descriptor;
}

void ensure_adequate_defence(ai_empire& ai, empire* e)
{
    float ship_fraction_in_defence = 0.6f;

    ///how do we pathfind ships
    ///ship command queue? :(
    ///Hooray! All neceessary work is done to implement empire behaviours! :)

    std::vector<orbital_system_descriptor> descriptors;
}



void ai_empire::tick(system_manager& sm, empire* e)
{
    ensure_adequate_defence(*this, e);
}
