#ifndef AI_EMPIRE_HPP_INCLUDED
#define AI_EMPIRE_HPP_INCLUDED

#include <vector>
#include <map>

struct ship_manager;

namespace ai_empire_info
{
    enum ship_state
    {
        IDLE,
        DEFENCE,
        INVASION,
    };
}
///ok new plan for empire ship states
///systems request a certain amount of ships for their defence
///we set to defence if we don't have enough
///more ships can be repurposed for defence depending on amount of hostile ships in a system
///otherwise returned to the general pool

using ship_general_state = ai_empire::ship_state;

struct ai_empire
{
    std::vector<ship_manager*> general_purpose;

    std::map<ship_manager*, ship_general_state> general_purpose_state;
};

#endif // AI_EMPIRE_HPP_INCLUDED
