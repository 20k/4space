#include "ai_empire.hpp"
#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include "ship_definitions.hpp"

namespace ship_type
{
    enum types
    {
        COLONY,
        MINING,
        MILITARY,
        COUNT,
    };
}

struct orbital_system_descriptor
{
    int position = -1;

    orbital_system* os = nullptr;

    //float weighted_resource_importance = 0.f;

    ///how valuable are the resources in this system to us
    bool is_owned_by_me = false;
    bool is_owned = false;
    bool is_allied = false;
    ///is_hostile?
    bool contains_hostiles = false;

    int num_unowned_planets = 0;

    std::vector<orbital*> owned_planets;
    std::vector<orbital*> constructor_ships;

    //int num_colony_ships = 0;

    //int num_mining_ships = 0;

    int num_ships[ship_type::COUNT] = {0};

    int num_resource_asteroids = 0;

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
            if(o->type == orbital_info::PLANET && o->parent_empire == nullptr)
            {
                desc.num_unowned_planets++;
            }

            if(o->type == orbital_info::PLANET && o->parent_empire == e && o->can_construct_ships)
            {
                desc.owned_planets.push_back(o);
            }

            if(o->type == orbital_info::FLEET && o->parent_empire == e && o->can_construct_ships)
            {
                desc.constructor_ships.push_back(o);
            }

            if(o->type == orbital_info::ASTEROID && o->is_resource_object)
            {
                desc.num_resource_asteroids++;
            }

            if(o->type != orbital_info::FLEET)
                continue;

            ship_manager* sm = (ship_manager*)o->data;

            if(sm->any_with_element(ship_component_elements::COLONISER))
            {
                desc.num_ships[ship_type::COLONY]++;
            }

            if(sm->any_with_element(ship_component_elements::ORE_HARVESTER))
            {
                desc.num_ships[ship_type::MINING]++;
            }

            if(e != o->parent_empire && e->is_hostile(o->parent_empire))
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

            if(e != o->parent_empire && e->is_allied(o->parent_empire))
            {
                desc.friendly_threat_rating += sm->get_tech_adjusted_military_power();
            }

            if(o->parent_empire == e)
            {
                desc.my_threat_rating += sm->get_tech_adjusted_military_power();
            }
        }

        for(orbital* o : e->owned)
        {
            if(o->type != orbital_info::FLEET)
                continue;

            ship_manager* sm = (ship_manager*)o->data;

            if(sm->any_with_element(ship_component_elements::COLONISER) && sm->ai_controller.on_route_to == os)
            {
                desc.num_ships[ship_type::COLONY]++;
            }

            if(sm->any_with_element(ship_component_elements::ORE_HARVESTER) && sm->ai_controller.on_route_to == os)
            {
                desc.num_ships[ship_type::MINING]++;
            }

            if(sm->ai_controller.ai_state == ai_empire_info::DEFEND && sm->ai_controller.on_route_to == os)
            {
                desc.my_threat_rating += sm->get_tech_adjusted_military_power();
            }
        }

        //if(desc.contains_hostiles)
        if(fabs(desc.hostiles_threat_rating) > 0.1f)
        {
            //printf("%f\n", desc.hostiles_threat_rating);
        }

        descriptor.push_back(desc);

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
}

struct ships_info
{
    int num_colony_ships = 0;
    int num_military_ships = 0;
    int num_mining_ships = 0;
};

///integrate tech levels in here? I guess by the time that's necessary itll no longer affect cost
bool try_construct(fleet_manager& fleet_manage, orbital_system_descriptor& desc, ship_type::types type, empire* e, bool warp_capable)
{
    //static ship to_build[] = {make_colony_ship(), make_mining_ship(), make_default()};

    ///we should probably tag ships with their purpose to let the AI know as any decision tree here
    ///will be very incomplete (except warp/not warp)
    ship* identified_ship = nullptr;

    for(ship& s : default_ships_list)
    {
        if(type == ship_type::MILITARY)
        {
            if(s.is_military())
            {
                identified_ship = &s;
                break;
            }
        }

        std::vector<ship_component_elements::types> features;

        if(type == ship_type::COLONY)
        {
            features.push_back(ship_component_elements::COLONISER);
        }

        if(type == ship_type::MINING)
        {
            features.push_back(ship_component_elements::ORE_HARVESTER);
        }

        if(warp_capable)
        {
            features.push_back(ship_component_elements::WARP_POWER);
        }

        bool all = true;

        for(auto& i : features)
        {
            if(!s.get_component_with(i))
            {
                all = false;
            }
        }

        if(all)
        {
            identified_ship = &s;
            break;
        }
    }

    if(identified_ship == nullptr)
    {
        printf("rip");

        return false;
    }

    auto res_cost = identified_ship->resources_cost();

    bool build = false;
    float rad = 0.f;
    float angle = 0.f;

    if(e->can_fully_dispense(res_cost) && desc.owned_planets.size() > 0)
    {
        orbital* o = desc.owned_planets.front();

        e->dispense_resources(res_cost);

        rad = o->orbital_length;
        angle = o->orbital_angle;

        build = true;
    }

    if(!build && desc.constructor_ships.size() > 0)
    {
        for(orbital* o : desc.constructor_ships)
        {
            ship_manager* sm = (ship_manager*)o->data;

            if(sm->can_fully_dispense(res_cost))
            {
                sm->fully_dispense(res_cost);
                build = true;

                rad = o->orbital_length;
                angle = o->orbital_angle;

                break;
            }
        }
    }

    if(build)
    {
        orbital* new_o = desc.os->make_fleet(fleet_manage, rad, angle, e);

        ship_manager* sm = (ship_manager*)new_o->data;

        ship* s = sm->make_new_from(e, identified_ship->duplicate());
    }

    return build;
}

void ai_empire::tick(fleet_manager& fleet_manage, system_manager& system_manage, empire* e)
{
    if(e->is_pirate)
        return;

    ensure_adequate_defence(*this, e);

    std::vector<orbital*> free_defence_ships;

    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        if(!sm->is_military())
            continue;

        if(sm->any_in_combat())
            continue;

        if(sm->any_derelict())
            continue;

        if(o->command_queue.get_warp_destinations().size() > 0)
            continue;

        if(sm->ai_controller.ai_state == ai_empire_info::IDLE)
        {
            free_defence_ships.push_back(o);
        }
    }

    std::vector<std::vector<orbital*>> free_ships;
    free_ships.resize(ship_type::COUNT);

    //std::vector<orbital*> free_mining_ships;
    //std::vector<orbital*> free_colony_ships;

    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        if(sm->is_military())
            continue;

        if(sm->any_in_combat())
            continue;

        if(sm->any_derelict())
            continue;

        //if(o->command_queue.get_warp_destinations().size() > 0)
        //    continue;

        if(o->command_queue.command_queue.size() != 0)
            continue;

        if(sm->any_with_element(ship_component_elements::COLONISER))
        {
            //free_colony_ships.push_back(o);
            free_ships[ship_type::COLONY].push_back(o);
        }

        if(sm->any_with_element(ship_component_elements::ORE_HARVESTER))
        {
            //free_mining_ships.push_back(o);
            free_ships[ship_type::MINING].push_back(o);
        }
    }

    std::vector<orbital_system_descriptor> descriptors = process_orbitals(system_manage, e);

    int num_resource_asteroids = 0;
    int num_ships[ship_type::COUNT] = {0};

    for(orbital_system_descriptor& desc : descriptors)
    {
        if(!desc.is_owned)
            continue;

        if(fabs(desc.hostiles_threat_rating) >= FLOAT_BOUND)
        {
            //printf("hi there\n");

            if(desc.hostiles_threat_rating * 1.5f > (desc.friendly_threat_rating + desc.my_threat_rating))
            {
                for(int i=(int)free_defence_ships.size()-1; i >= 0; i--)
                {
                    orbital* o = free_defence_ships[i];
                    free_defence_ships.pop_back();

                    ship_manager* sm = (ship_manager*)o->data;
                    desc.my_threat_rating += sm->get_tech_adjusted_military_power();

                    auto path = system_manage.pathfind(o, desc.os);

                    //printf("DEFENDING\n");

                    for(auto& sys : path)
                    {
                        o->command_queue.try_warp(sys, true);
                    }
                }
            }
        }

        int ship_deficit[ship_type::COUNT] = {0};

        int mining_deficit = std::max(desc.num_resource_asteroids - desc.num_ships[ship_type::MINING], 0);
        int colony_deficit = std::max(desc.num_unowned_planets - desc.num_ships[ship_type::COLONY], 0);

        ship_deficit[ship_type::MINING] = mining_deficit;
        ship_deficit[ship_type::COLONY] = colony_deficit;

        for(int i=0; i<ship_type::COUNT; i++)
        {
            for(int kk = 0; kk < ship_deficit[i]; kk++)
            {
                if(free_ships[i].size() > 0)
                {
                    orbital* o = free_ships[i].back();
                    free_ships[i].pop_back();

                    auto path = system_manage.pathfind(o, desc.os);

                    for(auto& sys : path)
                    {
                        o->command_queue.try_warp(sys, true);
                    }

                    continue;
                }

                bool success = try_construct(fleet_manage, desc, (ship_type::types)i, e, false);

                if(success)
                {
                    desc.num_ships[i]++;
                }
            }
        }

        num_resource_asteroids += desc.num_resource_asteroids;

        for(int i=0; i<ship_type::COUNT; i++)
        {
            num_ships[i] += desc.num_ships[i];
        }
    }

    int mining_ship_deficit = num_resource_asteroids - num_ships[ship_type::MINING];
    mining_ship_deficit = std::max(mining_ship_deficit, 0);
}
