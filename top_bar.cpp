#include "top_bar.hpp"
#include "../../render_projects/imgui/imgui.h"

std::vector<std::string> top_bar::headers = top_bar_info::names;
std::map<top_bar_info::types, bool> top_bar::active;

bool top_bar::get_active(top_bar_info::types type)
{
    return active[type];
}

void top_bar::display()
{
    ImGui::Begin("###TOP_BAR", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    int num = 0;

    for(auto& i : headers)
    {
        std::string pad = "-";

        if(active[(top_bar_info::types)num])
        {
            pad = "+";
        }

        ImGui::Text((pad + i + pad).c_str());

        if(ImGui::IsItemClicked())
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


    ImGui::End();
}
