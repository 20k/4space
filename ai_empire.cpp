#include "ai_empire.hpp"
#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include "ship_definitions.hpp"

struct orbital_system_descriptor
{
    int position = -1;

    orbital_system* os = nullptr;

    ///how valuable are the resources in this system to us
    bool is_speculatively_owned_by_me = false;
    bool is_owned_by_me = false;
    bool is_owned = false;
    bool is_allied = false;
    ///is_hostile?
    bool contains_hostiles = false;

    bool viewed = false;

    int num_unowned_planets = 0;

    std::vector<orbital*> owned_planets;
    std::vector<orbital*> constructor_ships;

    int num_ships_raw[ship_type::COUNT] = {0};
    int num_ships_predicted[ship_type::COUNT] = {0};

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

int estimate_number_of_defence_ships_base(orbital_system_descriptor& desc)
{
    int rough_valuable_entities = desc.owned_planets.size() + desc.num_unowned_planets + desc.num_resource_asteroids;

    return std::max((int)ceil(rough_valuable_entities/2), 1);
}

std::vector<orbital_system_descriptor> process_orbitals(system_manager& sm, empire* e, ai_empire& ai_emp)
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

        //if(!base->viewed_by[e] && base->parent_empire != e)
        //    continue;

        orbital_system_descriptor desc;
        desc.os = os;

        for(orbital_system* other_os : ai_emp.speculatively_owned)
        {
            if(os == other_os)
            {
                desc.is_speculatively_owned_by_me = true;
            }
        }

        desc.viewed = base->viewed_by[e];

        if(base->parent_empire == e)
        {
            desc.is_owned_by_me = true;
            desc.is_speculatively_owned_by_me = true;
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

            desc.resource_rating = potential_resources.get_weighted_rarity();
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

            for(int i=0; i<ship_type::COUNT && o->parent_empire == e; i++)
            {
                if(sm->majority_of_type((ship_type::types)i))
                {
                    desc.num_ships_raw[i]++;
                    break;
                }
            }

            /*if(sm->majority_of_type(ship_type::MILITARY))
            {
                std::cout << "hi\n";
                std::cout << o->name << std::endl;
                std::cout << o->parent_empire->name << std::endl;
            }*/

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

            auto fin = o->command_queue.get_warp_destination();

            ship_manager* sm = (ship_manager*)o->data;

            if(fin == os)
            {
                for(int i=0; i<ship_type::COUNT; i++)
                {
                    if(sm->majority_of_type((ship_type::types)i))
                    {
                        desc.num_ships_predicted[i]++;
                        break;
                    }
                }
            }

            if(sm->ai_controller.ai_state == ai_empire_info::DEFEND && fin == os)
            {
                desc.my_threat_rating += sm->get_tech_adjusted_military_power();
            }
        }

        //if(desc.contains_hostiles)
        if(fabs(desc.hostiles_threat_rating) > 0.1f)
        {
            //printf("%f\n", desc.hostiles_threat_rating);
        }

        for(int kk = 0; kk < ship_type::COUNT; kk++)
        {
            desc.num_ships_predicted[kk] += desc.num_ships_raw[kk];
        }

        descriptor.push_back(desc);

        id++;
    }

    //printf("end\n");

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

///need to tag ships with what they do instead of this
ship* get_ship_with_need(ship_type::types type, bool warp_capable)
{
    ship* identified_ship = nullptr;

    for(ship& s : default_ships_list)
    {
        if(s.ai_fleet_type != type)
            continue;

        std::vector<ship_component_elements::types> features;

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
                break;
            }
        }

        if(all)
        {
            identified_ship = &s;
            break;
        }
    }

    return identified_ship;
}

orbital* get_constructor_for(empire* e, const std::vector<orbital_system_descriptor>& descriptors, const std::vector<ship*>& ships)
{
    std::map<resource::types, float> res_cost;

    for(ship* s : ships)
    {
        auto nres_cost = s->resources_cost();

        for(auto& i : nres_cost)
        {
            res_cost[i.first] += i.second;
        }
    }

    bool can_empire_dispense = e->can_fully_dispense(res_cost);

    for(const orbital_system_descriptor& desc : descriptors)
    {
        if(can_empire_dispense && desc.owned_planets.size() > 0)
        {
            orbital* o = desc.owned_planets.front();

            return o;
        }

        if(desc.constructor_ships.size() > 0)
        {
            for(orbital* o : desc.constructor_ships)
            {
                ship_manager* sm = (ship_manager*)o->data;

                if(sm->can_fully_dispense(res_cost))
                {
                    return o;
                }
            }
        }
    }

    return nullptr;
}

///update to allow partial building through shipyards?
bool can_afford_resource_cost(empire* e, orbital_system_descriptor& desc, const std::vector<ship*>& ships)
{
    std::map<resource::types, float> res_cost;

    for(ship* s : ships)
    {
        auto nres_cost = s->resources_cost();

        for(auto& i : nres_cost)
        {
            res_cost[i.first] += i.second;
        }
    }

    bool can_empire_dispense = e->can_fully_dispense(res_cost);

    if(can_empire_dispense)
        return true;

    return get_constructor_for(e, {desc}, ships) != nullptr;
}

orbital* try_construct_any(fleet_manager& fleet_manage, const std::vector<orbital_system_descriptor>& descriptors, ship_type::types type, empire* e, bool warp_capable)
{
    ///we should probably tag ships with their purpose to let the AI know as any decision tree here
    ///will be very incomplete (except warp/not warp)
    ship* identified_ship = get_ship_with_need(type, warp_capable);

    if(identified_ship == nullptr)
    {
        printf("rip");

        return nullptr;
    }

    auto res_cost = identified_ship->resources_cost();

    bool build = false;
    float rad = 0.f;
    float angle = 0.f;

    bool can_empire_dispense = e->can_fully_dispense(res_cost);

    for(const orbital_system_descriptor& desc : descriptors)
    {
        if(can_empire_dispense && desc.owned_planets.size() > 0)
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

            return new_o;
        }
    }

    return nullptr;
}

///integrate tech levels in here? I guess by the time that's necessary itll no longer affect cost
orbital* try_construct(fleet_manager& fleet_manage, orbital_system_descriptor& desc, ship_type::types type, empire* e, bool warp_capable)
{
    return try_construct_any(fleet_manage, {desc}, type, e, warp_capable);
}

void scout_explore(const std::vector<std::vector<orbital*>>& free_ships, std::vector<orbital_system_descriptor>& descriptors, system_manager& system_manage)
{
    std::vector<orbital_system*> to_explore;

    ///systematic exploration behaviour
    for(orbital* o : free_ships[ship_type::SCOUT])
    {
        for(auto& desc : descriptors)
        {
            if(desc.os->get_base()->viewed_by[o->parent_empire])
                continue;

            if(desc.num_ships_predicted[ship_type::SCOUT] != 0)
                continue;

            desc.num_ships_predicted[ship_type::SCOUT]++;

            auto path = system_manage.pathfind(o, desc.os);

            if(path.size() > 0)
            {
                //o->command_queue.try_warp(path, true);

                to_explore.push_back(desc.os);
                break;
            }
        }
    }

    if(to_explore.size() == 0)
        return;

    ///find nearest ship and use that to explore
    for(orbital* o : free_ships[ship_type::SCOUT])
    {
        float min_dist = FLT_MAX;
        orbital_system* found = nullptr;

        auto it = to_explore.begin();

        for(it = to_explore.begin(); it != to_explore.end(); it++)
        {
            float dist = ((*it)->universe_pos - o->parent_system->universe_pos).length();

            auto path = system_manage.pathfind(o, *it);

            if(path.size() == 0)
                continue;

            if(dist < min_dist)
            {
                min_dist = dist;
                found = *it;
            }
        }

        if(it != to_explore.end())
            to_explore.erase(it);

        if(found == nullptr)
            continue;

        auto path = system_manage.pathfind(o, found);

        if(path.size() > 0)
        {
            o->command_queue.try_warp(path, true);
        }
    }

    ///random exploration behaviour, need a mix of the two.. ie random explore if we don't need any nearby exploration
    /*for(orbital* o : free_ships[ship_type::SCOUT])
    {
        int random_start = randf_s(0.f, system_manage.systems.size());

        int max_count = system_manage.systems.size();

        for(int i=0; i<max_count; i++)
        {
            int modc = (i + random_start) % max_count;

            orbital_system* sys = system_manage.systems[modc];

            if(!sys->get_base()->viewed_by[e])
            {
                auto path = system_manage.pathfind(o, sys);

                if(path.size() == 0)
                    continue;

                o->command_queue.try_warp(path);

                break;
            }
        }
    }*/
}

void check_colonisation(std::vector<orbital_system_descriptor>& descriptors, int global_ship_deficit[], empire* e, system_manager& system_manage, fleet_manager& fleet_manage)
{
    ///move all of this into a check_colonise thing
    ///potentially arbitrarily limit colonisation speed at the moment to 1 every ~30 seconds
    ///at least until there are enough mechanics in place to make this more organic
    std::vector<orbital_system_descriptor> to_consider_colonising;

    for(int i=0; i < descriptors.size(); i++)
    {
        orbital_system_descriptor& desc = descriptors[i];

        if(desc.is_speculatively_owned_by_me)
            continue;

        if(desc.is_owned)
            continue;

        if(desc.contains_hostiles)
            continue;

        if(!desc.viewed)
            continue;

        if(desc.num_unowned_planets == 0)
            continue;

        to_consider_colonising.push_back(desc);

        if(to_consider_colonising.size() >= 20)
            break;
    }

    if(global_ship_deficit[ship_type::MINING] != 0)
    {
        to_consider_colonising.clear();
    }

    std::sort(to_consider_colonising.begin(), to_consider_colonising.end(), [](auto& s1, auto& s2){return s1.resource_rating > s2.resource_rating;});

    for(auto& desc : to_consider_colonising)
    {
        ship* colony = get_ship_with_need(ship_type::COLONY, true);
        ship* mil = get_ship_with_need(ship_type::MILITARY, true);

        int estimated_defence_ships = estimate_number_of_defence_ships_base(desc);

        std::vector<ship*> ships = {colony};

        for(int i=0; i<estimated_defence_ships; i++)
        {
            ships.push_back(mil);
        }

        orbital* constructor_orbital = get_constructor_for(e, descriptors, ships);

        if(constructor_orbital == nullptr)
            continue;

        auto test_path = system_manage.pathfind(std::min(colony->get_warp_distance(), mil->get_warp_distance()),
                                                constructor_orbital->parent_system,
                                                desc.os);

        if(test_path.size() == 0)
            continue;

        ///expected to vary per system down the road based on military costs etc
        ///this can return different for different systems currently due to ships with internal construction bays
        if(can_afford_resource_cost(e, desc, {mil, colony}))
        {
            orbital* o = try_construct_any(fleet_manage, descriptors, ship_type::MILITARY, e, true);

            if(o != nullptr)
            {
                auto path = system_manage.pathfind(o, desc.os);

                if(path.size() == 0)
                {
                    continue;
                }

                o->command_queue.try_warp(path, true);

                e->ai_empire_controller.speculatively_owned.insert(desc.os);
                break;
            }
        }

        break;
    }
}

bool has_resources_to_colonise(empire* e, orbital_system_descriptor& desc)
{
    int num_military_ships = 1;
    int num_colony_ships = 1;

    std::vector<ship*> ships;

    for(int i=0; i<num_military_ships; i++)
    {
        ships.push_back(get_ship_with_need(ship_type::MILITARY, true));
    }

    for(int i=0; i<num_colony_ships; i++)
    {
        ships.push_back(get_ship_with_need(ship_type::COLONY, true));
    }

    return can_afford_resource_cost(e, desc, ships);
}

bool wants_to_expand(empire* e, const std::vector<orbital_system_descriptor>& descriptors)
{
    int num_owned_speculatively = 0;

    for(const orbital_system_descriptor& desc : descriptors)
    {
        if(desc.is_speculatively_owned_by_me)
            num_owned_speculatively++;
    }

    if(num_owned_speculatively < e->desired_empire_size)
        return true;

    return false;
}

/**
Ok, I can do invasion in a fun way now
Firstly: Only applies to empries who don't like other empires, we're talking < 0.3
Only applies to empires of roughly the same size or smaller
Only applies if we've reached max size, or we're unable to reach max size
Then, if we find a close empire that we think we can defeat
Do invasion
*/

void ai_empire::tick(fleet_manager& fleet_manage, system_manager& system_manage, empire* e)
{
    if(e->is_pirate)
        return;

    ensure_adequate_defence(*this, e);

    std::vector<std::vector<orbital*>> free_ships;
    free_ships.resize(ship_type::COUNT);

    int num_ships[ship_type::COUNT] = {0};

    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        for(int i=0; i<ship_type::COUNT; i++)
        {
            if(sm->majority_of_type((ship_type::types)i))
            {
                num_ships[i]++;
                break;
            }
        }

        /*if(sm->majority_of_type(ship_type::MILITARY))
        {
            std::cout << o->name << std::endl;
        }*/

        if(sm->any_in_combat())
            continue;

        if(sm->any_derelict())
            continue;

        if(o->command_queue.get_warp_destinations().size() > 0)
            continue;

        ///this breaks everything
        //if(o->command_queue.command_queue.size() > 0)
        //    continue;

        if(o->command_queue.is_ever(object_command_queue_info::ANCHOR))
            continue;

        if(o->command_queue.is_ever(object_command_queue_info::COLONISE))
            continue;

        if(sm->ai_controller.ai_state != ai_empire_info::IDLE)
            continue;

        if(!sm->all_with_element(ship_component_elements::WARP_POWER))
            continue;

        for(int i=0; i<ship_type::COUNT; i++)
        {
            if(sm->majority_of_type((ship_type::types)i))
            {
                free_ships[i].push_back(o);
                break;
            }
        }
    }

    std::vector<orbital_system_descriptor> descriptors = process_orbitals(system_manage, e, *this);

    int num_unowned_planets = 0;
    int num_resource_asteroids = 0;
    int needed_military_ships = 0;

    bool do_print = false;

    for(orbital_system_descriptor& desc : descriptors)
    {
        if(!desc.is_speculatively_owned_by_me)
            continue;

        ///if a fight becomes too costly, we need to have a way to abandon a system
        if(fabs(desc.hostiles_threat_rating) >= FLOAT_BOUND)
        {
            ///we're not updating threat rating calculation which means we'll send all available ships
            if(desc.hostiles_threat_rating * 1.5f > (desc.friendly_threat_rating + desc.my_threat_rating))
            {
                for(int i=((int)free_ships[ship_type::MILITARY].size())-1; i >= 0; i--)
                {
                    orbital* o = free_ships[ship_type::MILITARY][i];
                    free_ships[ship_type::MILITARY].pop_back();

                    ship_manager* sm = (ship_manager*)o->data;

                    if(!sm->all_with_element(ship_component_elements::WARP_POWER))
                        continue;

                    auto path = system_manage.pathfind(o, desc.os);

                    if(path.size() == 0)
                        continue;

                    desc.my_threat_rating += sm->get_tech_adjusted_military_power();

                    o->command_queue.try_warp(path, true);

                    bool high_threat = desc.hostiles_threat_rating * 1.5f > (desc.friendly_threat_rating + desc.my_threat_rating);

                    if(!high_threat)
                        break;
                }
            }
        }

        int military_deficit = std::max(estimate_number_of_defence_ships_base(desc) - desc.num_ships_predicted[ship_type::MILITARY], 0);
        int mining_deficit = std::max(desc.num_resource_asteroids - desc.num_ships_predicted[ship_type::MINING], 0);
        int colony_deficit = std::max(desc.num_unowned_planets - desc.num_ships_predicted[ship_type::COLONY], 0);

        int ship_deficit[ship_type::COUNT] = {0};
        ship_deficit[ship_type::MINING] = mining_deficit;
        ship_deficit[ship_type::COLONY] = colony_deficit;

        if(mining_deficit == 0)
        {
            ship_deficit[ship_type::MILITARY] = military_deficit;
        }

        for(int i=0; i<ship_type::COUNT; i++)
        {
            for(int kk = 0; kk < ship_deficit[i]; kk++)
            {
                bool found = false;

                int fs_size = free_ships[i].size();

                for(int jj=fs_size-1; jj >= 0; jj--)
                {
                    orbital* o = free_ships[i][jj];

                    auto path = system_manage.pathfind(o, desc.os);

                    o->command_queue.try_warp(path, true);

                    if(path.size() > 0)
                    {
                        free_ships[i].erase(free_ships[i].begin() + jj);

                        found = true;
                        desc.num_ships_predicted[i]++;
                        break;
                    }
                }

                if(found)
                    continue;

                ///need some way to fairly distribute building ships so we never get stuck
                ///only build mining ships, or military ships
                orbital* new_orbital = try_construct(fleet_manage, desc, (ship_type::types)i, e, false);

                if(new_orbital != nullptr)
                {
                    desc.num_ships_raw[i]++;
                    desc.num_ships_predicted[i]++;
                }
            }
        }

        num_resource_asteroids += desc.num_resource_asteroids;
        num_unowned_planets += desc.num_unowned_planets;
        needed_military_ships += estimate_number_of_defence_ships_base(desc);
    }

    std::sort(descriptors.begin(), descriptors.end(), [](auto& s1, auto& s2){return s1.distance_rating < s2.distance_rating;});

    int military_deficit = std::max(needed_military_ships - num_ships[ship_type::MILITARY], 0);
    int mining_deficit = std::max(num_resource_asteroids - num_ships[ship_type::MINING], 0);
    int colony_deficit = std::max(num_unowned_planets - num_ships[ship_type::COLONY], 0);

    //printf("%i %i\n", needed_military_ships, num_ships[ship_type::MILITARY]);

    int max_scout_ships = 3;

    int global_ship_deficit[ship_type::COUNT] = {0};
    global_ship_deficit[ship_type::MINING] = mining_deficit;
    global_ship_deficit[ship_type::COLONY] = colony_deficit;
    global_ship_deficit[ship_type::SCOUT] = std::max(max_scout_ships - num_ships[ship_type::SCOUT], 0);
    global_ship_deficit[ship_type::MILITARY] = military_deficit;

    if(global_ship_deficit[ship_type::MINING] > 0)
    {
        global_ship_deficit[ship_type::SCOUT] = 0;
        global_ship_deficit[ship_type::MILITARY] = 0;
    }

    for(int i=0; i<ship_type::COUNT; i++)
    {
        int deficit = global_ship_deficit[i];

        if(deficit <= 0)
            continue;

        for(int kk = 0; kk < deficit; kk++)
            try_construct_any(fleet_manage, descriptors, (ship_type::types)i, e, true);
    }

    scout_explore(free_ships, descriptors, system_manage);

    if(wants_to_expand(e, descriptors))
        check_colonisation(descriptors, global_ship_deficit, e, system_manage, fleet_manage);
}
