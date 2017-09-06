#include "ai_fleet.hpp"
#include "system_manager.hpp"
#include "empire.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include <assert.h>
#include "profile.hpp"
#include <unordered_set>

uint32_t ai_fleet::gid;

std::pair<orbital*, ship_manager*> get_nearest(const std::vector<std::pair<orbital*, ship_manager*>>& targets, orbital* me)
{
    float min_dist = 999999.f;
    std::pair<orbital*, ship_manager*> ret = {nullptr, nullptr};

    for(auto& i : targets)
    {
        orbital* o = i.first;

        vec2f my_pos = me->absolute_pos;
        vec2f their_pos = o->absolute_pos;

        float dist = (their_pos - my_pos).length();

        if(dist < min_dist)
        {
            ret = i;
            min_dist = dist;
        }
    }

    return ret;
}

///change command queue to accept is_currently(item_type);
///and get_target(item_type); or something, so we can at least try and generalise this a bit
///this codebase suffers way too much from a lack of genericness partly driven by the slightly not quite
///enough kinds of things that can be done, so its very easy to just have can_x and can_y, not can(x)
void try_colonise(ship_manager* sm, orbital* my_o)
{
    if(!sm->any_with_element(ship_component_elements::COLONISER))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::IN_SYSTEM_PATH))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::WARP))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::COLONISE_UI))
        return;

    ///if hostiles in system, flee!
    ///set ai state to request flee

    std::unordered_set<orbital*> free_planets;

    for(orbital* test_o : my_o->parent_system->orbitals)
    {
        if(test_o->type == orbital_info::PLANET && test_o->parent_empire == nullptr)
        {
            free_planets.insert(test_o);
        }
    }

    if(free_planets.size() == 0)
        return;

    for(orbital* orb_1 : my_o->parent_system->orbitals)
    {
        if(orb_1->type != orbital_info::FLEET)
            continue;

        ///enforce colonising is happening in current system
        if(!orb_1->command_queue.is_currently_colonising())
            continue;

        ///slight hack, but we're making sure that the only way our current colony target
        ///is not in the free pool is if someone else has nicked it, not us
        if(orb_1 == my_o)
            continue;

        ///get colonising target gets first colonising target, but we've enforced in system
        free_planets.erase(orb_1->command_queue.get_colonising_target());

        if(free_planets.size() == 0)
            return;
    }

    if(my_o->command_queue.is_ever(object_command_queue_info::COLONISE))
    {
        orbital* colony_target = my_o->command_queue.get_colonising_target();

        ///should be impossible
        if(colony_target == nullptr)
            return;

        if(colony_target->parent_system != my_o->parent_system)
            return;

        ///someone else is colonising the planet, cancel our colonisation efforts
        if(free_planets.find(colony_target) == free_planets.end())
        {
            my_o->command_queue.cancel();
        }

        return;
    }

    if(free_planets.size() > 0)
    {
        for(ship* s : sm->ships)
        {
            if(s->can_colonise())
            {
                my_o->command_queue.cancel();
                my_o->command_queue.colonise(*free_planets.begin(), s);

                //std::cout << (*free_planets.begin())->name << std::endl;

                return;
            }
        }
    }
}

void try_mine(ship_manager* sm, orbital* my_o)
{
    if(!sm->any_with_element(ship_component_elements::ORE_HARVESTER))
    {
        my_o->mining_target = nullptr;
        return;
    }

    if(my_o->command_queue.is_ever(object_command_queue_info::IN_SYSTEM_PATH))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::ANCHOR))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::WARP))
        return;

    if(my_o->command_queue.is_ever(object_command_queue_info::ANCHOR_UI))
        return;

    std::unordered_set<orbital*> free_asteroids;

    for(orbital* test_o : my_o->parent_system->orbitals)
    {
        if(test_o->type == orbital_info::ASTEROID && test_o->is_resource_object)
        {
            free_asteroids.insert(test_o);
        }
    }

    if(free_asteroids.size() == 0)
        return;

    for(orbital* orb_1 : my_o->parent_system->orbitals)
    {
        if(orb_1->type != orbital_info::FLEET)
            continue;

        if(orb_1 == my_o)
            continue;

        //if(!orb_1->is_mining)
        //    continue;

        free_asteroids.erase(orb_1->mining_target);

        if(free_asteroids.size() == 0)
            return;
    }

    if(free_asteroids.size() == 0)
        return;

    for(ship* s : sm->ships)
    {
        component* c = s->get_component_with_primary(ship_component_elements::ORE_HARVESTER);

        if(c == nullptr)
            continue;

        //my_o->command_queue.cancel();
        my_o->command_queue.anchor(*free_asteroids.begin());

        my_o->mining_target = *free_asteroids.begin();

        return;
    }
}

void try_decolonising(orbital* o)
{
    ship_manager* sm = (ship_manager*)o->data;

    if(!sm->majority_of_type(ship_type::MILITARY))
        return;

    if(sm->decolonising)
        return;

    if(sm->any_in_combat())
        return;

    if(sm->any_derelict())
        return;

    if(!sm->can_move_in_system())
        return;

    if(o->command_queue.is_ever(object_command_queue_info::WARP))
        return;

    if(o->command_queue.is_ever(object_command_queue_info::IN_SYSTEM_PATH))
        return;

    for(orbital* test_orbital : o->parent_system->orbitals)
    {
        if(test_orbital->type != orbital_info::PLANET)
            continue;

        if(test_orbital->parent_empire == o->parent_empire)
            continue;

        if(o->parent_empire == nullptr || !o->parent_empire->is_hostile(test_orbital->parent_empire))
            continue;

        o->command_queue.transfer(test_orbital->absolute_pos, o, o->parent_system, true, false, true);
    }
}

void clear_ai_state(ai_fleet& fleet)
{
    fleet.ai_state = ai_empire_info::IDLE;
}

///this takes like 4ms
void ai_fleet::tick_fleet(ship_manager* ship_manage, orbital* o, all_battles_manager& all_battles, system_manager& system_manage, bool has_full_ai)
{
    ///change this whole function to work with idle state and defend state

    auto warp_path = o->command_queue.get_warp_destinations();

    if(warp_path.size() > 0)
    {
        ship_manage->ai_controller.on_route_to = warp_path.back();
    }
    else
    {
        ship_manage->ai_controller.on_route_to = nullptr;
    }

    auto timer = MAKE_AUTO_TIMER();

    timer.start();

    if(has_full_ai)
    {
        battle_manager* bm = all_battles.get_battle_involving(ship_manage);

        if(bm != nullptr)
        {
            //printf("in battle\n");

            if(bm->can_end_battle_peacefully(ship_manage->parent_empire))
            {
                all_battles.end_battle_peacefully(bm);
            }
        }
    }

    /*if(ship_manage->any_colonising())
    {
        printf("any\n");
    }*/

    if(ship_manage->any_in_combat())
        return;

    ///we're not in combat here

    if(ship_manage->auto_resupply)
    {
        ///spread resupply check across this many frames
        uint32_t resupply_frames = 10;

        if(((current_resupply_frame + resupply_offset) % resupply_frames) == 0 && ship_manage->should_resupply())
        {
            ship_manage->resupply(ship_manage->parent_empire, false);

            //printf("ai resupply\n");
        }
    }


    current_resupply_frame++;

    ///split fleets up after we finish basic ai
    if(ship_manage->any_derelict())
    {
        clear_ai_state(*this);
        return;
    }

    if(ship_manage->auto_colonise)
    {
        try_colonise(ship_manage, o);
    }

    if(ship_manage->auto_harvest_ore)
    {
        try_mine(ship_manage, o);
    }

    ///AI ONLY PAST THIS POINT
    if(!has_full_ai)
    {
        return;
    }

    if(!ship_manage->majority_of_type(ship_type::MILITARY))
    {
        clear_ai_state(*this);
        return;
    }

    ///fly around?
    if(!ship_manage->can_engage())
    {
        clear_ai_state(*this);
        return;
    }

    orbital_system* os = o->parent_system;

    empire* my_empire = ship_manage->parent_empire;

    std::vector<std::pair<orbital*, ship_manager*>> targets;

    for(orbital* other : os->orbitals)
    {
        if(other->type != orbital_info::FLEET)
        {
            continue;
        }

        ship_manager* other_ships = (ship_manager*)other->data;

        if(!my_empire->is_hostile(other_ships->parent_empire))
            continue;

        if(other_ships->parent_empire == my_empire)
            continue;

        if(!other_ships->can_engage())
            continue;

        ///STEALTH PROBABLY BORKED
        ///Why are we skipping ships that are decolonising?
        if(my_empire->available_scanning_power_on(other_ships, system_manage) <= 0 && !other_ships->any_in_combat() && !other_ships->any_colonising() && !other_ships->decolonising)
            continue;

        targets.push_back({other, other_ships});
    }

    if(targets.size() == 0)
    {
        try_decolonising(o);

        clear_ai_state(*this);
        return;
    }

    ///perform !suicidal check in here once we can instruct the ai to run ships away
    ///can use same check for fuel etc
    ///but the ai currently has no friendly systems (or resources) so can't run away *to* anywhere
    ///will have to check for nearby systems etc

    auto nearest = get_nearest(targets, o);

    assert(nearest.first != nullptr);

    o->transfer(nearest.first->absolute_pos, o->parent_system);

    ///can_engage deliberately does not check any_in_combat (so we can chain battles), so be careful that we don't do this
    ///currently fine as check is above
    if(os->can_engage(o, nearest.first))
    {
        battle_manager* bm = all_battles.make_new_battle({o, nearest.first});

        //o->data->requesting_or_in_battle = true;
    }
}
