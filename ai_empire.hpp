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
    orbital_system* sys = nullptr;
    float invasion_timer_s = 0.f;
    float invasion_time_max = 120.f;
};

inline
bool operator==(const invasion_info& i1, const invasion_info& i2)
{
    return i1.sys == i2.sys;
}

struct invasion_hash
{
    size_t operator()(const invasion_info& i1) const {
        return std::hash<orbital_system*>()(i1.sys);
    }
};

struct ai_empire
{
    std::map<ship_manager*, ship_general_state> general_purpose_state;
    std::unordered_set<orbital_system*> speculatively_owned;
    std::unordered_set<invasion_info, invasion_hash> invasion_targets;

    void tick(float dt_s, fleet_manager& fm, system_manager& sm, empire* e);
};

#endif // AI_EMPIRE_HPP_INCLUDED
