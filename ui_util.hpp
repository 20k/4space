#ifndef UI_UTIL_HPP_INCLUDED
#define UI_UTIL_HPP_INCLUDED

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
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

    inline
    ImVec4 GetStyleCol(ImGuiCol name)
    {
        auto res = ImGui::GetColorU32(name, 1.f);

        return ImGui::ColorConvertU32ToFloat4(res);
    }

    inline
    void OutlineHoverText(const std::string& txt, vec3f col, vec3f text_col)
    {
        ImGui::BeginGroup();

        //auto cursor_pos = ImGui::GetCursorPos();
        auto screen_pos = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(text_col.x(), text_col.y(), text_col.z(), 1));

        auto dim = ImGui::CalcTextSize(txt.c_str());

        dim.y += 2;
        dim.x += 1;

        ImVec2 p2 = screen_pos;
        p2.x += dim.x;
        p2.y += dim.y;

        int thickness = 1;

        if(ImGui::IsWindowHovered() && ImGui::IsRectVisible(dim) && ImGui::IsMouseHoveringRect(screen_pos, p2))
        {
            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x(), col.y(), col.z(), 1));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::PopStyleColor(3);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x, screen_pos.y));

            ImGui::PushStyleColor(ImGuiCol_Button, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetStyleCol(ImGuiCol_WindowBg));

            if(!ImGui::IsMouseDown(0))
                ImGui::Button("", dim);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x, screen_pos.y));

            ImGui::Text(txt.c_str());
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetStyleCol(ImGuiCol_WindowBg));

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x, screen_pos.y));

            ImGui::Text(txt.c_str());
        }

        ImGui::PopStyleColor(4);

        ImGui::EndGroup();
    }

    inline
    void OutlineHoverTextAuto(const std::string& txt, vec3f text_col)
    {
        return OutlineHoverText(txt, text_col/2.f, text_col);
    }
}

#endif // UI_UTIL_HPP_INCLUDED
