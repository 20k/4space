#ifndef AI_EMPIRE_HPP_INCLUDED
#define AI_EMPIRE_HPP_INCLUDED

#include <vector>
#include <map>
#include <unordered_set>

struct ship_manager;
struct empire;

///need to set ship ai to be able to path
///maybe literally do in ai_fleet?
namespace ai_empire_info
{
    enum ship_state
    {
        IDLE,
        DEFEND,
        SCOUT_INVASION_PREP,

        //DEFENCE,
        //STAGING,
        //INVASION,
    };
}

namespace ship_type
{
    enum types
    {
        COLONY,
        MINING,
        MILITARY,
        SCOUT,
        COUNT,
    };
}

struct system_manager;
struct orbital_system;

///ok new plan for empire ship states
///systems request a certain amount of ships for their defence
///we set to defence if we don't have enough
///more ships can be repurposed for defence depending on amount of hostile ships in a system
///otherwise returned to the general pool

using ship_general_state = ai_empire_info::ship_state;

struct fleet_manager;

struct invasion_info
{
    std::unordered_set<orbital_system*> systems;
    float invasion_timer_s = 0.f;
    float invasion_timer_max = 60 * 5;

    bool invading(orbital_system* sys)
    {
        return systems.find(sys) != systems.end();
    }
};

/*inline
bool operator==(const invasion_info& i1, const invasion_info& i2)
{
    return i1.sys == i2.sys;
}

struct invasion_hash
{
    size_t operator()(const invasion_info& i1) const {
        return std::hash<orbital_system*>()(i1.sys);
    }
};*/

struct ai_empire
{
    std::map<ship_manager*, ship_general_state> general_purpose_state;
    std::unordered_set<orbital_system*> speculatively_owned;
    //std::map<orbital_system*, invasion_info> invasion_targets;
    std::map<empire*, invasion_info> invasion_targets;

    bool at_war_in(empire* e, orbital_system* sys)
    {
        if(invasion_targets.find(e) == invasion_targets.end())
            return false;

        return invasion_targets[e].invading(sys);
    }

    void tick(float dt_s, fleet_manager& fm, system_manager& sm, empire* e);
};

#endif // AI_EMPIRE_HPP_INCLUDED
