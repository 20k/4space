#include "event_system.hpp"
#include "system_manager.hpp"
#include "../../render_projects/imgui/imgui.h"
#include "util.hpp"
#include "ship.hpp"
#include "ship_definitions.hpp"
#include "empire.hpp"

dialogue_node resolution =
{
    "Resolution",
    "Test resolution",
};

void spawn_derelict(game_event& event)
{
    orbital* o = event.alert_location;
    empire* owner_faction = event.parent->ancient_faction;
    orbital_system* os = o->parent_system;
    fleet_manager* fleet_manage = event.parent->fleet_manage;

    ship_manager* derelict_fleet = fleet_manage->make_new();

    ship* new_ship = derelict_fleet->make_new_from(owner_faction->team_id, make_default());
    new_ship->name = "SS Toimplement name generation";

    orbital* onew_fleet = os->make_new(orbital_info::FLEET, 5.f);
    onew_fleet->orbital_angle = o->orbital_angle;
    onew_fleet->orbital_length = o->orbital_length + 40;
    onew_fleet->parent = os->get_base(); ///?
    onew_fleet->data = derelict_fleet;

    owner_faction->take_ownership(onew_fleet);
    owner_faction->take_ownership(derelict_fleet);

    new_ship->set_tech_level_from_empire(owner_faction);
    new_ship->randomise_make_derelict();

    onew_fleet->tick(0.f);
}

///we need to data drive the event system from here somehow
///maybe have ptrs to bools that get set if we click one (or store internally)
///that way the next event can see what the previous event picked
///or like, trigger events somehow from here
dialogue_node dia_first =
{
    "Hi",
    "Hello there you look like fun",
    {
        "Die die die aliens",
        "Oh you're totally right",
        "I like cereal",
    },
    {
        &resolution, &resolution, &resolution,
    },
    {
        spawn_derelict, spawn_derelict, spawn_derelict
    }
};


int present_dialogue(const std::vector<std::string>& options)
{
    int selected = -1;
    int num = 0;

    for(auto& str : options)
    {
        //std::string display_str = "(" + std::to_string(num + 1) + ") " + str;
        std::string display_str = str;

        if(display_str.length() > 0 && num != 0)
            ImGui::BulletText(display_str.c_str());

        if(display_str.length() > 0 && num == 0)
        {
            ImGui::Text(display_str.c_str());
            ImGui::NewLine();
        }

        if(ImGui::IsItemClicked())
        {
            selected = num;
        }

        num++;
    }

    return selected;
}

///somehow implement branching in here. Maybe we should literally make like a ptr structure of dialogue options?
/*std::vector<std::string> get_dialogue_and_options(int arc_type, int event_offset)
{
    if(event_offset == 0)
    {
        return {"Sample Dialogue here, also there, etc i'm tired ok", "Ruh roh, click this to not die!",
        "Well.... I guess this is dialogue",
        "I'm not very good at writing, we're all going to die",};
    }
    if(event_offset == 1)
    {
        return {"Whelp"};
    }

    return {""};
}*/

void game_event::draw_ui()
{
    if(dialogue.header == "")
        return;

    if(!dialogue.is_open)
        return;

    ImGui::Begin((dialogue.header + "###DLOGUE").c_str(), &dialogue.is_open, ImGuiWindowFlags_AlwaysAutoResize);

    std::vector<std::string> dialogue_options = {dialogue.text};

    ///patch dialogue options here if necessary
    for(auto& i : dialogue.options)
    {
        dialogue_options.push_back(i);
    }

    if(dialogue.options.size() == 0)
    {
        dialogue_options.push_back("Ok");
    }

    int selected = present_dialogue(dialogue_options) - 1;

    if(selected >= 0)
    {
        game_event next = parent->make_next_event();

        if(selected < dialogue.onclick.size() && dialogue.onclick[selected] != nullptr)
        {
            void (*fptr)(game_event&);

            fptr = dialogue.onclick[selected];
            fptr(*this);
        }

        if(selected < dialogue.travel.size())
        {
            next.dialogue = *(dialogue.travel[selected]);
        }

        parent->event_history.push_back(next);
    }

    ImGui::End();
}

void game_event::tick(float step_s)
{
    alert_location->has_quest_alert = true;

    /*if(alert_location->clicked)
    {
        alert_location->clicked = false;

        set_dialogue_open_state(!dialogue.is_open);
    }*/

    set_dialogue_open_state(alert_location->dialogue_open);
}

void game_event::set_dialogue_open_state(bool open)
{
    dialogue.is_open = open;
}

game_event game_event_manager::make_next_event()
{
    game_event ret;
    ret.arc_type = arc_type;
    ret.alert_location = event_history.back().alert_location;
    ret.parent = this;
    ret.event_num = event_history.size();

    return ret;
}

game_event_manager::game_event_manager(orbital* o, fleet_manager& pfleet_manage)
{
    arc_type = randf_s(0.f, game_event_info::COUNT);

    game_event first;
    first.alert_location = o;
    first.arc_type = arc_type;
    first.parent = this;
    first.event_num = 0;
    first.dialogue = dia_first; ///set from map the starting dialogues?

    event_history.push_back(first);

    ///fleet manager never expires, so this is fine
    fleet_manage = &pfleet_manage;
}

void game_event_manager::set_facton(empire* e)
{
    ancient_faction = e;
}

void game_event_manager::draw_ui()
{
    event_history.back().draw_ui();
}

void game_event_manager::tick(float step_s)
{
    internal_time_s += step_s;

    event_history.back().tick(step_s);
}
