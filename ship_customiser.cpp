#include "ship_customiser.hpp"

#include "top_bar.hpp"
#include <imgui/imgui.h>
#include <set>
#include "util.hpp"
#include "ui_util.hpp"
#include "popup.hpp"
#include "drag_and_drop.hpp"
#include "ship_definitions.hpp"
#include "tooltip_handler.hpp"

std::map<int, bool> component_open;

ship_customiser::ship_customiser()
{
    //last_selected = current.id;

    saved = default_ships_list;
}

void ship_customiser::tick(float scrollwheel, bool lclick, vec2f mouse_change)
{
    text_input_going = false;

    if(lclick)
    {
        renaming_id = -1;
    }

    if(current.name == "")
        current.name = "New Ship";

    current.refill_all_components();

    if(!top_bar::active[top_bar_info::SHIP_CUSTOMISER])
        return;

    do_save_window();

    if(last_selected == -1)
        return;


    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_1");

    ImGui::BeginOverride((current.name + "###SHIPSTATSCUSTOMISE").c_str(), &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

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
    std::vector<ship_component_element> found_elements;

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

        if(prod - cons == 0.f && maximum == 0.f && ship_component_elements::element_infos[(int)id].resource_type != resource::COUNT)
            continue;

        headers.push_back(header_str);
        prod_list.push_back(prod_str);
        cons_list.push_back(cons_str);
        net_list.push_back(net_str);
        store_max_list.push_back(store_max_str);
        found_elements.push_back(id);
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

        vec3f col = {1,1,1};

        ship_component_element& id = found_elements[i];

        if(produced[id] - consumed[id] <= 0 && ship_component_elements::element_infos[(int)id].negative_is_bad)
        {
            col = popup_colour_info::bad_ui_colour;
        }

        ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), net_formatted.c_str());

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

    ImGui::NewLine();

    ImGui::Text("Size");

    ImGui::SameLine();

    ImGui::PushItemWidth(150.f);
    ImGui::DragFloat("###SIZE_FLOATER_SHIP_CUSTOMISE", &current.editor_size_storage, 0.1f, 0.1f, 100.f, "%.1f");
    ImGui::PopItemWidth();

    auto win_pos = ImGui::GetWindowPos();
    auto win_size = ImGui::GetWindowSize();

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(win_pos.x + win_size.x, win_pos.y + get_title_bar_height()));

    global_drag_and_drop.begin_drag_section("SIDE_FOLDOUT");

    ImGui::BeginOverride((current.name + "###SHIPPITYSHIPSHAPE").c_str(), &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_JUST_TEXT_WINDOW_INPUTS);
    //ImGui::BeginOverride((current.name + "###SHIPPITYSHIPSHAPE").c_str(), &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

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

        vec3f col = {1,1,1};

        bool component_cant_be_used = false;

        if(c.test_if_can_use_in_ship_customisation)
        {
            if(!current.can_use(c))
            {
                col = popup_colour_info::bad_ui_colour;

                component_cant_be_used = true;
            }
        }

        ImGui::TextColored(name, col);

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

        if(ImGui::IsItemHovered())
        {
            auto res = c.resources_cost();

            resource_manager rm;
            rm.add(res);

            tooltip::add(rm.get_formatted_str(true));
        }

        if(ImGui::IsItemHovered() && component_cant_be_used)
        {
            tooltip::add("Cannot be used due to insufficient:");

            auto fully_merged = current.get_fully_merged(1.f);

            //for(auto& item : c.components)
            for(int type = 0; type < c.components.size(); type++)
            {
                component_attribute& attr = c.components[type];

                if(!attr.present)
                    continue;

                float diff = attr.produced_per_use - attr.drained_per_use;

                if(-diff > fully_merged[type].max_amount)
                {
                    tooltip::add(ship_component_elements::display_strings[(int)type]);
                }
            }
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

    vec3f space_colour = popup_colour_info::good_ui_colour;

    if(!current.is_ship_design_valid())
    {
        space_colour = popup_colour_info::bad_ui_colour;
    }

    ImGui::ColourNoHoverText("Space: " + to_string_with_enforced_variable_dp(current.get_total_components_size()) + "/" + to_string_with_enforced_variable_dp(ship_component_elements::max_components_total_size), space_colour);

    auto res = current.resources_cost();

    resource_manager rm;
    rm.add(res);

    ImGui::Text(rm.get_formatted_str(true).c_str());

    if(to_erase != -1)
    {
        current.entity_list.erase(current.entity_list.begin() + to_erase);
    }

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::COMPONENT && global_drag_and_drop.let_go_on_window())
    {
        component& c = *(component*)global_drag_and_drop.data;

        current.add(c);
    }

    ImGui::End();

    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_2");

    ImGui::BeginOverride("Ship Components", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    for(int i=0; i<full_component_list.size(); i++)
    {
        component& c = full_component_list[i];

        std::string pad = "+";

        if(component_open[i])
        {
            pad = "-";
        }

        ImGui::Text((pad + c.name).c_str());

        if(ImGui::IsItemClicked_DragCompatible())
        {
            component_open[i] = !component_open[i];
        }

        if(ImGui::IsItemClicked_UnRegistered())
        {
            global_drag_and_drop.begin_dragging(&c, drag_and_drop_info::COMPONENT, c.name);
        }

        if(ImGui::IsItemClicked_Registered(1))
        {
            current.add(c);
        }

        if(!component_open[i])
            continue;

        ImGui::Indent();

        float tech_level = c.get_tech_level_of_primary();

        for(int type = 0; type < c.components.size(); type++)
        {
            const component_attribute& attr = c.components[type];

            if(!attr.present)
                continue;

            std::string component_str = get_component_attribute_display_string(c, type);

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

    current.editor_size_storage = clamp(current.editor_size_storage, 0.1f, 1000.f);

    if(current.editor_size_storage != current.current_size)
    {
        current.set_size(current.editor_size_storage);
    }

    save();
}

void ship_customiser::save()
{
    bool found = false;

    for(ship& s : saved)
    {
        if(s.id == last_selected)
        {
            found = true;
            s = current;
            break;
        }
    }

    if(!found)
    {
        saved.push_back(current);
    }
}

void ship_customiser::do_save_window()
{
    ImGui::BeginOverride("Ship Design Manager", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    for(int i=0; i<saved.size(); i++)
    {
        ship& s = saved[i];

        std::string name = s.name;

        if(renaming_id != s.id)
        {
            if(s.is_ship_design_valid())
                ImGui::NeutralText(name);
            else
                ImGui::BadText(name);

            if(ImGui::IsItemClicked() && last_selected != s.id)
            {
                current = s;
                last_selected = current.id;
            }

            if(ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
            {
                ship_name_buffer = s.name;
                renaming_id = last_selected;
            }
        }
        else
        {
            ship_name_buffer.resize(100);

            text_input_going = true;

            if(ImGui::InputText("###Input_savewindow", &ship_name_buffer[0], ship_name_buffer.size() - 1, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                renaming_id = -1;
            }

            ///we don't want nulls
            int c_style_length = strlen(ship_name_buffer.c_str());

            s.name = ship_name_buffer;

            s.name.resize(c_style_length);

            if(current.id == s.id)
            {
                current.name = s.name;
            }
        }
    }

    if(last_selected != -1)
    {
        ImGui::NeutralText("(Delete current design)");

        if(ImGui::IsItemClicked())
        {
            //current = ship();

            int i = 0;

            for(i=0; i<saved.size(); i++)
            {
                if(last_selected == saved[i].id)
                {
                    saved.erase(saved.begin() + i);
                    i--;
                    break;
                }
            }

            if(i >= 0 && i < saved.size())
            {
                current = saved[i];
            }
            else
            {
                current = ship();
            }

            last_selected = current.id;
        }

        ImGui::NeutralText("(Duplicate)");

        if(ImGui::IsItemClicked())
        {
            ///this is wrong a s it duplicates ids
            saved.push_back(current.duplicate());

            //current = ship();
            last_selected = current.id;
        }

        ImGui::NeutralText("(Save)");

        if(ImGui::IsItemClicked())
        {
            save();
        }
    }

    ImGui::NeutralText("(New Design)");

    if(ImGui::IsItemClicked())
    {
        current = ship();

        current.name = "New Design";
        saved.push_back(current);
        last_selected = current.id;
    }

    ImGui::End();
}
