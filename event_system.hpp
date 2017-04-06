#ifndef EVENT_SYSTEM_HPP_INCLUDED
#define EVENT_SYSTEM_HPP_INCLUDED

namespace game_event_info
{
    ///star trek
    enum types
    {
        LOCAL_UPRISING, ///revolts on planet. Needs space -> planet interactiont to work
        DARK_PLANET, ///missing planet. Needs scanners to work fully, basic exploration will be a stopgap
        DARK_ASTEROID_BELT, ///missing belt, see above
        LOCAL_NATIVES, ///natives on planet doing something. Needs space -> planet, very little else (this is more lore)
        ANCIENT_PRECURSOR, ///found something. Includes alien tech, traces of civilisation.
        LONE_DERELICT, ///individual derelict, needs salvage mechanics and combat to work, research would help. Could be ancient
        ///implement these for the moment
        ///Not yet:
        ///Ringworld
        ///AI being born
        ///AI revolt (tie into above?)
        ///
    };
}

///
struct game_event
{

};

#endif // EVENT_SYSTEM_HPP_INCLUDED
