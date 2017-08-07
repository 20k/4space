#include "ship_customiser.hpp"

#include "top_bar.hpp"
#include <imgui/imgui.h>

void ship_customiser::tick()
{
    if(!top_bar::active[top_bar_info::SHIP_CUSTOMISER])
        return;

    ImGui::Begin("Ship Customisation", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS);


    ImGui::End();
}
