#ifndef DRAG_AND_DROP_HPP_INCLUDED
#define DRAG_AND_DROP_HPP_INCLUDED

#include <imgui/imgui.h>
#include <string>
#include <map>
#include <vec/vec.hpp>

namespace drag_and_drop_info
{
    enum type
    {
        NONE = 0,
        ORBITAL,
        SHIP,
    };
}

struct window_info
{
    vec2f pos;
    bool locked = false;
};

struct drag_and_drop
{
    int frames_to_drop = 2;

    bool dragging = false;
    void* data = nullptr;

    std::map<std::string, window_info> window_map;

    drag_and_drop_info::type currently_dragging = drag_and_drop_info::NONE;

    vec2f locked_window_pos = {0,0};

    std::string current_tag;
    std::string tooltip_str;

    void begin_drag_section(const std::string& tag);
    void end_drag_section(const std::string& tag);

    void tick_locking_window();

    void begin_dragging(void* data, drag_and_drop_info::type type, const std::string& tooltip_str);
    void finish_dragging();

    bool let_go_on_item();

    void tick();
};

extern drag_and_drop global_drag_and_drop;

#endif // DRAG_AND_DROP_HPP_INCLUDED
