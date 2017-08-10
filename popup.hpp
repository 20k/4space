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
    struct ScopedIndent
    {
        ScopedIndent()
        {
            ImGui::Indent();
        }

        ~ScopedIndent()
        {
            ImGui::Unindent();
        };
    };

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
    void NeutralText(const std::string& str, vec2f dim_extra = {0,0})
    {
        //ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());

        ImGui::OutlineHoverTextAuto(str, {1,1,1}, true, dim_extra);
    }

    inline
    void ColourHoverText(const std::string& str, vec3f col)
    {
        ImGui::OutlineHoverTextAuto(str, col);
    }

    inline
    void ColourNoHoverText(const std::string& str, vec3f col)
    {
        ImGui::OutlineHoverTextAuto(str, col, false);
    }

    inline
    void GoodTextNoHoverEffect(const std::string& str)
    {
        vec3f col = popup_colour_info::good_ui_colour;

        ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
    }

    inline
    void BadTextNoHoverEffect(const std::string& str)
    {
        vec3f col = popup_colour_info::bad_ui_colour;

        ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), str.c_str());
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

    void schedule_rem_all_but(void* element)
    {
        for(popup_element& elem : elements)
        {
            if(elem.element != element)
            {
                elem.schedule_erase = true;
            }
        }
    }

    void cleanup(void* element);

    void rem(void* element)
    {
        for(int i=0; i<elements.size(); i++)
        {
            if(elements[i].element == element)
            {
                cleanup(element);

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
                cleanup(elements[i].element);

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
                cleanup(elements[i].element);

                elements.erase(elements.begin() + i);
                i--;
                continue;
            }
        }
    }

    void insert(void* data)
    {
        for(popup_element& elem : elements)
        {
            if(elem.element == data)
                return;
        }

        popup_element e;
        e.element = data;

        elements.push_back(e);
    }

    void clear();

    bool going = false;
};


#endif // POPUP_HPP_INCLUDED
