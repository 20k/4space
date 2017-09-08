#include "top_bar.hpp"
#include "../../render_projects/imgui/imgui.h"
#include "ui_util.hpp"
#include "popup.hpp"

std::vector<std::string> top_bar::headers = top_bar_info::names;
std::map<top_bar_info::types, bool> top_bar::active;

bool top_bar::get_active(top_bar_info::types type)
{
    return active[type];
}

void top_bar::display(vec2i window_dim)
{
    static vec2i last_size;
    static bool has_size = false;

    if(has_size)
    {
        //ImGui::SetNextWindowSize(ImVec2(last_size.x(), last_size.y()));
        ImGui::SetNextWindowPos(ImVec2(window_dim.x()/2 - last_size.x()/2, last_size.y()/8));
    }
    else
    {
        ///this is not an error
        ImGui::SetNextWindowPos(ImVec2(0, window_dim.y() * 2));
    }

    ImGui::BeginOverride("###TOP_BAR", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    int num = 0;

    for(auto& i : headers)
    {
        std::string pad = "-";

        if(active[(top_bar_info::types)num])
        {
            pad = "+";
        }

        /*ImGui::NeutralText(pad + i + pad);

        if(ImGui::IsItemClicked_Registered())
        {
            active[(top_bar_info::types)num] = !active[(top_bar_info::types)num];
        }*/

        bool is_active = active[(top_bar_info::types)num];
        vec3f highlight_col = {0.4, 0.4, 0.4};
        ImGui::OutlineHoverText(i, highlight_col, {1,1,1}, true, {8,2}, 1, is_active, highlight_col/2, is_active);

        if(ImGui::IsItemClicked_Registered())
        {
            active[(top_bar_info::types)num] = !active[(top_bar_info::types)num];
        }

        if(num != headers.size()-1)
        {
            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();
        }

        num++;
    }

    auto dim = ImGui::GetWindowSize();

    last_size = {dim.x, dim.y};
    has_size = true;

    ImGui::End();
}
