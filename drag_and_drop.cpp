#include "drag_and_drop.hpp"

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
