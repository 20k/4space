#ifndef POPUP_HPP_INCLUDED
#define POPUP_HPP_INCLUDED

#include <string>
#include <deque>
#include <vector>
#include <map>
#include "../../render_projects/imgui/imgui.h"
#include "top_bar.hpp"
#include <vec/vec.hpp>
#include "ui_util.hpp"

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

namespace popup_colour_info
{
    static vec3f good_ui_colour{0.5, 0.5, 1};
    static vec3f bad_ui_colour{1, 0.5, 0.5};
}

namespace ImGui
{
    inline
    void GoodText(const std::string& str)
    {
        vec3f col = popup_colour_info::good_ui_colour;

        //ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());

        ImGui::OutlineHoverTextAuto(str, col);
    }

    inline
    void BadText(const std::string& str)
    {
        vec3f col = popup_colour_info::bad_ui_colour;

        //ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());

        ImGui::OutlineHoverTextAuto(str, col);
    }

    inline
    void NeutralText(const std::string& str)
    {
        vec3f col = popup_colour_info::bad_ui_colour;

        //ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());

        ImGui::OutlineHoverTextAuto(str, {1,1,1});
    }
}

struct popup_element
{
    static int gid;
    int id = gid++;

    void* element = nullptr;
    bool declaring_war = false;

    bool schedule_erase = false;
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

    void schedule_rem(void* element)
    {
        for(popup_element& elem : elements)
        {
            if(elem.element == element)
            {
                elem.schedule_erase = true;
            }
        }
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

    void rem_all_but(void* element)
    {
        for(int i=0; i<elements.size(); i++)
        {
            if(elements[i].element != element)
            {
                elements.erase(elements.begin() + i);
                i--;
                continue;
            }
        }
    }

    void remove_scheduled()
    {
        for(int i=0; i<elements.size(); i++)
        {
            if(elements[i].schedule_erase)
            {
                elements.erase(elements.begin() + i);
                i--;
                continue;
            }
        }
    }

    void clear();

    bool going = false;
};


#endif // POPUP_HPP_INCLUDED
