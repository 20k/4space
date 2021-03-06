#include "tooltip_handler.hpp"

#include <imgui/imgui.h>

std::string tooltip::tool;

void tooltip::add(const std::string& str)
{
    tool += str + "\n";
}

void tooltip::set_clear_tooltip()
{
    while(tool.size() > 0 && tool.back() == '\n')
    {
        tool.pop_back();
    }

    if(tool.size() == 0)
        return;

    ImGui::SetTooltip(tool.c_str());

    tool.clear();
}
