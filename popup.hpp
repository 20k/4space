#ifndef POPUP_HPP_INCLUDED
#define POPUP_HPP_INCLUDED

#include <string>
#include <deque>
#include <vector>
#include <map>
#include "../../render_projects/imgui/imgui.h"
#include "top_bar.hpp"

namespace popup_element_type
{
    enum types
    {
        RESUPPLY,
        ENGAGE,
        ENGAGE_COOLDOWN,
        COLONISE,
        DECLARE_WAR,
        DECLARE_WAR_SURE,
        COUNT
    };
}

struct button_element
{
    std::string name;
    bool pressed = false;
};

struct popup_element
{
    static int gid;
    int id = gid++;

    std::string header;
    std::vector<std::string> data;
    std::deque<bool> checked;
    bool mergeable = false;
    bool toggle_clickable = false;

    std::map<popup_element_type::types, button_element> buttons_map;

    void* element = nullptr;

    void try_set_button_map(bool should_set, popup_element_type::types type, const std::string& name)
    {
        if(!should_set)
        {
            buttons_map.erase(type);
            return;
        }
        else
        {
            buttons_map[type].name = name;
        }
    }
};

struct popup_info
{
    std::vector<popup_element> elements;

    popup_element* fetch(void* element)
    {
        for(auto& i : elements)
        {
            if(i.element == element)
                return &i;
        }

        return nullptr;
    }

    void rem(void* element)
    {
        for(int i=0; i<elements.size(); i++)
        {
            if(elements[i].element == element)
            {
                elements.erase(elements.begin() + i);
                return;
            }
        }
    }

    bool going = false;

    bool declaring_war = false;

    /*bool started = false;

    void imgui_begin()
    {
        if(elements.size() == 0)
            going = false;

        if(!going)
            return;

        started = true;

        ImGui::Begin(("Selected###INFO_PANEL"), nullptr, ImVec2(0,0), -1.f, ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);
    }

    void imgui_end()
    {
        if(!started)
            return;

        ImGui::End();

        started = false;
    }*/
};


#endif // POPUP_HPP_INCLUDED
