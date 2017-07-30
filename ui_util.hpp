#ifndef UI_UTIL_HPP_INCLUDED
#define UI_UTIL_HPP_INCLUDED

#include <imgui/imgui.h>
#include "drag_and_drop.hpp"

#define IMGUI_JUST_TEXT_WINDOW ImGuiWindowFlags_AlwaysAutoResize | \
                     ImGuiWindowFlags_NoBringToFrontOnFocus| \
                     ImGuiWindowFlags_NoCollapse| \
                     ImGuiWindowFlags_NoFocusOnAppearing | \
                     ImGuiWindowFlags_NoInputs | \
                     ImGuiWindowFlags_NoMove | \
                     ImGuiWindowFlags_NoResize | \
                     ImGuiWindowFlags_NoTitleBar

inline
void imgui_hp_bar(float fraction, vec3f col, vec2f dim)
{
    constexpr int num_divisions = 100;

    float num_filled = floor(fraction * num_divisions);

    float extra = fraction * num_divisions - num_filled;

    float vals[num_divisions];

    for(int i=0; i<num_divisions; i++)
    {
        if(i < num_filled)
        {
            vals[i] = 1.f;
        }
        else if(i == num_filled)
        {
            vals[i] = extra;
        }
        else
        {
            vals[i] = 0;
        }
    }

    ImGui::PlotHistogram("", vals, num_divisions, 0, nullptr, 0, 1, ImVec2(dim.x(), dim.y()));
}

namespace ImGui
{
    extern bool suppress_clicks;
    extern int suppress_frames;

    inline
    bool IsItemClicked_DragCompatible()
    {
        return ImGui::IsItemHovered() && !ImGui::IsMouseDragging() && ImGui::IsMouseReleased(0) && !global_drag_and_drop.is_dragging() && !suppress_clicks;
    }

    inline
    bool IsItemClicked_Registered()
    {
        if(ImGui::IsItemClicked())
        {
            suppress_clicks = true;
            return true;
        }

        return false;
    }

    inline
    bool IsItemClicked_UnRegistered()
    {
        return ImGui::IsItemClicked();
    }

    inline
    void tick_suppress_frames()
    {
        if(suppress_clicks)
        {
            suppress_frames--;
        }

        if(suppress_frames <= 0)
        {
            suppress_clicks = false;
            suppress_frames = 2;
        }
    }
}

#endif // UI_UTIL_HPP_INCLUDED
