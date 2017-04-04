#include "resource_manager.hpp"
#include "../../render_projects/imgui/imgui.h"

#include "util.hpp"
#include <vec/vec.hpp>

resource::types resource::get_random_unprocessed()
{
    return (resource::types)randf_s(0.f, resource::unprocessed_end);
}

bool resource::is_processed(resource::types type)
{
    return type >= resource::unprocessed_end;
}

resource_manager::resource_manager()
{
    resources.resize(resource::types::COUNT);

    for(int i=0; i<resources.size(); i++)
    {
        resources[i].type = (resource::types)i;
    }
}

resource_element& resource_manager::get_resource(resource::types type)
{
    for(auto& i : resources)
    {
        if(i.type == type)
            return i;
    }

    throw;
}

void resource_manager::draw_ui(sf::RenderWindow& win)
{
    ImGui::Begin("Resources");

    std::vector<std::string> names_up;
    std::vector<std::string> vals_up;

    std::vector<std::string> names_p;
    std::vector<std::string> vals_p;

    for(resource_element& elem : resources)
    {
        std::string name = resource::short_names[elem.type];

        std::string val = "(" + to_string_with_variable_prec(elem.amount) + ")";

        if(resource::is_processed(elem.type))
        {
            names_up.push_back("");
            vals_up.push_back("");

            names_p.push_back(name);
            vals_p.push_back(val);
        }
        else
        {
            names_p.push_back("");
            vals_p.push_back("");

            names_up.push_back(name);
            vals_up.push_back(val);
        }
    }

    ImGui::BeginGroup();

    for(int i=0; i<resources.size(); i++)
    {
        resource_element& elem = resources[i];

        std::string name;
        std::string val;

        if(resource::is_processed(elem.type))
        {
            name = "|  " + format(names_p[i], names_p);
            val = format(vals_p[i], vals_p);
        }
        else
        {
            name = format(names_up[i], names_up);
            val = format(vals_up[i], vals_up);
        }

        ImGui::Text(name.c_str());
        ImGui::SameLine();
        ImGui::Text(val.c_str());

        if(!resource::is_processed(elem.type) && resource::is_processed((resource::types)((int)elem.type + 1)))
        {
            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            ImGui::BeginGroup();
        }
    }

    ImGui::EndGroup();

    ImGui::End();
}

std::string resource_manager::get_unprocessed_str()
{
    std::vector<std::string> names;
    std::vector<std::string> vals;

    for(resource_element& elem : resources)
    {
        std::string name = resource::short_names[elem.type];

        std::string val = "(" + to_string_with_variable_prec(elem.amount) + ")";

        if(fabs(elem.amount) <= 0.001f)
            continue;

        names.push_back(name);
        vals.push_back(val);
    }

    names.resize(resource::unprocessed_end);
    vals.resize(resource::unprocessed_end);

    std::string ret;

    for(int i=0; i<names.size(); i++)
    {
        ret = ret + names[i] + " " + vals[i] + "\n";
    }

    return ret;
}
