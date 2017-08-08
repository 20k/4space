#include "ship_customiser.hpp"

#include "top_bar.hpp"
#include <imgui/imgui.h>
#include <set>
#include "util.hpp"
#include "ui_util.hpp"
#include "popup.hpp"
#include "drag_and_drop.hpp"
#include "ship_definitions.hpp"

std::map<int, bool> component_open;

void ship_customiser::tick(float scrollwheel)
{
    current.name = "Customised";

    if(!top_bar::active[top_bar_info::SHIP_CUSTOMISER])
        return;

    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_1");

    ImGui::Begin("Ship Customisation", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    auto produced = current.get_produced_resources(1.f); ///modified by efficiency, ie real amount consumed
    auto consumed = current.get_needed_resources(1.f); ///not actually consumed, but requested
    auto stored = current.get_stored_resources();
    auto max_res = current.get_max_resources();

    std::set<ship_component_element> elements;

    for(auto& i : produced)
    {
        elements.insert(i.first);
    }

    for(auto& i : consumed)
    {
        elements.insert(i.first);
    }

    for(auto& i : stored)
    {
        elements.insert(i.first);
    }

    for(auto& i : max_res)
    {
        elements.insert(i.first);
    }

    std::vector<std::string> headers;
    std::vector<std::string> prod_list;
    std::vector<std::string> cons_list;
    std::vector<std::string> net_list;
    std::vector<std::string> store_max_list;

    if(elements.size() == 0)
    {
        ImGui::Text("Empty Ship");
        ImGui::NewLine();
    }

    for(const ship_component_element& id : elements)
    {
        float prod = produced[id];
        float cons = consumed[id];
        float store = stored[id];
        float maximum = max_res[id];

        std::string prod_str = "+" + to_string_with_enforced_variable_dp(prod, 1);
        std::string cons_str = "-" + to_string_with_enforced_variable_dp(cons, 1);

        std::string store_max_str;

        if(maximum > 0)
            store_max_str += to_string_with_variable_prec(maximum);

        std::string header_str = ship_component_elements::display_strings[id];

        std::string net_str = to_string_with_enforced_variable_dp(prod - cons, 1);

        if(prod - cons >= 0)
        {
            net_str = "+" + net_str;
        }

        if(ship_component_elements::skippable_in_display[id] != -1)
            continue;

        headers.push_back(header_str);
        prod_list.push_back(prod_str);
        cons_list.push_back(cons_str);
        net_list.push_back(net_str);
        store_max_list.push_back(store_max_str);
    }

    for(int i=0; i<headers.size(); i++)
    {
        std::string header_formatted = format(headers[i], headers);
        std::string prod_formatted = format(prod_list[i], prod_list);
        std::string cons_formatted = format(cons_list[i], cons_list);
        std::string net_formatted = format(net_list[i], net_list);
        std::string store_max_formatted = format(store_max_list[i], store_max_list);

        ImGui::Text((header_formatted + " : ").c_str());

        ImGui::SameLine(0.f, 0.f);

        ImGui::Text((net_formatted).c_str());

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip((prod_list[i] + " | " + cons_list[i]).c_str());
        }

        ImGui::SameLine(0.f, 0.f);

        ImGui::Text((" | " + store_max_formatted).c_str());
    }

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::COMPONENT && global_drag_and_drop.let_go_on_window())
    {
        component& c = *(component*)global_drag_and_drop.data;

        current.add(c);
    }

    auto win_pos = ImGui::GetWindowPos();

    ImGui::End();

    global_drag_and_drop.begin_drag_section("SIDE_FOLDOUT");

    ImGui::Begin("Ship Customisation 2", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    std::vector<std::string> names;
    std::vector<std::string> sizes;

    ///ships need ids so the ui can work

    int to_erase = -1;

    for(component& c : current.entity_list)
    {
        std::string name = c.name;

        if(c.clicked)
        {
            name = "-" + name;
        }
        else
        {
            name = "+" + name;
        }

        names.push_back(name);

        auto cstr = to_string_with_enforced_variable_dp(c.current_size, 2);

        //name = name + " " + cstr;

        sizes.push_back(cstr);
    }

    if(current.entity_list.size() == 0)
    {
        ImGui::Text("No systems");
    }

    int c_id = 0;

    for(component& c : current.entity_list)
    {
        std::string cname = format(names[c_id], names);
        std::string sname = format(sizes[c_id], sizes);

        std::string name = cname + "   " + sname;

        ImGui::TextColored({1,1,1, 1.f}, name.c_str());

        if(ImGui::IsItemClicked_Registered())
        {
            c.clicked = !c.clicked;
        }

        if(ImGui::IsItemClicked_Registered(1))
        {
            to_erase = c_id;
        }

        if(ImGui::IsItemHovered() && scrollwheel != 0)
        {
            float size_change = 0.f;

            if(scrollwheel > 0)
            {
                size_change = 0.25f;
            }
            else
            {
                size_change = -0.25f;
            }

            float csize = c.current_size;

            csize += size_change;

            csize = clamp(csize, 0.25f, 4.f);

            c.set_size(csize);

            current.repair(nullptr, 1);
        }

        if(c.clicked)
        {
            ImGui::Indent();

            std::string str = get_component_display_string(c);

            ImGui::Text(str.c_str());
            ImGui::Unindent();
        }

        c_id++;
    }

    if(to_erase != -1)
    {
        current.entity_list.erase(current.entity_list.begin() + to_erase);
    }

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::COMPONENT && global_drag_and_drop.let_go_on_window())
    {
        component& c = *(component*)global_drag_and_drop.data;

        current.add(c);
    }

    auto dim = ImGui::GetWindowSize();

    ImGui::SetWindowPos(ImVec2(win_pos.x - dim.x, win_pos.y));

    ImGui::End();

    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_2");

    ImGui::Begin("Ship Components", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    for(int i=0; i<full_component_list.size(); i++)
    {
        component& c = full_component_list[i];

        std::string pad = "-";

        if(component_open[i])
        {
            pad = "+";
        }

        ImGui::Text((pad + c.name + pad).c_str());

        if(ImGui::IsItemClicked_DragCompatible())
        {
            component_open[i] = !component_open[i];
        }

        if(ImGui::IsItemClicked_UnRegistered())
        {
            global_drag_and_drop.begin_dragging(&c, drag_and_drop_info::COMPONENT, c.name);
        }

        if(!component_open[i])
            continue;

        ImGui::Indent();

        float tech_level = c.get_tech_level_of_primary();

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

            std::string use_string = "" + to_string_with_precision(net_usage, 3) + "(/s)";
            std::string per_use_string = "" + to_string_with_precision(net_per_use, 3) + "(/use)";

            std::string fire_time_remaining = to_string_with_precision(time_between_uses, 3) + ("(s /use)");
            //std::string fire_time_remaining = "Time Left (s): " + to_string_with_enforced_variable_dp(time_until_next_use) + "/" + to_string_with_precision(time_between_uses, 3);

            std::string storage_str = "(" + to_string_with_variable_prec(max_amount) + ")";

            std::string component_str = header;

            if(net_usage != 0)
                component_str += " " + use_string;

            if(net_per_use != 0)
                component_str += " " + per_use_string;

            if(time_between_uses > 0)
                component_str += " " + fire_time_remaining;

            if(max_amount > 0)
                component_str += " " + storage_str;

            ImGui::Text(component_str.c_str());

            /*if(num != c.components.size() - 1)
            {
                component_str += "\n";
            }

            num++;*/
        }

        ImGui::Unindent();
    }

    ImGui::End();
}
