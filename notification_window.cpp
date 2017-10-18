#include "notification_window.hpp"

#include <SFML/Graphics.hpp>

#include "../../render_projects/imgui/imgui.h"
#include "ui_util.hpp"

void notification_window::tick_draw()
{
    ImGui::BeginOverride(title.c_str());

    ImGui::clamp_window_to_screen();

    for(int id = 0; id < (int)elements.size(); id++)
    {
        text_element& elem = elements[id];

        ImGui::Text(elem.text.c_str());

        if(ImGui::IsItemClicked_Registered())
        {
            elem.pressed = true;

            elem.toggle_state = !elem.toggle_state;
        }
        else
        {
            elem.pressed = false;
        }
    }

    ImGui::End();
}

notification_window* notification_manager::make_new()
{
    notification_window* win = new notification_window;

    windows.push_back(win);

    return win;
}

void notification_manager::destroy(notification_window* win)
{
    for(auto it = windows.begin(); it != windows.end(); it++)
    {
        if((*it) == win)
        {
            windows.erase(it);
            return;
        }
    }
}
