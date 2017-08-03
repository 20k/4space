#include "context_menu.hpp"

#include <imgui/imgui.h>

contextable* context_menu::current = nullptr;

void context_menu::start()
{
    if(!current)
        return;

    ImGui::Begin("Context Menu");
}

void context_menu::stop()
{
    if(!current)
        return;

    ImGui::End();
}
