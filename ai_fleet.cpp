#include "ai_fleet.hpp"
#include "system_manager.hpp"
#include "empire.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include <assert.h>

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

void ai_fleet::tick_fleet(ship_manager* ship_manage, orbital* o, all_battles_manager& all_battles)
{
    ///split fleets up after we finish basic ai
    if(ship_manage->any_derelict())
    {
        return;
    }

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

    ///fly around?
    if(!ship_manage->can_engage())
        return;

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

        targets.push_back({other, other_ships});
    }

    if(targets.size() == 0)
        return;

    ///perform !suicidal check in here once we can instruct the ai to run ships away
    ///can use same check for fuel etc
    ///but the ai currently has no friendly systems (or resources) so can't run away *to* anywhere
    ///will have to check for nearby systems etc

    auto nearest = get_nearest(targets, o);

    assert(nearest.first != nullptr);

    o->transfer(nearest.first->absolute_pos);

    ///can_engage deliberately does not check any_in_combat (so we can chain battles), so be careful that we don't do this
    ///currently fine as check is above
    if(os->can_engage(o, nearest.first))
    {
        battle_manager* bm = all_battles.make_new_battle({o, nearest.first});
    }
}
