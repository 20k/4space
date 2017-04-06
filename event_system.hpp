#ifndef EVENT_SYSTEM_HPP_INCLUDED
#define EVENT_SYSTEM_HPP_INCLUDED

namespace game_event_info
{
    ///star trek
    ///basic mechanics: Scanning, salvage, combat, research, player -> lore interaction, resources, diplomacy
    ///Implement these first
    ///possible consequences: New tech, new ship, more resources, hostile ship spawning, hostile crew boarding, ship damaged
    ///new planet, new asteroids (resource), new solar system, ship destroyed, crew destroyed, lose tech, lose resources,
    ///empire being born, planet colonised, planet taken over by hostile species, robot uprising (new empire?), robot augmentation,
    ///ship augmentation, solar system destroyed, planet wiped, diplomacy
    ///region of space mapped, region of space unmapped, hidden secrets uncovered (dark planet/belt/stars)
    enum types
    {
        LOCAL_UPRISING, ///revolts on planet. Needs space -> planet interactiont to work
        DARK_PLANET, ///missing planet. Needs scanners to work fully, basic exploration will be a stopgap
        DARK_ASTEROID_BELT, ///missing belt, see above
        DARK_STAR, ///missing star, see above
        LOCAL_NATIVES, ///natives on planet doing something. Needs space -> planet, very little else (this is more lore)
        ANCIENT_PRECURSOR, ///found something. Includes alien tech, traces of civilisation.
        LONE_DERELICT, ///individual derelict, needs salvage mechanics and combat to work, research would help. Could be ancient
        ///implement these for the moment
        ///Not yet:
        ///Ringworld
        ///AI being born
        ///AI revolt (tie into above?)
        ///something wrong with the star
    };
}

///
struct game_event
{

};

#endif // EVENT_SYSTEM_HPP_INCLUDED
