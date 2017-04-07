#include "event_system.hpp"
#include "system_manager.hpp"
#include "../../render_projects/imgui/imgui.h"
#include "util.hpp"

dialogue_node resolution =
{
    "Resolution",
    "Test resolution",
};

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

    ImGui::Begin((dialogue.header + "###DLOGUE").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    //ImGui::Text("Sample Dialogue here, also there, and everywhere");

    //ImGui::NewLine();

    ///we'll have to manually format this
    /*std::vector<std::string> dialogue_options
    {
        "Ruh roh, click this to not die!",
        "Well.... I guess this is dialogue",
        "I'm not very good at writing, we're all going to die",
    };*/

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

        if(selected < dialogue.travel.size())
        {
            next.dialogue = *(dialogue.travel[selected]);
        }

        parent->event_history.push_back(next);
    }

    ImGui::End();
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

game_event_manager::game_event_manager(orbital* o)
{
    arc_type = randf_s(0.f, game_event_info::COUNT);

    game_event first;
    first.alert_location = o;
    first.arc_type = arc_type;
    first.parent = this;
    first.event_num = 0;
    first.dialogue = dia_first; ///set from map the starting dialogues?

    event_history.push_back(first);
}

void game_event_manager::draw_ui()
{
    event_history.back().draw_ui();
}
