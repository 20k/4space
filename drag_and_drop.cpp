#include "drag_and_drop.hpp"
#include <SFML/Graphics.hpp>

drag_and_drop global_drag_and_drop;

void drag_and_drop::begin_drag_section(const std::string& tag)
{
    current_tag = tag;

    tick_locking_window();
}

void drag_and_drop::tick_locking_window()
{
    window_info& inf = window_map[current_tag];

    if(inf.locked)
    {
        ImGui::SetNextWindowPos({inf.pos.x(), inf.pos.y()});
    }
}

void drag_and_drop::begin_dragging(void* _data, drag_and_drop_info::type type, const std::string& _tooltip_str)
{
    frames_to_drop = 2;

    window_info& inf = window_map[current_tag];

    inf.locked = true;
    inf.pos = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};

    dragging = true;

    tooltip_str = _tooltip_str;

    currently_dragging = type;
    data = _data;
}

void drag_and_drop::finish_dragging()
{
    for(auto& i : window_map)
    {
        i.second.locked = false;
    }

    dragging = false;
}

bool drag_and_drop::let_go_on_item()
{
    if(!dragging)
        return false;

    window_info& inf = window_map[current_tag];

    sf::Mouse mouse;

    bool lclick = mouse.isButtonPressed(sf::Mouse::Left);

    if(ImGui::IsItemHovered() && !lclick && dragging)
    {
        finish_dragging();

        return true;
    }

    return false;
}

void drag_and_drop::tick()
{
    if(!dragging)
        return;

    sf::Mouse mouse;

    if(!mouse.isButtonPressed(sf::Mouse::Left))
    {
        frames_to_drop--;
    }

    if(frames_to_drop <= 0)
    {
        finish_dragging();
    }

    ImGui::SetTooltip(tooltip_str.c_str());
}
