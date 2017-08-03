#include "context_menu.hpp"

#include <imgui/imgui.h>

contextable* context_menu::current = nullptr;

void context_menu::start()
{
    ImGui::Begin("Context Menu");
}

void context_menu::stop()
{
    ImGui::End();
}
