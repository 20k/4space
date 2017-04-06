#include "notification_window.hpp"

#include <SFML/Graphics.hpp>

#include "../../render_projects/imgui/imgui.h"

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
