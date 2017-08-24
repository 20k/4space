#ifndef IMGUI_CUSTOMISATION_HPP_INCLUDED
#define IMGUI_CUSTOMISATION_HPP_INCLUDED

#include <imgui/imgui.h>

namespace ImGui
{
    bool CloseButtonOverride(ImGuiID id, const ImVec2& pos, float radius);

    bool BeginOverride(const char* name, bool* p_open, ImGuiWindowFlags flags);
    bool BeginOverride(const char* name, bool* p_open, const ImVec2& size_on_first_use, float bg_alpha, ImGuiWindowFlags flags);
}

#endif // IMGUI_CUSTOMISATION_HPP_INCLUDED
