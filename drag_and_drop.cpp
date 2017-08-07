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
    frames_to_drop = 3;

    window_info& inf = window_map[current_tag];

    inf.locked = true;
    inf.pos = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};

    dragging = true;

    tooltip_str = _tooltip_str;

    currently_dragging = type;
    data = _data;

    sf::Mouse mouse;

    drag_mouse_pos = {mouse.getPosition().x, mouse.getPosition().y};
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

    //window_info& inf = window_map[current_tag];

    sf::Mouse mouse;

    bool lclick = mouse.isButtonPressed(sf::Mouse::Left);

    if(ImGui::IsItemHovered() && !lclick && dragging)
    {
        finish_dragging();

        if(adequate_drag_distance())
            return true;
        else
            return false;
    }

    return false;
}

bool drag_and_drop::let_go_on_window()
{
    if(!dragging)
        return false;

    //window_info& inf = window_map[current_tag];

    sf::Mouse mouse;

    bool lclick = mouse.isButtonPressed(sf::Mouse::Left);

    if(ImGui::IsWindowHovered() && !lclick && dragging)
    {
        finish_dragging();

        if(adequate_drag_distance())
            return true;
        else
            return false;
    }

    return false;
}

bool drag_and_drop::let_go_outside_window()
{
    if(!dragging)
        return false;

    //window_info& inf = window_map[current_tag];

    sf::Mouse mouse;

    bool lclick = mouse.isButtonPressed(sf::Mouse::Left);

    if(!ImGui::IsWindowHovered() && !lclick && dragging)
    {
        finish_dragging();

        if(adequate_drag_distance())
            return true;
        else
            return false;
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

    if(adequate_drag_distance())
        ImGui::SetTooltip(tooltip_str.c_str());
}

bool drag_and_drop::adequate_drag_distance()
{
    sf::Mouse mouse;

    vec2f nmouse_pos = {mouse.getPosition().x, mouse.getPosition().y};

    float dist = (nmouse_pos - drag_mouse_pos).length();

    return dist >= min_drag_distance;
}

bool drag_and_drop::is_dragging()
{
    return dragging && adequate_drag_distance();
}
