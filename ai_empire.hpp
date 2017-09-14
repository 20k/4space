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
    float invasion_timer_max_reconsider = 30 * 1;
    float invasion_timer_max_force = 60 * 5;

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
    std::unordered_set<orbital_system*> speculatively_owned;
    //std::map<orbital_system*, invasion_info> invasion_targets;
    std::map<empire*, invasion_info> invasion_targets;

    bool at_war_in(empire* e, orbital_system* sys)
    {
        if(invasion_targets.find(e) == invasion_targets.end())
            return false;

        return invasion_targets[e].invading(sys);
    }

    bool may_cancel_invasion(empire* e, orbital_system* sys)
    {
        if(invasion_targets.find(e) == invasion_targets.end())
            return false;

        return invasion_targets[e].invasion_timer_s >= invasion_targets[e].invasion_timer_max_reconsider;
    }

    bool must_cancel_invasion(empire* e, orbital_system* sys)
    {
        if(invasion_targets.find(e) == invasion_targets.end())
            return false;

        return invasion_targets[e].invasion_timer_s >= invasion_targets[e].invasion_timer_max_force;
    }

    bool system_being_invaded(orbital_system* sys)
    {
        for(auto& i : invasion_targets)
        {
            if(i.second.systems.find(sys) != i.second.systems.end())
                return true;
        }

        return false;
    }

    void cancel_invasion(empire* e, empire* my_empire);
    void cancel_invasion_system(orbital_system* sys);

    void tick(float dt_s, fleet_manager& fm, system_manager& sm, empire* e);

    float invasion_cooldown_max = 60 * 5.f;
    float invasion_cooldown_s = invasion_cooldown_max;
};

#endif // AI_EMPIRE_HPP_INCLUDED
