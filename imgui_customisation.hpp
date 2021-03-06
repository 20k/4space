#ifndef IMGUI_CUSTOMISATION_HPP_INCLUDED
#define IMGUI_CUSTOMISATION_HPP_INCLUDED

#include <imgui/imgui.h>

/*namespace ImGui
{
    bool CloseButtonOverride(ImGuiID id, const ImVec2& pos, float radius);

    bool BeginOverride(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
    bool BeginOverride(const char* name, bool* p_open, const ImVec2& size_on_first_use, float bg_alpha = -1.0f, ImGuiWindowFlags flags = 0);
}*/

namespace ImGui
{
    bool BeginOverride(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
    bool BeginOverride(const char* name, bool* p_open, const ImVec2& size_on_first_use, float bg_alpha = -1.0f, ImGuiWindowFlags flags = 0);
}

#endif // IMGUI_CUSTOMISATION_HPP_INCLUDED
