#ifndef SHIP_CUSTOMISER_HPP_INCLUDED
#define SHIP_CUSTOMISER_HPP_INCLUDED

#include "ship.hpp"
#include "ui_util.hpp"
#include "util.hpp"
#include <map>
#include "ship_definitions.hpp"

extern std::map<int, bool> component_open;

///so display this (ish) on mouseover for a component
inline
std::string get_component_display_string(component& c)
{
    std::string component_str = "";//c.name + ":\n";

    int num = 0;


    float tech_level = c.get_tech_level_of_primary();

    std::string tech_str = "Tech Level: " + std::to_string((int)tech_level) + "\n";

    component_str += tech_str;


    std::string eff = "";

    float efficiency = 1.f;

    if(c.components.begin() != c.components.end())
        efficiency = c.components.begin()->second.cur_efficiency * 100.f;

    if(efficiency < 0.001f)
        efficiency = 0.f;

    if(efficiency < 99.9f)
        eff = "Efficiency %%: " + to_string_with_enforced_variable_dp(efficiency) + "\n";

    component_str += eff;

    for(auto& i : c.components)
    {
        auto type = i.first;
        component_attribute attr = i.second;

        std::string header = ship_component_elements::display_strings[type];

        ///ok, we can take the nets

        float net_usage = attr.produced_per_s - attr.drained_per_s;
        float net_per_use = attr.produced_per_use - attr.drained_per_use;

        float time_between_uses = attr.time_between_uses_s;
        float time_until_next_use = std::max(0.f, attr.time_between_uses_s - (attr.current_time_s - attr.time_last_used_s));

        float max_amount = attr.max_amount;
        float cur_amount = attr.cur_amount;

        //float tech_level = attr.tech_level;

        std::string use_string = "" + to_string_with_precision(net_usage, 3) + "(/s)";
        std::string per_use_string = "" + to_string_with_precision(net_per_use, 3) + "(/use)";

        //std::string time_str = "Time Between Uses (s): " + to_string_with_precision(time_between_uses, 3);
        //std::string left_str = "Time Till Next Use (s): " + to_string_with_precision(time_until_next_use, 3);

        std::string fire_time_remaining = "Time Left (s): " + to_string_with_enforced_variable_dp(time_until_next_use) + "/" + to_string_with_precision(time_between_uses, 3);

        //std::string mamount_str = "Max Storage: " + to_string_with_precision(max_amount, 3);
        //std::string camount_str = "Current Storage: " + to_string_with_precision(cur_amount, 3);

        std::string storage_str = "(" + to_string_with_enforced_variable_dp(cur_amount) + "/" + to_string_with_variable_prec(max_amount) + ")";

        //std::string tech_str = "(Tech " + std::to_string((int)tech_level) + ")";

        //std::string efficiency_str = "Efficiency %%: " + to_string_with_precision(attr.cur_efficiency*100.f, 3);

        component_str += header;

        if(net_usage != 0)
            component_str += " " + use_string;

        if(net_per_use != 0)
            component_str += " " + per_use_string;

        if(time_between_uses > 0)
            component_str += " " + fire_time_remaining;

        /*if(max_amount > 0)
            component_str += "\n" + mamount_str;*/

        ///not a typo
        /*if(max_amount > 0)
            component_str += "\n" + camount_str;*/

        if(max_amount > 0)
            component_str += " " + storage_str;

        //component_str += " " + tech_str;

        //if(attr.cur_efficiency < 0.99999f)
        //    component_str += "\n" + efficiency_str;


        if(num != c.components.size() - 1)
        {
            component_str += "\n";
        }

        num++;
    }

    return component_str;
}

///vertical list of attributes, going horizontally with separators between components
///we'll also want a systems level version of this, ie overall not specific
inline
std::vector<std::string> get_components_display_string(ship& s)
{
    std::vector<component> components = s.entity_list;

    std::vector<std::string> display_str;

    for(component& c : components)
    {
        std::string component_str = get_component_display_string(c);

        display_str.push_back(component_str);
    }

    return display_str;
}

struct ship_customiser
{
    bool text_input_going = false;

    std::vector<ship> saved;

    ship current;
    int64_t last_selected = -1;

    ship_customiser();

    void tick(float scrollwheel, bool lclick, vec2f mouse_change);

private:
    //ImVec2 last_stats_dim = ImVec2(0, 0);
    void save();
    void do_save_window();
    int64_t renaming_id = -1;
    std::string ship_name_buffer;
};

#endif // SHIP_CUSTOMISER_HPP_INCLUDED
