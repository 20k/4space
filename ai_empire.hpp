#ifndef AI_EMPIRE_HPP_INCLUDED
#define AI_EMPIRE_HPP_INCLUDED

#include <vector>
#include <map>

struct ship_manager;
struct empire;

///need to set ship ai to be able to path
///maybe literally do in ai_fleet?
namespace ai_empire_info
{
    enum ship_state
    {
        IDLE,
        DEFENCE,
        STAGING,
        INVASION,
    };
}

struct system_manager;

///ok new plan for empire ship states
///systems request a certain amount of ships for their defence
///we set to defence if we don't have enough
///more ships can be repurposed for defence depending on amount of hostile ships in a system
///otherwise returned to the general pool

using ship_general_state = ai_empire_info::ship_state;

struct ai_empire
{
    std::map<ship_manager*, ship_general_state> general_purpose_state;

    void tick(system_manager& sm, empire* e);
};

#endif // AI_EMPIRE_HPP_INCLUDED
