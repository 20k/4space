#include "resource_manager.hpp"
#include "../../render_projects/imgui/imgui.h"

#include "util.hpp"
#include <vec/vec.hpp>
#include "top_bar.hpp"

/*resource::types resource::get_random_unprocessed()
{
    return (resource::types)randf_s(0.f, resource::unprocessed_end);
}*/

resource::types resource::get_random_processed()
{
    return (resource::types)randf_s(resource::unprocessed_end, resource::COUNT);
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

void resource_manager::add(const std::map<resource::types, float>& val)
{
    for(auto& i : val)
    {
        resources[i.first].amount += i.second;
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
    if(!top_bar::get_active(top_bar_info::ECONOMY))
        return;

    ImGui::Begin("Resources", &top_bar::active[top_bar_info::ECONOMY], IMGUI_WINDOW_FLAGS);

    /*std::vector<std::string> names_up;
    std::vector<std::string> vals_up;

    std::vector<std::string> names_p;
    std::vector<std::string> vals_p;

    for(resource_element& elem : resources)
    {
        std::string name = resource::short_names[elem.type];

        std::string val = "(" + to_string_with_enforced_variable_dp(elem.amount) + ")";

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

    ImGui::EndGroup();*/

    auto str = get_formatted_str(false);

    ImGui::Text(str.c_str());

    ImGui::End();
}

/*std::string resource_manager::get_unprocessed_str()
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
        ret = ret + format(names[i], names) + " " + format(vals[i], vals) + "\n";
    }

    return ret;
}*/

std::string resource_manager::get_processed_str(bool can_skip)
{
    std::vector<std::string> names;
    std::vector<std::string> vals;

    for(resource_element& elem : resources)
    {
        std::string name = resource::short_names[elem.type];

        std::string val = "(" + to_string_with_enforced_variable_dp(elem.amount) + ")";

        if(can_skip && fabs(elem.amount) <= 0.001f)
            continue;

        names.push_back(name);
        vals.push_back(val);
    }

    //names.resize(resource::unprocessed_end);
    //vals.resize(resource::unprocessed_end);

    for(int i=0; i<resource::unprocessed_end; i++)
    {
        names.erase(names.begin());
        vals.erase(vals.begin());
    }

    std::string ret;

    for(int i=0; i<names.size(); i++)
    {
        ret = ret + format(names[i], names) + " " + format(vals[i], vals) + "\n";
    }

    return ret;
}

std::string resource_manager::get_formatted_str(bool can_skip)
{
    return get_processed_str(can_skip);

    #if 0

    std::vector<std::string> names_up;
    std::vector<std::string> vals_up;

    std::vector<std::string> names_p;
    std::vector<std::string> vals_p;

    bool any_processed = has_any_processed();

    bool skip_low = !any_processed;

    for(resource_element& elem : resources)
    {
        std::string name = resource::short_names[elem.type];

        std::string val = "(" + to_string_with_enforced_variable_dp(elem.amount) + ")";

        if(skip_low)
        {
            if(fabs(elem.amount) < 0.001f)
                continue;
        }

        if(resource::is_processed(elem.type))
        {
            names_p.push_back(name);
            vals_p.push_back(val);
        }
        else
        {
            names_up.push_back(name);
            vals_up.push_back(val);
        }
    }


    std::string ret;

    if(any_processed)
    {
        for(int i=0; i<std::max(names_up.size(), names_p.size()); i++)
        {
            if(i < names_up.size())
                ret += format(names_up[i], names_up) + " " + format(vals_up[i], vals_up);
            else
                ret += format("", names_up); + " " + format("", vals_up);

            if(i < names_p.size())
                ret += "  |  " + format(names_p[i], names_p) + " " + format(vals_p[i], vals_p);
            else
                ret += format("", names_p); + " " + format("", vals_p);

            ret += "\n";
        }
    }
    else
    {
        for(int i=0; i<names_up.size(); i++)
        {
            //if(fabs(resources[i].amount) < 0.001f)
            //    continue;

            ret += format(names_up[i], names_up) + " " + format(vals_up[i], vals_up);

            ret += "\n";
        }
    }

    return ret;
    #endif
}

bool resource_manager::has_any_processed()
{
    for(auto& i : resources)
    {
        if(!resource::is_processed(i.type))
            continue;

        if(fabs(i.amount) > 0.0001f)
            return true;
    }

    return false;
}
