#include "ai_fleet.hpp"
#include "system_manager.hpp"
#include "empire.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include <assert.h>
#include "profile.hpp"

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

///this takes like 4ms
void ai_fleet::tick_fleet(ship_manager* ship_manage, orbital* o, all_battles_manager& all_battles, system_manager& system_manage)
{
    ///change this whole function to work with idle state and defend state

    auto timer = MAKE_AUTO_TIMER();

    timer.start();

    battle_manager* bm = all_battles.get_battle_involving(ship_manage);

    if(bm != nullptr)
    {
        //printf("in battle\n");

        if(bm->can_end_battle_peacefully(ship_manage->parent_empire))
        {
            all_battles.end_battle_peacefully(bm);
        }
    }

    if(ship_manage->any_in_combat())
        return;

    ///we're not in combat here

    ///spread resupply check across this many frames
    uint32_t resupply_frames = 10;

    if(((current_resupply_frame + resupply_offset) % resupply_frames) == 0 && ship_manage->should_resupply())
    {
        ship_manage->resupply(ship_manage->parent_empire);

        //printf("ai resupply\n");
    }

    current_resupply_frame++;

    ///split fleets up after we finish basic ai
    if(ship_manage->any_derelict())
    {
        ai_state = 0;
        return;
    }

    ///fly around?
    if(!ship_manage->can_engage())
    {
        ai_state = 0;
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
        ai_state = 0;
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
    }
}
