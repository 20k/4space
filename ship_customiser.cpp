#include "ship_customiser.hpp"

#include "top_bar.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
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

    if(saved.size() > 0)
    {
        last_selected = saved.front().id;
        current = saved.front();
    }
}

void display_ship(ship& current)
{
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

        if(produced[id] - consumed[id] < 0)// && ship_component_elements::element_infos[(int)id].negative_is_bad)
        {
            col = popup_colour_info::bad_ui_colour;
        }
        else
        {
            col = popup_colour_info::good_ui_colour;
        }

        ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), net_formatted.c_str());

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip((prod_list[i] + " | " + cons_list[i]).c_str());
        }

        ImGui::SameLine(0.f, 0.f);

        ImGui::Text((" | " + store_max_formatted).c_str());
    }
}

struct size_manager
{
    ImVec2 last_size = ImVec2(0,0);

    void set_size(ImVec2 s)
    {
        last_size = s;
    }

    ImVec2 get_last_size()
    {
        auto wsize = ImGui::GetWindowSize();

        ImVec2 ret = last_size;

        ret.x += ImGui::CalcTextSize(" ").x;
        ret.y += 30;

        int max_window_size = 600;

        if(ret.y >= max_window_size)
        {
            ret.y = max_window_size;
        }

        return ret;
    }
};

void child_pad()
{
    ImGui::Dummy(ImVec2(1, ImGui::CalcTextSize(" ").y / 2.f));

    ImGui::Indent(ImGui::CalcTextSize(" ").x);
}

void child_unpad()
{
    ImGui::Unindent(ImGui::CalcTextSize(" ").x);
}

#define CHILD_WINDOW_FLAGS ImGuiWindowFlags_MenuBar

void display_ship_stats_window(ship& current)
{
    static size_manager display_ship_size;

    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_1");

    //ImGui::BeginOverride((current.name + "###SHIPSTATSCUSTOMISE").c_str(), &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::BeginChild("###ship_state_customise", display_ship_size.get_last_size(), false, CHILD_WINDOW_FLAGS);

    ImGui::BeginMenuBar();
    ImGui::Text("Ship State");
    ImGui::EndMenuBar();

    ImGui::BeginGroup();

    child_pad();

    display_ship(current);

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::COMPONENT && global_drag_and_drop.let_go_on_window())
    {
        component& c = *(component*)global_drag_and_drop.data;

        current.add(c);
    }

    ImGui::NewLine();

    auto win_pos = ImGui::GetWindowPos();
    auto win_size = ImGui::GetWindowSize();

    child_unpad();

    ImGui::EndGroup();

    display_ship_size.set_size(ImGui::GetItemRectSize());

    ImGui::EndChild();
}

void do_side_foldout_window(ship& current, float scrollwheel)
{
    static size_manager foldout_size;

    global_drag_and_drop.begin_drag_section("SIDE_FOLDOUT");

    ImGui::BeginChild("###side_foldout", foldout_size.get_last_size(), false, CHILD_WINDOW_FLAGS);

    ImGui::BeginMenuBar();
    ImGui::Text("Ship Components");
    ImGui::EndMenuBar();

    ImGui::BeginGroup();

    child_pad();

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

            rm.render_tooltip(true);
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

    rm.render_formatted_str(true);

    if(to_erase != -1)
    {
        current.entity_list.erase(current.entity_list.begin() + to_erase);
    }

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::COMPONENT && global_drag_and_drop.let_go_on_window())
    {
        component& c = *(component*)global_drag_and_drop.data;

        current.add(c);
    }

    child_unpad();

    ImGui::EndGroup();

    foldout_size.set_size(ImGui::GetItemRectSize());

    ImGui::EndChild();
}

int get_max_spacing_len(ship& current)
{
    int max_len = 0;

    for(auto& c : full_component_list)
    {
        max_len = std::max(max_len, (int)c.name.length());
    }

    max_len = std::max(max_len, (int)current.name.length());

    int min_title_length = 4;

    if(max_len < min_title_length)
    {
        max_len = min_title_length;
    }

    max_len += 8;

    return max_len;
}

float do_consistent_pad(ship_customiser& ship_customise)
{
    int max_len = 0;

    for(ship& s : ship_customise.saved)
    {
        max_len = std::max(max_len, get_max_spacing_len(s));
    }

    max_len = std::max(max_len, get_max_spacing_len(ship_customise.current));

    max_len += 4;

    std::string length_test(max_len, ' ');

    return ImGui::CalcTextSize(length_test.c_str()).x;
}

#define HIGHLIGHT_COL (vec3f){ImGui::GetStyleCol(ImGuiCol_TitleBgActive).x, ImGui::GetStyleCol(ImGuiCol_TitleBgActive).y, ImGui::GetStyleCol(ImGuiCol_TitleBgActive).z}

void do_ship_component_display(ship& current, ship_customiser& ship_customise)
{
    static size_manager display_component_size;

    global_drag_and_drop.begin_drag_section("SHIP_CUSTOMISE_2");

    float dim = do_consistent_pad(ship_customise);

    ImGui::BeginChild("###ship_components", ImVec2(dim, display_component_size.get_last_size().y), false, CHILD_WINDOW_FLAGS);

    ImGui::BeginMenuBar();
    ImGui::Text("Available Components");
    ImGui::EndMenuBar();

    ImGui::BeginGroup();

    child_pad();

    std::stable_sort(full_component_list.begin(), full_component_list.end(),
                     [](auto& c1, auto& c2){return c1.ui_category < c2.ui_category;});

    for(int category = 0; category < component_category_info::NONE; category++)
    {
        std::string my_name = component_category_info::names[category];

        bool is_active = component_category_info::is_active[category];

        if(is_active)
        {
            my_name = "-" + my_name;
        }
        else
        {
            my_name = "+" + my_name;
        }

        ImGui::SolidSmallButton(my_name, HIGHLIGHT_COL, {1,1,1}, false, {0,0});

        if(ImGui::IsItemClicked_Registered())
        {
            component_category_info::is_active[category] = !component_category_info::is_active[category];
        }

        if(!component_category_info::is_active[category])
            continue;

        ImGui::Indent();

        for(int i=0; i<full_component_list.size(); i++)
        {
            component& c = full_component_list[i];

            if((int)c.ui_category != category)
                continue;

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

        ImGui::Unindent();
    }

    child_unpad();

    ImGui::EndGroup();

    display_component_size.set_size(ImGui::GetItemRectSize());

    ImGui::EndChild();
}

void handle_top_bar(ship& current)
{
    int max_name_length = 40;

    std::string dummy(40, ' ');

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4,2));
    ImGui::AlignFirstTextHeightToWidgets();

    ImGui::Text("Name: ");

    ImGui::SameLine();

    current.name.resize(max_name_length + 1);
    ImGui::PushItemWidth(ImGui::CalcTextSize(dummy.c_str()).x);
    ImGui::InputText("##test", &current.name[0], max_name_length);
    ImGui::PopItemWidth();

    current.name.resize(strlen(current.name.c_str()));

    if(ImGui::IsItemActive())
    {
        ImGui::suppress_keyboard = true;
    }

    ///SCALE

    ImGui::AlignFirstTextHeightToWidgets();

    ImGui::Text("Scale:");

    ImGui::SameLine();

    ImGui::PushItemWidth(150.f);
    ImGui::DragFloat("###SIZE_FLOATER_SHIP_CUSTOMISE", &current.editor_size_storage, 0.1f, 0.1f, 100.f, "%.1f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar(1);
}

void do_selection_bar(ship_customiser& ship_customise)
{
    ImGui::SolidSmallButton("Design List", HIGHLIGHT_COL, {1,1,1}, ship_customise.edit_state == 0, {100, 0});

    if(ImGui::IsItemClicked_Registered())
    {
        ship_customise.edit_state = 0;
    }

    ImGui::SameLine();

    ImGui::SolidSmallButton("Edit Design", HIGHLIGHT_COL, {1,1,1}, ship_customise.edit_state == 1, {100, 0});

    if(ImGui::IsItemClicked_Registered())
    {
        ship_customise.edit_state = 1;
    }
}

///Ok. If I'm going to stick with 1d ship layout, need to colour code the components by category
///otherwise it is simply too difficult to see wtfs going on
///draw up colour list, not hard to do
void ship_customiser::tick(float scrollwheel, bool lclick, vec2f mouse_change)
{
    text_input_going = false;

    if(current.name == "")
        current.name = "New Ship";

    current.refill_all_components();

    if(!top_bar::active[top_bar_info::SHIP_CUSTOMISER])
        return;

    current.sort_components();

    for(auto& i : saved)
    {
        i.sort_components();
    }

    if(last_selected == -1 && saved.size() != 0)
    {
        last_selected = saved.front().id;
    }

    ImGui::BeginOverride("Ship Customise", &top_bar::active[top_bar_info::SHIP_CUSTOMISER], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::clamp_window_to_screen();

    ImGui::BeginGroup();

    do_selection_bar(*this);
    ImGui::NewLine();
    handle_top_bar(current);
    ImGui::NewLine();


    if(edit_state == 1)
        do_ship_component_display(current, *this);
    if(edit_state == 0)
        do_save_window();

    ImGui::SameLine();

    do_side_foldout_window(current, scrollwheel);

    ImGui::SameLine();

    display_ship_stats_window(current);

    ImGui::EndGroup();

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
        last_selected = current.id;
        saved.push_back(current);
    }
}

void ship_customiser::do_save_window()
{
    static size_manager size_manage;

    float dim = do_consistent_pad(*this);

    ImGui::BeginChild("###Ship Designs", ImVec2(dim, size_manage.get_last_size().y), false, CHILD_WINDOW_FLAGS);

    ImGui::BeginMenuBar();
    ImGui::Text("Ship Designs");
    ImGui::EndMenuBar();

    ImGui::BeginGroup();

    child_pad();

    for(int i=0; i<saved.size(); i++)
    {
        ship& s = saved[i];

        std::string name = s.name;

        vec3f col = popup_colour_info::neutral_ui_colour;

        if(!s.is_ship_design_valid())
        {
            col = popup_colour_info::bad_ui_colour;
        }

        ImGui::SolidSmallButton(name, HIGHLIGHT_COL, col, s.id == last_selected);

        if(ImGui::IsItemClicked() && last_selected != s.id)
        {
            current = s;
            last_selected = current.id;
        }
    }

    if(last_selected != -1)
    {
        ImGui::NewLine();

        ImGui::NeutralText("(Delete current)");

        if(ImGui::IsItemClicked())
        {
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
            auto dupe = current.duplicate();

            dupe.name = dupe.name + " (1)";

            ///this is wrong a s it duplicates ids
            if(saved.size() == 0)
            {
                saved.push_back(dupe);
            }
            else
            {
                for(auto it = saved.begin(); it != saved.end(); it++)
                {
                    if(it->id == last_selected)
                    {
                        saved.insert(it + 1, dupe);
                        break;
                    }
                }
            }

            current = dupe;
            last_selected = dupe.id;
        }

        /*ImGui::NeutralText("(Save)");

        if(ImGui::IsItemClicked())
        {
            save();
        }*/
    }

    ImGui::NeutralText("(New Design)");

    if(ImGui::IsItemClicked())
    {
        current = ship();

        current.name = "New Design";
        saved.push_back(current);
        last_selected = current.id;
    }

    child_unpad();

    ImGui::EndGroup();

    size_manage.set_size(ImGui::GetItemRectSize());

    ImGui::EndChild();
}
