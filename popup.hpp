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

struct popup_element
{
    static int gid;
    int id = gid++;

    void* element = nullptr;
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
};


#endif // POPUP_HPP_INCLUDED
