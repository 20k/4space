#include "drag_and_drop.hpp"

void drag_and_drop::begin_drag_section(const std::string& tag)
{
    current_tag = tag;
}

void drag_and_drop::tick_locking_window()
{
    window_info& inf = window_map[current_tag];

    if(inf.locked)
    {
        ImGui::SetWindowPos({inf.pos.x(), inf.pos.y()});
    }
}

void drag_and_drop::begin_dragging(void* data, drag_and_drop_info::type type)
{
    window_info& inf = window_map[current_tag];

    inf.locked = true;
    inf.pos = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};
}

void drag_and_drop::finish_dragging()
{
    for(auto& i : window_map)
    {
        i.second.locked = false;
    }
}
