#ifndef EVENT_SYSTEM_HPP_INCLUDED
#define EVENT_SYSTEM_HPP_INCLUDED

#include <vector>
#include <deque>
#include <vec/vec.hpp>
#include <functional>

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

        COUNT,
    };
}

struct orbital;
struct orbital_system;
struct game_event_manager;
struct game_event;
struct empire;
struct fleet_manager;

struct dialogue_node
{
    std::string header;
    std::string text;
    std::vector<std::string> options;
    //std::vector<dialogue_node*> travel;
    std::vector<std::function<void(game_event&)>> onclick;

    bool is_open = true;
};

struct waiting_event
{
    //dialogue_node node;

    std::function<bool(game_event&)> is_finished;
};

namespace lone_derelict_dialogue
{
    extern dialogue_node resolution_destroyed;
    extern dialogue_node salvage_resolution;
    extern dialogue_node salvage_resolution_hostile;
    extern dialogue_node observation_resolution_hostile;
    extern dialogue_node observation_powerup;
    extern dialogue_node observation;
}

///Ok. This is basically a branching sequence of events, each event may trigger a new event depending on the circumstances
///so we need an event history, and the current event, which can just be the last event in the history
///that can be under event manager
///a game event is ONE single event
struct game_event
{
    int arc_type;
    int event_num;

    float scanning_difficulty = randf_s(0.f, 1.f);

    orbital* alert_location = nullptr;

    dialogue_node dialogue;

    void draw_ui();
    void tick(float step_s);

    void set_dialogue_open_state(bool open);

    game_event_manager* parent = nullptr;

    ///since this game event was created
    float time_alive_s = 0.f;

    std::deque<waiting_event> waiting_events;
};

///lets get a very basic interaction system going. "A thing has happened click between two options"
///then implement followon
///this manages one particularly game event, which is composed of sub game events
///will definitely need a regular tick
///Don't ditch any quest history so we can have logs
struct game_event_manager
{
    ///event_history.back() == current event
    std::vector<game_event> event_history;

    ///could be one of many
    empire* ancient_faction = nullptr;
    empire* interacting_faction = nullptr;
    fleet_manager* fleet_manage;

    int arc_type;
    ///basically determines the difficulty of this arc
    ///we may need to distribute this more intelligently than just prng
    float overall_tech_level = randf_s(0.f, 9.f);

    game_event make_next_event();

    game_event_manager(orbital* o, fleet_manager& fm);

    void set_faction(empire* e);
    void set_interacting_faction(empire* e);

    //notification_window window;

    void draw_ui();
    void tick(float step_s);

    float internal_time_s = 0.f;

    bool finished = false;
};

#endif // EVENT_SYSTEM_HPP_INCLUDED
