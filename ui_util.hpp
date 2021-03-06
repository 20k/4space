#ifndef UI_UTIL_HPP_INCLUDED
#define UI_UTIL_HPP_INCLUDED

#include "imgui_customisation.hpp"
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

#define IMGUI_JUST_TEXT_WINDOW_INPUTS ImGuiWindowFlags_AlwaysAutoResize | \
                     ImGuiWindowFlags_NoCollapse| \
                     ImGuiWindowFlags_NoFocusOnAppearing | \
                     ImGuiWindowFlags_NoMove | \
                     ImGuiWindowFlags_NoResize | \
                     ImGuiWindowFlags_NoTitleBar

inline
float get_title_bar_height()
{
    return ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.0f;
}

inline
vec3f hp_frac_to_col(float hp_frac)
{
    vec3f max_col = {1.f, 1.f, 1.f};
    vec3f min_col = {1.f, 0.f, 0.f};

    return mix(min_col, max_col, hp_frac);
}

inline
void imgui_hp_bar(float fraction, vec4f col, vec2f dim)
{
    constexpr int num_divisions = 20;

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

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(col.x(), col.y(), col.z(), col.w()));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    ImGui::PlotHistogram("", vals, num_divisions, 0, nullptr, 0, 1, ImVec2(dim.x(), dim.y()));
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(1);
}

struct empire;

struct has_context_menu
{
    bool context_request_open = false;
    bool context_request_close = false;

    bool context_is_open = false;

    void context_tick_menu();
};

namespace sf
{
    struct RenderWindow;
}

namespace ImGui
{
    extern bool suppress_keyboard;
    extern bool suppress_clicks;
    extern int suppress_frames;
    extern vec2i screen_dimensions;
    extern vec2f to_offset_mouse;
    extern bool lock_mouse;
    static inline vec2f lock_dir;
    static inline vec2f lock_pos;

    extern std::vector<std::string> to_skip_frosting;

    void DoFrosting(sf::RenderWindow& win);
    void SkipFrosting(const std::string& name);

    inline
    void set_screen_dimensions(vec2i sdim)
    {
        screen_dimensions = sdim;
    }

    ///allow bottom to go off screen, what we're really concerned about is the title bar
    void clamp_window_to_screen();

    inline
    bool IsItemClicked_DragCompatible()
    {
        return ImGui::IsItemHovered() && !ImGui::IsMouseDragging() && ImGui::IsMouseReleased(0) && !global_drag_and_drop.is_dragging() && !suppress_clicks;
    }

    inline
    bool IsItemClicked_Registered(int num = 0)
    {
        if(ImGui::IsItemClicked(num))
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
    void reset_suppress_clicks()
    {
        suppress_clicks = true;
        suppress_frames = 2;
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
    void OutlineHoverText(const std::string& txt, vec3f col, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false, vec3f hover_col = {-1, -1, -1}, int force_hover_thickness = 0)
    {
        ImGui::BeginGroup();

        //auto cursor_pos = ImGui::GetCursorPos();
        auto screen_pos = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(text_col.x(), text_col.y(), text_col.z(), 1));

        auto dim = ImGui::CalcTextSize(txt.c_str());

        dim.x += dim_extra.x();
        dim.y += dim_extra.y();

        dim.y += 2;
        dim.x += 1;

        ImVec2 p2 = screen_pos;
        p2.x += dim.x;
        p2.y += dim.y;

        auto win_bg_col = GetStyleCol(ImGuiCol_WindowBg);

        bool currently_hovered = ImGui::IsWindowHovered() && ImGui::IsRectVisible(dim) && ImGui::IsMouseHoveringRect(screen_pos, p2) && hover;

        if(currently_hovered || force_hover)
        {
            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::PushStyleColor(ImGuiCol_Button, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetStyleCol(ImGuiCol_WindowBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetStyleCol(ImGuiCol_WindowBg));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::PopStyleColor(3);

            if(force_hover && !currently_hovered)
            {
                thickness = force_hover_thickness;
            }

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x(), col.y(), col.z(), 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x(), col.y(), col.z(), 1));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::PopStyleColor(3);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x, screen_pos.y));

            auto button_col = GetStyleCol(ImGuiCol_WindowBg);

            if(hover_col.x() != -1)
            {
                button_col = ImVec4(hover_col.x(), hover_col.y(), hover_col.z(), 1);
            }

            ImGui::PushStyleColor(ImGuiCol_Button, button_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_col);

            if((!ImGui::IsMouseDown(0) && currently_hovered) || (force_hover && !currently_hovered))
                ImGui::Button("", dim);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + dim_extra.x()/2.f, screen_pos.y + dim_extra.y()/2.f));

            ImGui::Text(txt.c_str());
        }
        else
        {
            win_bg_col.w = 0.f;

            ImGui::PushStyleColor(ImGuiCol_Button, win_bg_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, win_bg_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, win_bg_col);

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - thickness, screen_pos.y - thickness));

            ImGui::Button("", ImVec2(dim.x + thickness*2, dim.y + thickness*2));

            ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + dim_extra.x()/2.f, screen_pos.y + dim_extra.y()/2.f));

            ImGui::Text(txt.c_str());
        }

        ImGui::PopStyleColor(4);

        ImGui::EndGroup();
    }

    inline
    void ToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
    {
        OutlineHoverText(txt, highlight_col, col, true, {8.f, 2.f}, 1, is_active, highlight_col/4.f, is_active);
    }

    inline
    void SolidToggleTextButton(const std::string& txt, vec3f highlight_col, vec3f col, bool is_active)
    {
        if(is_active)
            OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col, 1);
        else
            OutlineHoverText(txt, highlight_col, {1,1,1}, true, {8, 2}, 1, true, highlight_col/4.f, 1);
    }

    void SolidSmallButton(const std::string& txt, vec3f highlight_col, vec3f col, bool force_hover, vec2f dim = {0,0});

    inline
    void OutlineHoverTextAuto(const std::string& txt, vec3f text_col, bool hover = true, vec2f dim_extra = {0,0}, int thickness = 1, bool force_hover = false)
    {
        return OutlineHoverText(txt, text_col/2.f, text_col, hover, dim_extra, thickness, force_hover);
    }

    inline
    void TextColored(const std::string& str, vec3f col)
    {
        TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
    }

    inline
    void Text(const std::string& str)
    {
        ImGui::Text(str.c_str());
    }
}

struct popout_button
{
    void start(ImVec2 pos, ImVec2 dim, bool offset = true, std::string name = "dfsdf")
    {
        float ypad = get_title_bar_height() + ImGui::GetStyle().FramePadding.y*4;

        if(!offset)
        {
            ypad = 0;
        }

        ImGui::SetNextWindowPos(ImVec2(pos.x + dim.x - ImGui::GetStyle().FramePadding.x, pos.y + ypad));

        ImGui::BeginOverride(name.c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW_INPUTS);

        //ImGui::Button(">\n>\n>\n>\n>\n>\n>\n>");

        //ImGui::End();
    }

    void finish()
    {
        ImGui::End();
    }
};

#endif // UI_UTIL_HPP_INCLUDED
