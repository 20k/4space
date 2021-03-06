#ifndef AI_FLEET_HPP_INCLUDED
#define AI_FLEET_HPP_INCLUDED

#include <stdint.h>

struct ship_manager;
struct orbital;
struct battle_manager;
struct all_battles_manager;
struct system_manager;
struct orbital_system;

///fleet manager owns the ai_fleet?
///would simplify dependency management as it all already works fine
///but then... difficult to centralise
///I suppose fleets are the only thing that needs to be centralised
///and we're talking about fleet ai which is super low level, like engage enemies
///but then it needs high level control and strategy, like take fleet between systems
///higher level ai ie embedded in empire will have access to fleets, and i guess their fleet ai
///so can give direction to ai fleets in there
///makes sense, lets roll
///may contain state
struct ai_fleet
{
    static uint32_t gid;

    uint32_t resupply_offset = gid++;
    uint32_t current_resupply_frame = 0;

    ///externally managed
    int ai_state = 0;
    orbital_system* on_route_to = nullptr;

    void tick_fleet(ship_manager* ship_manage, orbital* o, all_battles_manager& all_battles, system_manager& system_manage, bool has_full_ai);
};

#endif // AI_FLEET_HPP_INCLUDED
