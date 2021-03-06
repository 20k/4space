#include <iostream>
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <iomanip>
#include "battle_manager.hpp"
#include <set>
#include <deque>
#include "system_manager.hpp"

#include "util.hpp"

#include "empire.hpp"
#include "research.hpp"
#include "event_system.hpp"
//#include "music.hpp"
#include "tooltip_handler.hpp"
#include "top_bar.hpp"

#include "ship_definitions.hpp"

#include "popup.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "drag_and_drop.hpp"
#include "ui_util.hpp"
#include "context_menu.hpp"
#include "profile.hpp"
#include "ship_customiser.hpp"
#include "imgui_customisation.hpp"
#include "stacktrace_helper.hpp"
#include "serialise.hpp"
#include "networking.hpp"
#include "network_update_strategies.hpp"
#include "clickable.hpp"


/*std::string obfuscate(const std::string& str, bool should_obfuscate)
{
    if(!should_obfuscate)
        return str;

    return "??/??";
}*/


void do_popout(ship& s, float known_information, empire* player_empire)
{
    if(s.display_popout)
    {
        ///ships need ids so the ui can work
        int c_id = 0;

        for(component& c : s.entity_list)
        {
            float hp = 1.f;

            if(c.has_element(ship_component_element::HP))
                hp = c.get_stored()[ship_component_element::HP] / c.get_stored_max()[ship_component_element::HP];

            vec3f ccol = hp_frac_to_col(hp);

            std::string name = c.name;

            if(c.clicked)
            {
                name = "-" + name;
            }
            else
            {
                name = "+" + name;
            }

            bool knows = known_information >= c.scanning_difficulty;

            if(s.owned_by->parent_empire->is_allied(player_empire))
            {
                knows = true;
            }

            name = obfuscate(name, !knows);

            ImGui::TextColored({ccol.x(), ccol.y(), ccol.z(), 1.f}, name.c_str());

            if(ImGui::IsItemClicked_Registered())
            {
                c.clicked = !c.clicked;
            }

            if(c.clicked)
            {
                ImGui::Indent();

                std::string str = get_component_display_string(c);

                ImGui::Text(obfuscate(str, !knows).c_str());
                ImGui::Unindent();
            }

            c_id++;
        }
    }
}

void do_title_colouring_preparation(ship& s, empire* player_empire, float info_available)
{
    auto default_bg_col = ImGui::GetStyleCol(ImGuiCol_TitleBg);
    vec3f vdefault = xyz_to_vec(default_bg_col);

    ///this will make a perceptual colour scientist cry
    float intensity_approx = vdefault.length();

    vec3f vbg_col;

    if(info_available > ship_info::accessory_information_obfuscation_level)
        vbg_col = player_empire->get_relations_colour(s.owned_by->parent_empire, true);
    else
        vbg_col = player_empire->get_relations_colour(nullptr, true);

    vbg_col = vbg_col * intensity_approx / vbg_col.length();

    vbg_col = mix(vbg_col, vdefault, 0.1f);

    ImVec4 bg_col = ImVec4(vbg_col.x(), vbg_col.y(), vbg_col.z(), default_bg_col.w);

    ImGui::PushStyleColor(ImGuiCol_TitleBg, bg_col);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, bg_col);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, bg_col);

    vec4f close_col = xyzw_to_vec(ImGui::GetStyleCol(ImGuiCol_CloseButton));
    vec4f close_col_active = xyzw_to_vec(ImGui::GetStyleCol(ImGuiCol_CloseButtonActive));
    vec4f close_col_hovered = xyzw_to_vec(ImGui::GetStyleCol(ImGuiCol_CloseButtonHovered));

    float i1 = close_col.xyz().length();
    float i2 = close_col_active.xyz().length();
    float i3 = close_col_hovered.xyz().length();

    vec3f s1 = i1 * vbg_col.norm();
    vec3f s2 = i2 * vbg_col.norm();
    vec3f s3 = i3 * vbg_col.norm();

    ImGui::PushStyleColor(ImGuiCol_CloseButton, ImVec4(s1.x(), s1.y(), s1.z(), close_col.w()));
    ImGui::PushStyleColor(ImGuiCol_CloseButtonActive, ImVec4(s2.x(), s2.y(), s2.z(), close_col_active.w()));
    ImGui::PushStyleColor(ImGuiCol_CloseButtonHovered, ImVec4(s3.x(), s3.y(), s3.z(), close_col_hovered.w()));
}

///claiming_empire for salvage, can be nullptr
void display_ship_info(ship& s, empire* owner, empire* claiming_empire, empire* player_empire, system_manager& system_manage, fleet_manager& fleet_manage, empire_manager& empire_manage, popup_info& popup, orbital* associated)
{
    sf::Keyboard key;

    auto produced = s.get_produced_resources(1.f); ///modified by efficiency, ie real amount consumed
    auto consumed = s.get_needed_resources(1.f); ///not actually consumed, but requested
    auto stored = s.get_stored_resources();
    auto max_res = s.get_max_resources();

    auto fully_merged = s.get_fully_merged(1.f);

    float known_information = player_empire->available_scanning_power_on(associated);

    //bool knows_prod = known_information >= 0.4f;
    //bool knows_prod_exact = known_information >= 0.9f;

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

    if(player_empire->is_allied(owner))
    {
        known_information = 1.f;
    }

    std::string name_str = obfuscate(s.name, known_information < ship_info::ship_obfuscation_level);

    if(s.in_combat())
    {
        name_str += " (In Combat)";
    }

    if(s.fully_disabled())
    {
        name_str += " (Derelict)";
    }

    std::map<ship_component_elements::types, bool> primary_obfuscated;

    for(component& c : s.entity_list)
    {
        if(known_information < c.scanning_difficulty)
            primary_obfuscated[c.primary_attribute] = true;
    }

    do_title_colouring_preparation(s, player_empire, known_information);

    ImGui::BeginOverride((name_str + "###" + s.name + std::to_string(s.id)).c_str(), &s.display_ui, ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    ImGui::clamp_window_to_screen();

    ImGuiWindow* win = ImGui::GetCurrentWindow();

    ImRect rect = win->TitleBarRect();

    if(ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(win->TitleBarRect().Min, win->TitleBarRect().Max, false) && ImGui::IsMouseClicked(1))
    {
        s.context_request_open = true;
    }

    std::vector<std::string> headers;
    std::vector<std::string> prod_list;
    std::vector<std::string> cons_list;
    std::vector<std::string> net_list;
    std::vector<std::string> store_max_list;
    std::vector<ship_component_element> found_elements;

    for(const ship_component_element& id : elements)
    {
        float prod = fully_merged[id].produced_per_s;
        float cons = fully_merged[id].drained_per_s;
        float store = fully_merged[id].cur_amount;
        float maximum = fully_merged[id].max_amount;

        std::string prod_str = "+" + to_string_with_enforced_variable_dp(prod, 1);
        std::string cons_str = "-" + to_string_with_enforced_variable_dp(cons, 1);

        std::string store_max_str;

        if(maximum > 0)
            store_max_str += to_string_with_enforced_variable_dp(store) + "/" + to_string_with_variable_prec(maximum);

        std::string header_str = ship_component_elements::display_strings[id];

        std::string net_str = to_string_with_enforced_variable_dp(prod - cons, 1);

        if(prod - cons >= 0)
        {
            net_str = "+" + net_str;
        }

        //std::string res = header + ": " + display_str + "\n";

        //bool knows = s.is_known_with_scanning_power(id, known_information);

        bool obfuscd = primary_obfuscated[id];

        bool is_resource = ship_component_elements::element_infos[id].resource_type != resource::COUNT;

        ///none of these systems have a component with them as a primary
        ///weird and hardcoded. Armour will when ships carry armour
        ///ammo too, hp and fuel only real special case
        if(id == ship_component_element::AMMO ||
           id == ship_component_element::ARMOUR ||
           id == ship_component_element::HP ||
           id == ship_component_element::FUEL ||
           is_resource)
        {
            if(known_information < ship_info::misc_resources_obfuscation_level)
            {
                obfuscd = true;
            }
        }

        if(s.owned_by->parent_empire->is_allied(player_empire))
        {
            obfuscd = false;
        }

        header_str = obfuscate(header_str, obfuscd);
        prod_str = obfuscate(prod_str, obfuscd);
        cons_str = obfuscate(cons_str, obfuscd);
        net_str = obfuscate(net_str, obfuscd);
        store_max_str = obfuscate(store_max_str, obfuscd);

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

    auto initial_pos = ImGui::GetCursorScreenPos();

    std::string display_str;

    for(int i=0; i<headers.size(); i++)
    {
        std::string header_formatted = format(headers[i], headers);
        std::string net_formatted = format(net_list[i], net_list);
        std::string store_max_formatted = format(store_max_list[i], store_max_list);

        ship_component_element& id = found_elements[i];

        float max_hp = s.get_max_storage_of_components_with_this_primary(id, ship_component_element::HP);
        float cur_hp = s.get_total_storage_of_components_with_this_primary(id, ship_component_element::HP);

        vec3f hp_frac_col = {1,1,1};

        if(max_hp > FLOAT_BOUND)
        {
            hp_frac_col = hp_frac_to_col(cur_hp / max_hp);
        }

        ImGui::TextColored(header_formatted, hp_frac_col);

        if(ImGui::IsItemHovered() && max_hp > FLOAT_BOUND)
        {
            tooltip::add(to_string_with_enforced_variable_dp(cur_hp) + "/" + to_string_with_enforced_variable_dp(max_hp) + " HP in these systems");
        }

        ImGui::SameLine(0, 0);

        ImGui::Text(" : ");

        ImGui::SameLine(0.f, 0.f);

        vec3f col = {1,1,1};

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

        std::string displayed = header_formatted + "   " + net_formatted + "   " + store_max_formatted;

        if(displayed.length() > display_str.length())
        {
            display_str = displayed;
        }
    }

    int num_weapons = 0;

    for(component& c : s.entity_list)
    {
        if(c.is_weapon())
        {
            num_weapons++;
        }
    }

    if(num_weapons > 0)
    {
        std::string pad = "+";

        if(s.display_weapon)
        {
            pad = "-";
        }

        bool total_obfuscate = known_information <= ship_info::accessory_information_obfuscation_level;

        ImGui::Text(obfuscate(pad + "Weapons", total_obfuscate).c_str());

        if(ImGui::IsItemClicked())
        {
            s.display_weapon = !s.display_weapon;
        }
    }

    if(s.display_weapon)
    {
        ImGui::Indent();
    }

    for(component& c : s.entity_list)
    {
        if(c.is_weapon() && s.display_weapon)
        {
            vec3f col = hp_frac_to_col(c.get_hp_frac());

            ImGui::TextColored(obfuscate(c.name, primary_obfuscated[c.primary_attribute]), col);

            if(ImGui::IsItemHovered())
            {
                float cur_hp = c.get_stored()[ship_component_element::HP];
                float max_hp = c.get_stored_max()[ship_component_element::HP];

                std::string hp_str = to_string_with_enforced_variable_dp(cur_hp) + "/" + to_string_with_enforced_variable_dp(max_hp);

                hp_str = obfuscate(hp_str, known_information < c.scanning_difficulty);

                tooltip::add(hp_str + " HP");
            }
        }
    }

    if(s.display_weapon)
    {
        ImGui::Unindent();
    }

    if(!s.display_popout)
    {
        ImGui::Text("+Systems");

    }
    else
    {
        ImGui::Text("-Systems");
    }

    if(ImGui::IsItemClicked_Registered())
        s.display_popout = !s.display_popout;


    float scanning_power = player_empire->available_scanning_power_on(associated);

    if(scanning_power < 1)
    {
        std::string scanning_str = "Scanning Power: " + std::to_string((int)(scanning_power * 100.f)) + "%%";

        ImGui::Text(scanning_str.c_str());
    }

    float research_left = s.research_left_from_crewing.units_to_currency(false);

    std::string research_left_str = "Research left from crewing: " + std::to_string((int)research_left);

    if(research_left > 0)
        ImGui::Text(research_left_str.c_str());

    float cost = s.get_real_total_cost();

    std::string cost_str = "Cost: " + obfuscate(std::to_string((int)cost), known_information < 0.8f);

    ImGui::Text(cost_str.c_str());

    if(ImGui::IsItemHovered() && s.fully_disabled())
    {
        ImGui::SetTooltip("Amount recovered through salvage");
    }
    ///have a recovery cost display

    //orbital* o = system_manage.get_by_element_orbital(s.owned_by);
    orbital* o = associated;

    if(s.owned_by != nullptr && s.owned_by->parent_empire != nullptr && popup.fetch(o) == nullptr)
    {
        ImGui::NeutralText("(Select Fleet)");

        if(ImGui::IsItemClicked_Registered())
        {
            if(!key_down(sf::Keyboard::LShift))
                popup.schedule_rem_all();

            if(o)
            {
                popup.insert(o);
                popup.going = true;
            }
            else
            {
                printf("select fleet popup error no orbital\n");
            }
        }
    }

    ///if derelict SALAGE BBZ or recapture YEAAAAAH
    ///recapturing will take some resources to prop up the crew and some necessary systems
    ///or just... fully repair? Maybe make a salvage literally just a resupply + empire change?

    auto win_size = ImGui::GetWindowSize();
    auto win_pos = ImGui::GetWindowPos();

    ImGui::End();

    if(s.display_popout)
    {
        ImGui::SetNextWindowPos(ImVec2(win_pos.x + win_size.x - ImGui::GetStyle().FramePadding.x, win_pos.y + get_title_bar_height()));
        ImGui::BeginOverride(("###SIDE" + s.name + std::to_string(s.id)).c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW_INPUTS);

        do_popout(s, known_information, player_empire);

        ImGui::End();
    }

    ImGui::PopStyleColor(6);
}

/*void display_ship_info_old(ship& s, float step_s)
{
    auto display_strs = get_components_display_string(s);

    ImGui::BeginOverride(s.name.c_str());

    int num = 0;

    for(auto& i : display_strs)
    {
        ImGui::Text(i.c_str());

        if(num != display_strs.size()-1)
            ImGui::Text("-");

        //ImGui::SameLine();
        num++;
    }

    for(component& i : s.entity_list)
    {
        if(i.has_element(ship_component_elements::RAILGUN))
        {
            if(s.can_use(i) && ImGui::Button("Fire Railgun"))
            {
                s.use(i);
            }
        }
    }

    ImGui::End();
}*/


/*void debug_menu(const std::vector<ship*>& ships)
{
    ImGui::BeginOverride("Debug");

    if(ImGui::Button("Tick 1s"))
    {
        for(auto& i : ships)
        {
            i->tick_all_components(1.f);
        }
    }

    ImGui::End();
}*/

/*void debug_battle(battle_manager* battle, sf::RenderWindow& win, bool lclick, system_manager& system_manage, empire* viewing_empire)
{
    if(battle == nullptr)
        return;

    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    for(const auto& ship_map : battle->ships)
    {
        for(ship* s : ship_map.second)
        {
            auto fully_merged = s->get_fully_merged(1.f);

            vec2f spos = s->local_pos;

            auto transformed = win.mapCoordsToPixel({spos.x(), spos.y()});

            ImGui::SetNextWindowPos(ImVec2(transformed.x, transformed.y));

            ImGui::BeginOverride((s->name + "##" + std::to_string(s->id)).c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW);

            ImGui::Text(s->name);

            //for(const component_attribute& attr : fully_merged)
            for(int type = 0; type < (int)ship_component_elements::NONE; type++)
            {
                const component_attribute& attr = fully_merged[type];

                float stored = attr.cur_amount;
                float stored_max = attr.max_amount;

                if(s->get_component_with((ship_component_element)type) == nullptr)
                    continue;


            }

            ImGui::End();
        }
    }
}*/

void debug_battle(battle_manager* battle, sf::RenderWindow& win, bool lclick, system_manager& system_manage, empire* viewing_empire)
{
    if(battle == nullptr)
        return;

    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    ship* s = battle->get_ship_under({transformed.x, transformed.y});
    orbital* o = battle->get_orbital_under({transformed.x, transformed.y});

    if(s)
    {
        s->highlight = true;

        float available_information = viewing_empire->available_scanning_power_on(o);

        ImGui::BeginTooltip();

        vec3f relations_col = viewing_empire->get_relations_colour(s->owned_by->parent_empire, true);

        std::string relations_str = viewing_empire->get_short_relations_str(s->owned_by->parent_empire);

        std::string sname = obfuscate(s->name, available_information < ship_info::ship_obfuscation_level);

        ImGui::TextColored(relations_str, relations_col);

        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        ImGui::Text(sname.c_str());

        if(s->fully_disabled())
        {
            ImGui::SameLine();

            ImGui::Text("|");

            ImGui::SameLine();

            ImGui::TextColored("Derelict", popup_colour_info::bad_ui_colour);
        }
        else
        {
            ImGui::SameLine();

            auto fully_merged = s->get_fully_merged(1.f);

            float hp = fully_merged[ship_component_element::HP].cur_amount;
            float hp_max = fully_merged[ship_component_element::HP].max_amount;

            ImGui::Text("|");

            ImGui::SameLine();

            std::string hp_str = to_string_with_enforced_variable_dp(hp) + "/" + to_string_with_enforced_variable_dp(hp_max);

            hp_str = obfuscate(hp_str, available_information < ship_info::misc_resources_obfuscation_level);

            ImGui::Text(hp_str);
        }

        ImGui::EndTooltip();

        if(lclick)
        {
            s->display_ui = !s->display_ui;
        }
    }

    /*ImGui::BeginOverride("Battle DBG");

    if(ImGui::Button("Step Battle 1s"))
    {
        battle->tick(1.f, system_manage);
    }

    ImGui::End();*/
}

void init_battle_window()
{
    ImGui::BeginOverride("Ongoing Battles", &top_bar::active[top_bar_info::BATTLES], IMGUI_WINDOW_FLAGS | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::clamp_window_to_screen();
}

///maybe battle view should be very flexible, like entering/leaving a system is, where it feels
///seamless completely entering and leaving
void debug_all_battles(all_battles_manager& all_battles, sf::RenderWindow& win, bool lclick, system_manager& system_manage, empire* player_empire, bool in_battle_view)
{
    if(in_battle_view)
        debug_battle(all_battles.get_currently_viewing(), win, lclick, system_manage, player_empire);

    if(!top_bar::get_active(top_bar_info::BATTLES))
        return;

    //for(int i=0; i<all_battles.battles.size(); i++)
    //for(battle_manager* bm : all_battles.battles)

    auto it = all_battles.battles.begin();
    int i = 0;

    bool have_any_battles = false;

    bool has_battle_window = false;

    for(; it != all_battles.battles.end(); it++, i++)
    {
        std::string id_str = std::to_string(i);

        battle_manager* bm = *it;

        if(!bm->any_in_empire_involved(player_empire))
            continue;

        std::vector<std::string> names;

        for(orbital* o : bm->ship_map)
        {
            for(ship* kk : o->data->ships)
            {
                names.push_back(kk->name);
            }
        }

        if(!has_battle_window)
        {
            init_battle_window();
            has_battle_window = true;
        }

        have_any_battles = true;

        for(orbital* o : bm->ship_map)
        {
            bool do_obfuscate_name = false;

            if(player_empire->available_scanning_power_on(o) <= ship_info::ship_obfuscation_level)
            {
                do_obfuscate_name = true;
            }

            bool do_obfuscate_hp = false;

            if(player_empire->available_scanning_power_on(o) <= ship_info::misc_resources_obfuscation_level)
            {
                do_obfuscate_hp = true;
            }

            for(ship* kk : o->data->ships)
            {
                std::string name = format(kk->name, names);

                name = obfuscate(name, do_obfuscate_name);

                std::string team = player_empire->get_single_digit_relations_str(kk->owned_by->parent_empire);

                auto fully_merged = kk->get_fully_merged(1.f);

                float damage = fully_merged[ship_component_elements::HP].cur_amount;
                float damage_max = fully_merged[ship_component_elements::HP].max_amount;

                std::string damage_str = to_string_with_enforced_variable_dp(damage) + "/" + to_string_with_enforced_variable_dp(damage_max);

                damage_str = obfuscate(damage_str, do_obfuscate_hp);

                ImGui::TextColored(team, player_empire->get_relations_colour(kk->owned_by->parent_empire, true));

                if(ImGui::IsItemHovered())
                {
                    tooltip::add(player_empire->get_short_relations_str(kk->owned_by->parent_empire));
                }

                ImGui::SameLine();

                ImGui::Text("|");

                ImGui::SameLine();

                vec3f text_col = {1,1,1};

                if(kk->fully_disabled())
                {
                    damage_str = "Derelict";
                    text_col = popup_colour_info::bad_ui_colour;
                }

                ImGui::NeutralText(name.c_str());

                if(ImGui::IsItemClicked_Registered())
                {
                    kk->display_ui = !kk->display_ui;
                }

                if(ImGui::IsItemHoveredRect())
                {
                    kk->highlight = true;
                }

                ImGui::SameLine();

                ImGui::Text("|");

                ImGui::SameLine();

                ImGui::TextColored(damage_str, text_col);
            }
        }

        ///will be incorrect for a frame when combat changes
        std::string jump_to_str = "(Jump To Battle)";

        ImGui::NeutralText(jump_to_str);

        if(ImGui::IsItemClicked_Registered())
        {
            all_battles.request_enter_battle_view = true;

            all_battles.set_viewing(bm, system_manage);
        }

        std::string jump_to_sys_str = "(Jump To System)";

        ImGui::NeutralText(jump_to_sys_str);

        if(ImGui::IsItemClicked_Registered())
        {
            all_battles.request_leave_battle_view = true;
            all_battles.request_stay_in_battle_system = true;
            all_battles.request_stay_system = bm->get_system_in();
        }

        if(!bm->can_end_battle_peacefully(player_empire))
        {
            std::string disengage_str = bm->get_disengage_str(player_empire);

            if(bm->can_disengage(player_empire))
            {
                disengage_str = "(Emergency Disengage!)";
            }

            ImGui::NeutralText(disengage_str);

            if(ImGui::IsItemClicked_Registered())
            {
                if(bm->can_disengage(player_empire))
                {
                    all_battles.disengage(bm, player_empire);
                    i--;
                    continue;
                }
            }

            if(ImGui::IsItemHovered())
            {
                if(bm->can_disengage(player_empire))
                    ImGui::SetTooltip("Warning, this fleet will take heavy damage!");
                else
                    ImGui::SetTooltip("Cannot disengage");
            }
        }
        else
        {
            std::string leave_str = "(Terminate Battle Peacefully)";

            ImGui::NeutralText(leave_str);

            if(ImGui::IsItemClicked_Registered())
            {
                all_battles.end_battle_peacefully(bm);
            }
        }
    }

    if(in_battle_view)
    {
        if(!has_battle_window)
        {
            init_battle_window();
            has_battle_window = true;
        }

        ImGui::NeutralText("(Leave Battle View)");

        if(ImGui::IsItemClicked_Registered())
        {
            all_battles.request_leave_battle_view = true;
        }
    }

    if(has_battle_window)
        ImGui::End();
}

///mostly working except we cant rebox select if we have something selected
///this was to prevent box selecting an object, but we could probably do this by limiting
///distance
struct box_selection
{
    bool going = false;

    vec2f start_pos;
    vec2f end_pos;

    bool last_was_not_click = false;
    bool cannot_click = false;

    void tick(system_manager& system_manage, orbital_system* cur, sf::RenderWindow& win, popup_info& popup, empire* viewer_empire)
    {
        sf::Mouse mouse;

        vec2f mpos = {mouse.getPosition(win).x, mouse.getPosition(win).y};

        bool lclick = mouse.isButtonPressed(sf::Mouse::Left);

        sf::Keyboard key;

        bool lctrl = key_down(sf::Keyboard::LControl);
        bool lalt = key_down(sf::Keyboard::LAlt);

        std::vector<orbital*> potential_orbitals;

        bool any_is_fleet = false;

        if(going)
        {
            std::vector<clickable> orbitals;

            if(cur != nullptr)
                orbitals = cur->get_clickables(win);

            if(!system_manage.in_system_view())
                orbitals = system_manage.advertised_universe_orbitals;

            for(clickable& click : orbitals)
            {
                orbital* o = click.o;

                if(o->type == orbital_info::FLEET && lalt)
                    continue;

                if(!o->viewed_by[viewer_empire])
                    continue;

                vec2f pos = click.pos;

                vec2f tl = min(mpos, start_pos);
                vec2f br = max(mpos, start_pos);

                auto spos = win.mapCoordsToPixel({pos.x(), pos.y()});

                if(spos.x < br.x() && spos.x >= tl.x() && spos.y < br.y() && spos.y >= tl.y())
                {
                    potential_orbitals.push_back(o);

                    if(o->type == orbital_info::FLEET)
                        any_is_fleet = true;
                }
            }

            if(!lclick)
                going = false;
        }

        for(auto it = potential_orbitals.begin(); it != potential_orbitals.end();)
        {
            if(any_is_fleet && (*it)->type != orbital_info::FLEET)
            {
                it = potential_orbitals.erase(it);
            }
            else
            {
                it++;
            }
        }

        bool can_select_not_my_orbitals = true;

        for(orbital* o : potential_orbitals)
        {
            if(o->parent_empire == viewer_empire)
            {
                can_select_not_my_orbitals = false;
                break;
            }
        }

        if(lctrl)
            can_select_not_my_orbitals = true;

        for(orbital* o : potential_orbitals)
        {
            if(!can_select_not_my_orbitals && o->parent_empire != viewer_empire)
                continue;

            if(!lclick && o->type == orbital_info::FLEET)
            {
                ship_manager* sm = (ship_manager*)o->data;

                sm->toggle_fleet_ui = true;
            }

            if(!lclick)
            {
                popup.insert(o);

                popup.going = true;
            }
            else
            {
                o->highlight = true;
            }
        }

        bool suppress_mouse = ImGui::IsAnyItemHovered() || ImGui::IsMouseHoveringAnyWindow();

        suppress_mouse = suppress_mouse || ImGui::suppress_clicks;

        if(suppress_mouse)
        {
            cannot_click = true;
            last_was_not_click = false;
            return;
        }

        if(!key_down(sf::Keyboard::LShift) && !going && cur != nullptr)
        {
            for(orbital* o : cur->orbitals)
            {
                if(o->was_hovered)
                {
                    cannot_click = true;
                    return;
                }
            }
        }

        if(!mouse.isButtonPressed(sf::Mouse::Left))
        {
            last_was_not_click = true;
            cannot_click = false;
        }

        if(last_was_not_click && mouse.isButtonPressed(sf::Mouse::Left) && !cannot_click)
        {
            going = true;
            start_pos = mpos;

            last_was_not_click = false;
        }

        if(going)
        {
            sf::RectangleShape shape;

            vec2f tl = min(mpos, start_pos);

            auto backup_view = win.getView();

            win.setView(win.getDefaultView());

            shape.setSize({fabs(mpos.x() - start_pos.x()), fabs(mpos.y() - start_pos.y())});
            shape.setOutlineColor(sf::Color(0, 128, 255, 255));
            shape.setFillColor(sf::Color(0, 128, 255, 60));
            shape.setOutlineThickness(1);

            shape.setPosition(tl.x(), tl.y());

            win.draw(shape);

            win.setView(backup_view);
        }
    }
};

///this function is one of the worst in the code, there's a lot of duplication that's gradually being exposed
///new functionality is however forcing this to be refactored to be less dumb
void debug_system(system_manager& system_manage, sf::RenderWindow& win, bool lclick, bool rclick, popup_info& popup, empire* player_empire, all_battles_manager& all_battles, fleet_manager& fleet_manage, all_events_manager& all_events)
{
    popup.remove_scheduled();

    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    sf::Keyboard key;

    bool lshift = key_down(sf::Keyboard::LShift);
    bool lctrl = key_down(sf::Keyboard::LControl);

    if(ImGui::suppress_clicks)
    {
        lclick = false;
    }

    ///this is where we click away fleets
    if(lclick && !lshift && (system_manage.hovered_system == nullptr || system_manage.in_system_view()) && !ImGui::suppress_clicks)
    {
        popup.going = false;

        popup.clear();
    }

    for(ship_manager* sm : fleet_manage.fleets)
    {
        if(sm->to_close_ui)
        {
            sm->toggle_fleet_ui = false;
        }

        sm->to_close_ui = false;
    }

    bool first = true;

    std::vector<orbital*> valid_selection_targets;

    if(system_manage.currently_viewed != nullptr)
    {
        for(orbital* orb : system_manage.currently_viewed->orbitals)
        {
            orb->was_hovered = false;
        }
    }

    if(system_manage.currently_viewed != nullptr && system_manage.in_system_view())
    {
        float min_dist = FLT_MAX;
        orbital* min_orb = nullptr;

        std::vector<clickable> clickables = system_manage.currently_viewed->get_clickables(win);

        for(clickable& click : clickables)
        {
            orbital* orb = click.o;

            if(orb->point_within({transformed.x, transformed.y}, win, click.pos))
            {
                vec2f dist = (vec2f){transformed.x, transformed.y} - orb->absolute_pos;

                if(dist.length() < min_dist)
                {
                    min_dist = dist.length();
                    min_orb = orb;
                }
            }
        }

        if(min_orb != nullptr)
        {
            valid_selection_targets.push_back(min_orb);

            min_orb->was_hovered = true;
        }
    }

    ///CLICKABLE: note not using clickables
    for(auto& i : system_manage.hovered_orbitals)
    {
        valid_selection_targets.push_back(i);
    }

    for(popup_element& elem : popup.elements)
    {
        orbital* o = (orbital*)elem.element;

        o->highlight = true;

        if(o->type == orbital_info::FLEET)
        {
            ship_manager* sm = (ship_manager*)o->data;

            sm->toggle_fleet_ui = true;
        }
    }

    float double_click_time_s = 0.3f;

    orbital* double_clicked = nullptr;

    for(orbital* orb : valid_selection_targets)
    {
        ///not necessarily valid
        ship_manager* sm = (ship_manager*)orb->data;

        orb->clicked = false;

        if(first)
        {
            orb->highlight = true;
            first = false;
        }

        /*if(rclick)
        {
            context_menu::set_item(orb);
        }*/

        bool double_click = orb->time_since_last_clicked.getElapsedTime().asMicroseconds() / 1000. / 1000. < double_click_time_s;

        if(lclick && double_click)
        {
            double_clicked = orb;
        }

        if(lclick)
        {
            orb->clicked = true;
            orb->time_since_last_clicked.restart();
        }

        if(lclick && popup.fetch(orb) && double_clicked == nullptr)
        {
            if(orb->type == orbital_info::FLEET)
            {
                ship_manager* sm = (ship_manager*)orb->data;

                sm->toggle_fleet_ui = false;
            }

            popup.rem(orb);
            break;
        }

        if(lclick && popup.fetch(orb) == nullptr)
        {
            popup.going = true;

            ///do buttons here
            popup_element elem;
            elem.element = orb;

            popup.elements.push_back(elem);

            if(orb->type == orbital_info::FLEET)
                sm->toggle_fleet_ui = true;
        }

        if(orb->type == orbital_info::STAR)
        {
            ImGui::SetTooltip((orb->get_full_name() + "\n" + orb->get_empire_str() + orb->description).c_str());
        }

        if(orb->is_resource_object)
        {
            std::string res_first = orb->get_full_name();

            res_first += "\n" + orb->get_empire_str();

            ImGui::BeginTooltip();

            if(orb->type != orbital_info::ASTEROID)
            {
                ImGui::Text(res_first.c_str());

                if(orb->viewed_by[player_empire])
                    orb->produced_resources_ps.render_formatted_str(true);
            }
            else
            {
                orb->render_detail_ui_info();
            }

            ImGui::EndTooltip();
        }
    }

    if(double_clicked != nullptr)
    {
        ImGui::suppress_clicks = true;

        for(orbital* orb : double_clicked->parent_system->orbitals)
        {
            if(orb == double_clicked)
                continue;

            bool allow_any_mix = lshift;

            if(orb->type == orbital_info::FLEET && double_clicked->type == orbital_info::FLEET)
            {
                ship_type::types type = double_clicked->data->get_most_common_ship_type();

                if(!allow_any_mix)
                {
                    if(!orb->data->majority_of_type(type))
                        continue;
                }

                bool should_allow_all_select = lctrl;

                if(!should_allow_all_select && orb->data->parent_empire != double_clicked->data->parent_empire)
                {
                    continue;
                }

                if(popup.fetch(orb) == nullptr)
                {
                    popup.going = true;

                    ///do buttons here
                    popup_element elem;
                    elem.element = orb;

                    popup.elements.push_back(elem);

                    if(orb->type == orbital_info::FLEET)
                        orb->data->toggle_fleet_ui = true;
                }
            }
        }

        if(double_clicked->type == orbital_info::PLANET && double_clicked->parent_empire == player_empire)
        {
            double_clicked->construction_ui_open = !double_clicked->construction_ui_open;
        }
    }
}

void do_popup(popup_info& popup, sf::RenderWindow& win, fleet_manager& fleet_manage, system_manager& system_manage, orbital_system* current_system, empire_manager& empires, empire* player_empire, all_events_manager& all_events, all_battles_manager& all_battles, bool rclick)
{
    popup.remove_scheduled();

    if(popup.elements.size() == 0)
        popup.going = false;

    if(!popup.going)
        return;

    sf::Mouse mouse;
    sf::Keyboard key;

    bool lshift = key_down(sf::Keyboard::LShift);
    bool lctrl = key_down(sf::Keyboard::LControl);

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    global_drag_and_drop.begin_drag_section("INFO_PANEL");

    ImGui::BeginOverride(("Selected###INFO_PANEL"), &popup.going, ImVec2(0,0), -1.f, ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    ImGui::clamp_window_to_screen();

    //global_drag_and_drop.begin_dragging(nullptr, drag_and_drop_info::ORBITAL);

    std::set<ship*> potential_new_fleet;

    ///use a custom sorter here to make sure that we end up at the top etc
    std::map<empire_popup, std::vector<orbital*>> orbitals_grouped_by_empire;

    std::map<ship_manager*, std::vector<ship*>> steal_map;

    for(popup_element& elem : popup.elements)
    {
        orbital* orb = (orbital*)elem.element;

        empire* cur_empire = orb->parent_empire;

        bool do_obfuscate_misc = false;

        if(orb->type == orbital_info::FLEET && player_empire->available_scanning_power_on(orb) <= ship_info::accessory_information_obfuscation_level)
        {
            do_obfuscate_misc = true;
        }

        if(player_empire->is_allied(orb->parent_empire))
        {
            do_obfuscate_misc = false;
        }

        empire_popup pop;
        pop.e = cur_empire;
        pop.id = orb->unique_id;
        pop.hidden = do_obfuscate_misc;
        pop.type = orb->type;
        pop.is_player = cur_empire == player_empire;
        pop.is_allied = player_empire->is_allied(cur_empire);

        orbitals_grouped_by_empire[pop].push_back(orb);
    }

    for(auto& grouped_orbitals : orbitals_grouped_by_empire)
    {
        const empire_popup& pop = grouped_orbitals.first;

        empire* current_empire = pop.e;

        std::string empire_name = "Empire: None";

        if(current_empire != nullptr)
        {
            empire_name = "Empire: " + current_empire->name;
        }

        if(pop.hidden)
        {
            empire_name = "Empire: Unknown";
        }

        vec3f col = player_empire->get_relations_colour(pop.hidden ? nullptr : current_empire);

        ImGui::ColourNoHoverText(empire_name, col);

        if(orbitals_grouped_by_empire.size() > 1)
        {
            ImGui::SameLine();

            ImGui::NeutralText("(Select)");

            if(ImGui::IsItemClicked_Registered())
            {
                for(auto& test_popup : orbitals_grouped_by_empire)
                {
                    if(test_popup.first.e == current_empire && test_popup.first.hidden == pop.hidden)
                        continue;

                    for(orbital* orb : test_popup.second)
                    {
                        popup.schedule_rem(orb);
                    }
                }
            }
        }

        ImGui::Indent();

        bool first_orbital = true;

        for(orbital* orb : grouped_orbitals.second)
        {
            if(orb->type != orbital_info::FLEET && !first_orbital)
                ImGui::NewLine();

            first_orbital = false;

            orbital* o = orb;

            ship_manager* sm = (ship_manager*)orb->data;

            {
                bool do_obfuscate_name = false;
                //bool do_obfuscate_misc_resources = false;

                if(o->type == orbital_info::FLEET && !player_empire->is_allied(o->parent_empire))
                {
                    if(player_empire->available_scanning_power_on(o) <= ship_info::ship_obfuscation_level)
                    {
                        do_obfuscate_name = true;
                    }

                    /*if(player_empire->available_scanning_power_on((ship_manager*)o->data, system_manage) <= ship_info::misc_resources_obfuscation_level)
                    {
                        do_obfuscate_misc_resources = true;
                    }*/
                }

                std::string name = o->name;

                if(name == "")
                    name = orbital_info::names[o->type];

                if(do_obfuscate_name)
                {
                    name = "Unknown";

                    if(o->type == orbital_info::FLEET)
                    {
                        name = "Unknown Fleet";
                    }
                }

                o->highlight = true;

                if(rclick && (o->type == orbital_info::FLEET) && o->parent_empire == player_empire && !sm->any_colonising() && system_manage.in_system_view())
                {
                    //o->request_transfer({transformed.x, transformed.y}, system_manage.currently_viewed);

                    o->command_queue.transfer({transformed.x, transformed.y}, o, system_manage.currently_viewed, true, false, false, lshift);
                }

                ImGui::BeginGroup();

                //ImGui::Text(name.c_str());

                if(o->type == orbital_info::FLEET)
                {
                    if(grouped_orbitals.second.size() > 1 || orbitals_grouped_by_empire.size() > 1)
                    {
                        ImGui::NeutralText(name);

                        if(ImGui::IsItemHovered())
                        {
                            tooltip::add("Click to select this fleet");
                        }

                        if(ImGui::IsItemClicked_Registered() && !lctrl)
                        {
                            popup.schedule_rem_all_but(o);
                        }

                        if(ImGui::IsItemClicked_Registered() && lctrl)
                        {
                            popup.schedule_rem(o);
                        }
                    }
                    else
                    {
                        ImGui::NeutralText(name);
                    }
                }
                else
                {
                    ImGui::Text(o->get_full_name().c_str());
                }

                if(ImGui::IsItemHovered() && o->type == orbital_info::FLEET)
                {
                    tooltip::add("Right Click for fleet options");
                }

                if(o->type == orbital_info::FLEET && ImGui::IsItemClicked_Registered(1))
                {
                    o->data->context_request_open = true;
                }

                ///do warp charging info here
                if(o->type == orbital_info::FLEET && o->parent_empire == player_empire)
                {
                    ImGui::SameLine();

                    ImGui::Text("|");

                    ImGui::SameLine();

                    std::string warp_str = "Warp: ";

                    float warp_use_frac = sm->get_overall_warp_drive_use_frac();

                    if(sm->can_use_warp_drives())
                    {
                        warp_str += "Ready";

                        //warp_str += to_string_with_enforced_variable_dp(warp_use_frac * 100.f, 1) + "%%";

                        ImGui::GoodTextNoHoverEffect(warp_str.c_str());
                    }
                    else
                    {
                        //warp_str += "Not Ready";

                        warp_str += to_string_with_enforced_variable_dp(warp_use_frac * 100.f, 1) + "%%";

                        ImGui::BadTextNoHoverEffect(warp_str.c_str());
                    }

                    if(ImGui::IsItemHovered())
                    {
                        for(ship* s : sm->ships)
                        {
                            //tooltip::add(s->name + ": " + s->get_resource_str(ship_component_element::WARP_POWER));

                            tooltip::add(s->name);

                            for(int kk = 0; kk < s->entity_list.size(); kk++)
                            {
                                component& c = s->entity_list[kk];

                                if(c.primary_attribute != ship_component_element::WARP_POWER)
                                    continue;

                                auto warp_fracs = s->get_use_frac(c);

                                bool any = false;

                                for(auto& i : warp_fracs)
                                {
                                    int type = i.first;

                                    float amount = i.second;

                                    if(amount >= 1.f - FLOAT_BOUND)
                                        continue;

                                    any = true;

                                    tooltip::add(ship_component_elements::display_strings[type] + " " + to_string_with_enforced_variable_dp(amount * 100.f, 1) + "%%");
                                }

                                if(!any && s->can_use_warp_drives())
                                {
                                    tooltip::add("Ready to warp");
                                }

                                ///Not a resource issue, ie damage
                                if(!any && !s->can_use_warp_drives())
                                {
                                    tooltip::add("Cannot Warp");
                                }
                            }

                            if(sm->ships.size() > 1 && s != sm->ships[sm->ships.size()-1])
                                tooltip::add("");

                            //tooltip::add(s->name + ": " + to_string_with_enforced_variable_dp(s->get_warp_use_frac() * 100.f, 1) + "%%");
                        }
                    }

                    //if(o->command_queue.trying_to_warp())
                    if(o->command_queue.get_warp_destinations().size() != 0)
                    {
                        ImGui::SameLine();

                        ImGui::NeutralText("(Locked)");

                        if(ImGui::IsItemHovered())
                        {
                            std::vector<orbital_system*> systems = o->command_queue.get_warp_destinations();

                            int num = 1;

                            for(orbital_system* sys : systems)
                            {
                                tooltip::add(std::to_string(num) + ". " + sys->get_base()->name);

                                num++;
                            }

                            tooltip::add("Click to cancel");
                        }

                        if(ImGui::IsItemClicked_Registered())
                        {
                            o->command_queue.cancel();
                        }

                        ImGui::SameLine();
                    }

                    ImGui::SameLine();

                    ImGui::Text("|");

                    ImGui::SameLine();

                    std::string fuel_text = sm->get_fuel_message();

                    vec3f col = mix(popup_colour_info::bad_ui_colour, popup_colour_info::good_ui_colour, sm->get_min_fuel_frac());

                    ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), ("Fuel: " + fuel_text).c_str());

                    if(ImGui::IsItemHovered())
                    {
                        for(ship* s : sm->ships)
                        {
                            tooltip::add(s->name + ": " + s->get_resource_str(ship_component_element::FUEL));
                        }
                    }
                }


                /*if(popup.elements.size() > 1)
                {
                    ImGui::SameLine();

                    ImGui::Text("(Select)");
                }

                if(ImGui::IsItemClicked_Registered())
                {
                    popup.rem_all_but(o);
                }*/

                std::vector<std::string> data = o->get_info_str(player_empire, true, true, false);

                if(o->description != "" && o->viewed_by[player_empire])
                    data.push_back(o->description);

                if(o->type == orbital_info::FLEET)
                    ImGui::Indent();

                for(int i=0; i<data.size(); i++)
                {
                    std::string str = data[i];

                    if(o->type == orbital_info::FLEET)
                    {
                        ship_manager* sm = (ship_manager*)o->data;

                        if(sm->ships[i]->shift_clicked)
                        {
                            str = str + "+";
                        }
                    }

                    ship* cur_ship = nullptr;
                    vec3f draw_col = {1,1,1};

                    float hp_frac = 1.f;

                    if(o->type == orbital_info::FLEET)
                    {
                        cur_ship = sm->ships[i];

                        auto fully_merged = cur_ship->get_fully_merged(1.f);

                        hp_frac = fully_merged[ship_component_elements::HP].cur_amount / fully_merged[ship_component_elements::HP].max_amount;

                        draw_col = hp_frac_to_col(hp_frac);
                    }

                    if(o->type == orbital_info::FLEET)
                    {
                        ship_manager* sm = o->data;

                        ship* s = o->data->ships[i];

                        #ifdef HEALTH_BAR_EXPERIMENT
                        ImVec2 text_dim = ImGui::CalcTextSize(str.c_str());

                        auto pos = ImGui::GetCursorPos();

                        imgui_hp_bar(hp_frac, {draw_col.x(), draw_col.y(), draw_col.z(), 0.2f}, {text_dim.x, text_dim.y});

                        ImGui::SetCursorPos(pos);
                        #endif

                        ImGui::ColourHoverText(str, draw_col);

                        if(ImGui::IsItemClicked_Registered(1))
                        {
                            s->context_request_open = true;
                        }
                    }
                    else
                        ImGui::Text(str.c_str());

                    if(o->type == orbital_info::FLEET)
                    {
                        bool shift = key_down(sf::Keyboard::LShift);

                        if(ImGui::IsItemHovered())
                            tooltip::add("Right Click for ship options");
                        if(sm->can_merge && ImGui::IsItemHovered())
                            tooltip::add("Shift-Click to add to fleet");

                        if(ImGui::IsItemClicked_DragCompatible() && !shift && !lctrl)
                        {
                            cur_ship->display_ui = !cur_ship->display_ui;
                        }

                        if(ImGui::IsItemClicked_DragCompatible() && shift && !lctrl && sm->can_merge)
                        {
                            cur_ship->shift_clicked = !cur_ship->shift_clicked;
                        }

                        if(ImGui::IsItemClicked_UnRegistered() && sm->can_merge)
                        {
                            global_drag_and_drop.begin_dragging(cur_ship, drag_and_drop_info::SHIP, cur_ship->name);
                        }
                    }
                }

                if(o->is_resource_object)
                {
                    o->produced_resources_ps.render_formatted_str(true);
                }

                if(o->type == orbital_info::FLEET)
                    ImGui::Unindent();

                ImGui::EndGroup();

                bool should_merge_into_fleet = false;

                if(global_drag_and_drop.currently_dragging == drag_and_drop_info::SHIP && global_drag_and_drop.let_go_on_item())
                {
                    should_merge_into_fleet = true;
                }

                if(should_merge_into_fleet)
                {
                    ship* s = (ship*)global_drag_and_drop.data;

                    ship_manager* found_sm = s->owned_by;

                    orbital* my_o = system_manage.get_by_element_orbital(found_sm);

                    if(my_o != nullptr && my_o->type == orbital_info::FLEET && sm != found_sm)
                    {
                        my_o->dirty = true;
                        found_sm->dirty = true;
                        s->dirty = true;

                        if(found_sm->ships.size() == 1)
                        {
                            popup.schedule_rem(my_o);
                        }

                        orbital_system* base_sm = system_manage.get_by_element(sm);

                        ///delay so we dont affect text rendering
                        if(base_sm == my_o->parent_system)
                        {
                            steal_map[sm].push_back(s);
                        }
                    }
                }
            }

            //ImGui::SameLine();

            //ImGui::Text("");

            /*if(popup.elements.size() > 1)
            {
                ImGui::NeutralText("(Select)");

                ImGui::SameLine();

                if(ImGui::IsItemClicked_Registered() && !lctrl)
                {
                    popup.schedule_rem_all_but(o);
                }

                if(ImGui::IsItemClicked_Registered() && lctrl)
                {
                    popup.schedule_rem(o);
                }
            }*/

            if(orb->type == orbital_info::FLEET)
            {
                bool can_resupply = orb->type == orbital_info::FLEET && (orb->parent_empire == player_empire || orb->parent_empire->is_allied(player_empire)) && !(sm->any_in_combat() || sm->all_derelict());

                ///disabling merging here and resupply invalides all fleet actions except moving atm
                ///unexpected fix to fleet merging problem
                ///disable resupply if in combat
                if(can_resupply && orb->parent_empire == player_empire)
                    sm->can_merge = true;
            }

            orbital_system* parent_system = orb->parent_system;

            std::vector<orbital*> hostile_fleets = parent_system->get_fleets_within_engagement_range(orb, true);

            bool can_engage = hostile_fleets.size() > 0 && orb->parent_empire == player_empire && orb->type == orbital_info::FLEET && sm->can_engage() && !sm->any_in_combat();

            if(can_engage)
            {
                ImGui::BadText("(Engage Fleets)");

                ImGui::SameLine();

                if(ImGui::IsItemClicked_Registered())
                {
                    ///will automatically engage all applicable fleets within range
                    ///instantly set to false if none applicable
                    ///is not a "hunting for fights" mode
                    //o->data->requesting_or_in_battle = true;

                    assert(parent_system);

                    std::vector<orbital*> hostile_fleets = parent_system->get_fleets_within_engagement_range(o, true);

                    hostile_fleets.push_back(o);

                    all_battles.make_new_battle(hostile_fleets);
                }
            }

            if(orb->type == orbital_info::FLEET)
            {
                bool enable_engage_cooldown = !sm->can_engage() && sm->parent_empire == player_empire;

                if(enable_engage_cooldown)
                {
                    ImGui::NeutralText(sm->get_engage_str().c_str());
                }
            }

            ImGui::SameLine();

            ImGui::Text("");

            bool this_fleet_is_coloniser = false;
            ship* colony_ship = nullptr;

            if(o->type == orbital_info::FLEET && sm->parent_empire == player_empire)
            {
                for(ship* s : sm->ships)
                {
                    component* fc = s->get_component_with_primary(ship_component_elements::COLONISER);

                    if(fc != nullptr)
                    {
                        this_fleet_is_coloniser = true;
                        colony_ship = s;
                        break;
                    }
                }
            }

            if(this_fleet_is_coloniser)
            {
                ImGui::OutlineHoverTextAuto("(Colonise Planet)", popup_colour_info::good_ui_colour, true, {0,0}, 1, sm->auto_colonise);

                if(ImGui::IsItemClicked_Registered())
                {
                    o->command_queue.colonise_ui_state(colony_ship, lshift);
                }

                if(ImGui::IsItemHovered())
                {
                    if(sm->auto_colonise)
                    {
                        tooltip::add("Right click to disable auto colonise");
                    }
                    else
                    {
                        tooltip::add("Right click to enable auto colonise");
                    }
                }

                if(ImGui::IsItemClicked_Registered(1))
                {
                    sm->auto_colonise = !sm->auto_colonise;
                }
            }

            if(o->type == orbital_info::FLEET && sm->any_with_element(ship_component_elements::MASS_ANCHOR) && o->parent_empire == player_empire)
            {
                ImGui::OutlineHoverTextAuto("(Harvest Ore)", popup_colour_info::good_ui_colour, true, {0,0}, 1, sm->auto_harvest_ore);

                if(ImGui::IsItemClicked_Registered())
                {
                    o->command_queue.anchor_ui_state(lshift);
                }

                if(ImGui::IsItemHovered())
                {
                    if(sm->auto_harvest_ore)
                    {
                        tooltip::add("Right click to disable auto mine");
                    }
                    else
                    {
                        tooltip::add("Right click to enable auto mine");
                    }
                }

                if(ImGui::IsItemClicked_Registered(1))
                {
                    sm->auto_harvest_ore = !sm->auto_harvest_ore;
                }
            }

            ///for drawing warp radiuses, but will take anything and might be extended later
            system_manage.add_selected_orbital(orb);

            if(o->has_quest_alert && o->viewed_by[player_empire])
            {
                game_event_manager* event = all_events.orbital_to_game_event(o);

                //ship_manager* nearest_fleet = event->get_nearest_fleet(player_empire);

                if(event != nullptr && event->interacting_faction != nullptr && event->interacting_faction != player_empire)
                {
                    ImGui::NeutralText("(Another empire has already claimed this event)");
                }
                else if(event != nullptr && !event->can_interact(player_empire))
                {
                    ImGui::NeutralText("(Requires nearby fleet to view alert)");

                    event->set_dialogue_state(false);
                }
                else
                {
                    if(o->dialogue_open)
                        ImGui::NeutralText("(Hide Alert)");
                    else
                        ImGui::NeutralText("(View Alert)");

                    if(ImGui::IsItemClicked_Registered())
                    {
                        if(event->interacting_faction == nullptr)
                        {
                            event->set_interacting_faction(player_empire);
                            ///set interacting fleet
                        }

                        event->toggle_dialogue_state();
                    }
                }
            }

            bool can_construct = true;

            for(orbital* test_o : o->parent_system->orbitals)
            {
                if(o->parent_empire != nullptr && o->parent_empire->is_hostile(test_o->parent_empire))
                {
                    can_construct = false;
                    break;
                }
            }

            if(o->type == orbital_info::PLANET && o->can_construct_ships && o->parent_empire == player_empire)
            {
                if(!can_construct)
                {
                    o->construction_ui_open = false;

                    ImGui::NeutralNoHoverText("(System Under Blockade)");
                }
                else
                {
                    if(o->construction_ui_open)
                        ImGui::NeutralText("(Hide Construction Window)");
                    else
                        ImGui::NeutralText("(Show Construction Window)");
                }

                if(ImGui::IsItemClicked_Registered() && can_construct)
                {
                    o->construction_ui_open = !o->construction_ui_open;
                }
            }

            if(o->type == orbital_info::FLEET && o->can_construct_ships && o->parent_empire == player_empire)
            {
                ///ships can always construct
                if(sm->any_with_element(ship_component_elements::SHIPYARD))
                {
                    if(o->construction_ui_open)
                        ImGui::NeutralText("(Hide Construction Window)");
                    else
                        ImGui::NeutralText("(Show Construction Window)");

                    if(ImGui::IsItemClicked_Registered())
                    {
                        o->construction_ui_open = !o->construction_ui_open;
                    }
                }
                else
                {
                    o->construction_ui_open = false;
                }
            }

            if(o->type == orbital_info::PLANET && o->decolonise_timer_s > 0.0001f)
            {
                std::string decolo_time = "Time until decolonised " + to_string_with_enforced_variable_dp(orbital_info::decolonise_time_s - o->decolonise_timer_s);

                ImGui::Text(decolo_time.c_str());
            }

            if(o->type == orbital_info::FLEET && o->parent_empire == player_empire)
            {
                std::vector<orbital_system*> systems{o->parent_system};

                auto warp_dest = o->command_queue.get_warp_destinations();

                for(auto& i : warp_dest)
                {
                    systems.push_back(i);
                }

                system_manage.add_draw_pathfinding(systems);
            }
        }

        ImGui::Unindent();
    }

    if(popup.elements.size() > 0 && popup.going)
    {
        if(system_manage.hovered_system != nullptr)
        {
            for(popup_element& elem : popup.elements)
            {
                orbital* o = (orbital*)elem.element;

                if(o->type != orbital_info::FLEET)
                    continue;

                ///This line of code must have been before parent sysetm right?
                //orbital_system* parent = system_manage.get_parent(o);
                orbital_system* parent = o->parent_system;

                if(parent == nullptr)
                    continue;

                if(o->parent_empire != player_empire)
                    continue;

                ship_manager* sm = (ship_manager*)o->data;

                auto warp_destinations = o->command_queue.get_warp_destinations();

                orbital_system* backup = o->parent_system;

                orbital_system* current = o->parent_system;

                if(warp_destinations.size() > 0 && key_down(sf::Keyboard::LShift))
                {
                    current = warp_destinations.back();
                }

                o->parent_system = current;

                auto path = system_manage.pathfind(o, system_manage.hovered_system);

                o->parent_system = backup;

                /*for(orbital_system* sys : path)
                {
                    std::string name = sys->get_base()->name;

                    tooltip::add(name);
                }*/

                if(parent != system_manage.hovered_system)
                {
                    if(path.size() == 0)
                    {
                        tooltip::add("No path to system");
                    }

                    if(path.size() > 0 && !sm->can_warp(system_manage.hovered_system, parent, o))
                    {
                        tooltip::add("Queue Warp");
                    }

                    if(sm->can_warp(system_manage.hovered_system, parent, o))
                    {
                        tooltip::add("Warp");
                    }
                }

                system_manage.add_draw_pathfinding(path);

                if(path.size() > 0)
                {
                    path.erase(path.begin());
                }

                if(rclick && sm->parent_empire == player_empire)
                {
                    bool skip = false;

                    if(!key_down(sf::Keyboard::LShift) && !system_manage.in_system_view())
                    {
                        o->command_queue.cancel();

                        ///this is where right click toggling warp destinations happens
                        if(warp_destinations.size() != 0 && warp_destinations.back() == system_manage.hovered_system)
                        {
                            skip = true;
                        }
                    }

                    if(!skip)
                    {
                        for(orbital_system* sys : path)
                            o->command_queue.try_warp(sys, true, lshift);
                    }
                }
            }
        }
    }


    ///remember we'll need to make an orbital associated with the new fleet
    ///going to need the ability to drag and drop these
    ///nah use checkboxes
    ///put element into a function, move up
    for(popup_element& i : popup.elements)
    {
        orbital* o = (orbital*)i.element;

        if(o->type == orbital_info::FLEET)
        {
            ship_manager* sm = (ship_manager*)o->data;

            for(ship* s : sm->ships)
            {
                if(s->shift_clicked)
                {
                    potential_new_fleet.insert(s);
                }
            }
        }
    }

    ///this is kind of useless now
    if(potential_new_fleet.size() > 0)
    {
        ImGui::NeutralText("(Make New Fleet)");

        if(ImGui::IsItemClicked_Registered())
        {
            bool bad = false;

            ship* test_ship = (*potential_new_fleet.begin());

            orbital_system* test_parent = system_manage.get_by_element(test_ship->owned_by);

            for(ship* i : potential_new_fleet)
            {
                if(system_manage.get_by_element(i->owned_by) != test_parent)
                    bad = true;
            }

            if(!bad)
            {
                ship_manager* ns = fleet_manage.make_new();

                vec2f fleet_pos;

                empire* parent = nullptr;

                for(ship* i : potential_new_fleet)
                {
                    ///we don't want to free associated orbital if it still exists from empire
                    ///only an issue if we cull, which is why ownership is culled there
                    orbital* real = test_parent->get_by_element((void*)i->owned_by);

                    if(real != nullptr)
                    {
                        parent = real->parent_empire;

                        fleet_pos = real->absolute_pos;
                    }

                    i->owned_by->toggle_fleet_ui = false;
                    i->shift_clicked = false;

                    ///if we're going to be removed from popup
                    if(real != nullptr && i->owned_by->ships.size() == 1)
                        popup.rem(real);

                    ns->steal(i);
                }

                orbital* associated = test_parent->make_new(orbital_info::FLEET, 5.f);
                associated->parent = test_parent->get_base();
                associated->set_orbit(fleet_pos);
                associated->data = ns;
                associated->tick(0.f);

                parent->take_ownership(associated);
                parent->take_ownership(ns);

                popup_element elem;
                elem.element = associated;

                popup.elements.push_back(elem);

                if(associated->type == orbital_info::FLEET)
                    ns->toggle_fleet_ui = true;
            }
            else
            {
                printf("Tried to create fleets cross systems\n");
            }
        }
    }

    if(global_drag_and_drop.currently_dragging == drag_and_drop_info::SHIP && global_drag_and_drop.let_go_outside_window())
    {
        ship* s = (ship*)global_drag_and_drop.data;

        ship_manager* sm = s->owned_by;

        orbital* real = system_manage.get_by_element_orbital(sm);

        orbital_system* test_parent = real->parent_system;

        empire* parent = sm->parent_empire;

        vec2f fleet_pos = real->absolute_pos;

        if(sm->ships.size() > 1 && real && parent)
        {
            ship_manager* ns = fleet_manage.make_new();

            ns->steal(s);

            orbital* associated = test_parent->make_new(orbital_info::FLEET, 5.f);
            associated->parent = test_parent->get_base();
            associated->set_orbit(fleet_pos);
            associated->data = ns;
            associated->tick(0.f);

            parent->take_ownership(associated);
            parent->take_ownership(ns);

            popup_element elem;
            elem.element = associated;

            popup.elements.push_back(elem);

            if(associated->type == orbital_info::FLEET)
                ns->toggle_fleet_ui = true;
        }
    }

    for(auto& i : steal_map)
    {
        for(auto& s : i.second)
        {
            i.first->steal(s);
        }
    }

    popup.remove_scheduled();

    ImGui::End();
}

struct construction_window_state
{
    research picked_research_levels;
};

///returns whether or not we've modified our system
bool do_construction_window(orbital* o, empire* player_empire, fleet_manager& fleet_manage, construction_window_state& window_state, ship_customiser& ship_customise)
{
    if(!o->construction_ui_open)
        return false;

    bool built = false;

    ImGui::BeginOverride(("Ship Construction (" + o->name + ")").c_str(), &o->construction_ui_open, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::clamp_window_to_screen();

    for(int i=0; i<window_state.picked_research_levels.categories.size(); i++)
    {
        research_category& category = window_state.picked_research_levels.categories[i];

        std::string text = format(research_info::names[i], research_info::names);

        ImGui::Text((text + " | ").c_str());

        ImGui::SameLine();

        ImGui::Text("(-)");

        if(ImGui::IsItemClicked_Registered())
        {
            category.amount -= 1.f;

            category.amount = clamp(category.amount, 0.f, player_empire->research_tech_level.categories[i].amount);

            category.amount = floor(category.amount);
        }

        ImGui::SameLine();

        std::string dstr = std::to_string((int)category.amount);

        ImGui::Text(dstr.c_str());

        ImGui::SameLine();

        ImGui::Text("(+)");

        if(ImGui::IsItemClicked_Registered())
        {
            category.amount += 1.f;

            category.amount = clamp(category.amount, 0.f, player_empire->research_tech_level.categories[i].amount);

            category.amount = floor(category.amount);
        }
    }

    ImGui::NewLine();

    /*std::vector<ship> ships
    {
        make_default(),
        make_scout(),
        make_colony_ship(),
        //ship_customise.current,
    };*/

    std::vector<ship> ships;

    for(ship& s : ship_customise.saved)
    {
        if(!s.is_ship_design_valid())
            continue;

        ships.push_back(s);
    }

    for(auto& test_ship : ships)
    {
        test_ship.set_tech_level_from_research(window_state.picked_research_levels);
        test_ship.empty_resources();

        auto cost = test_ship.resources_cost();

        bool can_dispense = player_empire->can_fully_dispense(cost);

        if(o->type == orbital_info::FLEET)
        {
            ship_manager* sm = (ship_manager*)o->data;

            can_dispense = sm->can_fully_dispense(cost);
        }

        resource_manager rm;
        rm.add(cost);

        std::string str = test_ship.name;

        std::string make_ship_full_name = ("(Make " + str + " Ship)");

        if(can_dispense)
            ImGui::NeutralText(make_ship_full_name.c_str());
        else
            ImGui::BadText(make_ship_full_name.c_str());

        bool clicked = ImGui::IsItemClicked_Registered();

        rm.render_formatted_str(true);
        ImGui::NewLine();

        if(clicked)
        {
            if(can_dispense)
            {
                if(o->type == orbital_info::FLEET)
                {
                    ship_manager* sm = (ship_manager*)o->data;
                    sm->fully_dispense(cost);
                }
                else
                {
                    player_empire->dispense_resources(cost);
                }

                orbital_system* os = o->parent_system;

                ship_manager* new_fleet = fleet_manage.make_new();

                ship* new_ship = new_fleet->make_new_from(player_empire, test_ship.duplicate());
                //new_ship->name = "SS Toimplement name generation";

                orbital* onew_fleet = os->make_new(orbital_info::FLEET, 5.f);
                onew_fleet->orbital_angle = o->orbital_angle;
                onew_fleet->orbital_length = o->orbital_length + 40;
                onew_fleet->parent = os->get_base(); ///?
                onew_fleet->data = new_fleet;

                player_empire->take_ownership(onew_fleet);
                player_empire->take_ownership(new_fleet);

                new_ship->set_tech_level_from_research(window_state.picked_research_levels);

                //new_ship->set_tech_level_from_empire(player_empire);

                onew_fleet->tick(0.f);

                built = true;
            }
        }
    }

    ImGui::End();

    return built;
}

void do_ownership_ui(empire_manager& empire_manage, network_state& net_state)
{
    for(empire* e : empire_manage.empires)
    {
        if(net_state.owns(e))
            return;
    }

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiSetCond_Appearing);

    ImGui::BeginOverride("Pick an Empire", 0, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::clamp_window_to_screen();

    for(empire* e : empire_manage.empires)
    {
        if(e->is_claimed)
            continue;

        ImGui::NeutralText(e->name);

        if(ImGui::IsItemClicked_Registered())
        {
            e->try_network_take_ownership(net_state);
        }
    }

    ImGui::End();
}

void handle_camera(sf::RenderWindow& window, system_manager& system_manage, all_battles_manager& all_battles, bool in_battle)
{
    sf::View view = window.getDefaultView();

    if(!in_battle)
    {
        view.setSize(window.getSize().x * system_manage.zoom_handle.get_zoom(), window.getSize().y * system_manage.zoom_handle.get_zoom());
    }
    else
    {
        view.setSize(window.getSize().x * all_battles.zoom_handle.get_zoom(), window.getSize().y * all_battles.zoom_handle.get_zoom());
    }

    //view.zoom(system_manage.zoom_level);

    if(!in_battle)
    {
        if(system_manage.in_system_view())
        {
            view.setCenter({system_manage.system_cam.pos.x(), system_manage.system_cam.pos.y()});
        }
        else
        {
            view.setCenter({system_manage.universe_cam.pos.x(), system_manage.universe_cam.pos.y()});
        }
    }
    else
    {
        view.setCenter({all_battles.battle_cam.pos.x(), all_battles.battle_cam.pos.y()});
    }
    //view.setCenter({system_manage.camera.x(), system_manage.camera.y()});

    window.setView(view);
}

int main()
{
    //register_signal_handlers();
    //check_stacktrace();

    ship_component_elements::generate_element_infos();

    full_component_list = get_component_list();

    default_ships_list = get_default_ships_list();

    test_serialisation();

    srand(102);

    randf_s();

    top_bar::active[top_bar_info::ECONOMY] = true;
    top_bar::active[top_bar_info::RESEARCH] = true;
    top_bar::active[top_bar_info::BATTLES] = true;
    top_bar::active[top_bar_info::FLEETS] = true;

    //music playing_music;

    empire_manager empire_manage;

    empire* player_empire = empire_manage.make_new();
    player_empire->is_player = true;
    player_empire->is_claimed = true;
    player_empire->name = "Glorious Azerbaijanian Conglomerate";
    player_empire->ship_prefix = "SS";

    player_empire->resources.resources[resource::IRON].amount = 5000.f;
    player_empire->resources.resources[resource::COPPER].amount = 5000.f;
    player_empire->resources.resources[resource::TITANIUM].amount = 5000.f;
    player_empire->resources.resources[resource::URANIUM].amount = 5000.f;
    player_empire->resources.resources[resource::HYDROGEN].amount = 8000.f;
    //player_empire->resources.resources[resource::OXYGEN].amount = 8000.f;

    //player_empire->resources.resources[resource::RESEARCH].amount = 8000.f;

    fleet_manager fleet_manage;

    //#define ONLY_PLAYER
    #ifndef ONLY_PLAYER

    empire* hostile_empire = empire_manage.make_new();
    hostile_empire->name = "Irate Uzbekiztaniaite Spacewombles";
    hostile_empire->has_ai = true;
    //hostile_empire->has_ai = false;

    hostile_empire->research_tech_level.categories[research_info::MATERIALS].amount = 3.f;
    hostile_empire->trade_space_access(player_empire, true);


    hostile_empire->resources.resources[resource::IRON].amount = 500.f;
    hostile_empire->resources.resources[resource::COPPER].amount = 500.f;
    hostile_empire->resources.resources[resource::TITANIUM].amount = 500.f;
    hostile_empire->resources.resources[resource::URANIUM].amount = 500.f;
    hostile_empire->resources.resources[resource::RESEARCH].amount = 800.f;
    hostile_empire->resources.resources[resource::HYDROGEN].amount = 800.f;
    //hostile_empire->resources.resources[resource::OXYGEN].amount = 800.f;

    //player_empire->ally(hostile_empire);
    //player_empire->become_hostile(hostile_empire);

    empire* derelict_empire = empire_manage.make_new();
    derelict_empire->name = "Test Ancient Faction";
    derelict_empire->is_derelict = true;

    ///manages FLEETS, not SHIPS
    ///this is fine. This is a global thing, the highest level of storage for FLEETS of ships
    ///FLEEEEETS
    ///Should have named this something better

    ship_manager* fleet1 = fleet_manage.make_new();
    ship_manager* fleet2 = fleet_manage.make_new();
    ship_manager* fleet3 = fleet_manage.make_new();
    ship_manager* fleet4 = fleet_manage.make_new();

    //ship test_ship = make_default();
    //ship test_ship2 = make_default();

    ship* test_ship = fleet1->make_new_from(player_empire, make_default());
    ship* test_ship3 = fleet1->make_new_from(player_empire, make_default());

    ship* test_ship2 = fleet2->make_new_from(hostile_empire, make_default());

    test_ship2->set_tech_level_from_empire(hostile_empire);
    //test_ship2->add(make_default_stealth());

    ship* test_ship4 = fleet3->make_new_from(player_empire, make_default());

    ship* derelict_ship = fleet4->make_new_from(hostile_empire, make_default());

    ship* scout_ship = fleet3->make_new_from(player_empire, make_mining_ship());

    /*test_ship->name = "SS Icarus";
    test_ship2->name = "SS Buttz";
    test_ship3->name = "SS Duplicate";
    test_ship4->name = "SS Secondary";

    derelict_ship->name = "SS Dereliction";

    scout_ship->name = "SS Scout";*/
    //scout_ship2->name = "SS Scout2";

    #endif

    /*test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);*/

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window;

    window.create(sf::VideoMode(1500, 900),"Wowee", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(false);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildWindowRounding = 2;

    sf::Clock clk;

    float last_time_s = 0.f;
    float diff_s = 0.f;

    all_battles_manager* all_battles = new all_battles_manager;

    //battle_manager* battle = all_battles.make_new();

    //battle->add_ship(test_ship);
    //battle->add_ship(test_ship2);

    system_manager system_manage;

    orbital_system* sys_2 = system_manage.make_new();
    sys_2->generate_full_random_system(false);
    sys_2->universe_pos = {10, 10};

    orbital_system* sys_3 = system_manage.make_new();
    sys_3->generate_full_random_system(false);
    sys_3->universe_pos = {10, 40};

    orbital_system* base = system_manage.make_new();

    orbital* sun = base->make_new(orbital_info::STAR, 10.f, 10);
    orbital* planet = base->make_new(orbital_info::PLANET, 5.f, 10);
    planet->parent = sun;

    sun->rotation_velocity_ps = 2*M_PI/60.f;

    planet->orbital_length = 150.f;
    planet->rotation_velocity_ps = 2*M_PI / 200.f;

    #ifndef ONLY_PLAYER
    orbital* ofleet = base->make_new(orbital_info::FLEET, 5.f);

    ofleet->orbital_angle = M_PI/13.f;
    ofleet->orbital_length = 200.f;
    ofleet->parent = sun;
    ofleet->data = fleet1;

    orbital* ofleet2 = base->make_new(orbital_info::FLEET, 5.f);

    ofleet2->orbital_angle = randf_s(0.f, 2*M_PI);
    ofleet2->orbital_length = randf_s(50.f, 300.f);
    ofleet2->parent = sun;
    ofleet2->data = fleet3;
    #endif

    orbital* tplanet = base->make_new(orbital_info::PLANET, 3.f);
    tplanet->orbital_length = 50.f;
    tplanet->parent = sun;

    base->generate_asteroids_old(500, 5, 5);
    base->calculate_radius();
    //base->generate_planet_resources(2.f);


    /*orbital* oscout = base->make_new(orbital_info::FLEET, 5.f);
    oscout->orbital_angle = 0.f;
    oscout->orbital_length = 270;
    oscout->parent = sun;
    oscout->data = fleet5;*/


    player_empire->take_ownership_of_all(base);
    #ifndef ONLY_PLAYER
    player_empire->take_ownership(fleet1);
    player_empire->take_ownership(fleet3);

    player_empire->take_ownership(ofleet);
    player_empire->take_ownership(ofleet2);
    #endif
    //player_empire->take_ownership(fleet5);

    #ifndef ONLY_PLAYER
    orbital* ohostile_fleet = base->make_new(orbital_info::FLEET, 5.f);
    ohostile_fleet->orbital_angle = 0.f;
    ohostile_fleet->orbital_length = 250;
    ohostile_fleet->parent = sun;
    ohostile_fleet->data = fleet2;

    orbital* oderelict_fleet = base->make_new(orbital_info::FLEET, 5.f);
    oderelict_fleet->orbital_angle = M_PI/16.f;
    oderelict_fleet->orbital_length = 200;
    oderelict_fleet->parent = sun;
    oderelict_fleet->data = fleet4;

    ///this ownership stuff is real dodgy
    hostile_empire->take_ownership(fleet2);
    hostile_empire->take_ownership(ohostile_fleet);

    hostile_empire->take_ownership(fleet4);
    hostile_empire->take_ownership(oderelict_fleet);

    derelict_ship->randomise_make_derelict();
    #endif


    //orbital_system* sys_2 = system_manage.make_new();
    //sys_2->generate_random_system(3, 100, 3, 5);

    //#ifndef ONLY_PLAYER
    //#define NO_GALAXY
    #ifndef NO_GALAXY

    empire* e2 = empire_manage.birth_empire(system_manage, fleet_manage, sys_2);
    e2->take_ownership_of_all(sys_3);

    e2->resources.resources[resource::IRON].amount = 5000.f;
    e2->resources.resources[resource::COPPER].amount = 5000.f;
    e2->resources.resources[resource::TITANIUM].amount = 5000.f;
    e2->resources.resources[resource::URANIUM].amount = 5000.f;
    e2->resources.resources[resource::RESEARCH].amount = 8000.f;
    e2->resources.resources[resource::HYDROGEN].amount = 8000.f;
    //e2->resources.resources[resource::OXYGEN].amount = 8000.f;

    //hostile_empire->unally(e2);

    ship_manager* fleet5 = fleet_manage.make_new();

    fleet5->make_new_from(e2, make_default());
    fleet5->make_new_from(e2, make_default());
    fleet5->make_new_from(e2, make_default());

    orbital* op3 = sys_3->make_new(orbital_info::FLEET, 5.f);
    op3->orbital_angle = 0.f;
    op3->orbital_length = 250;
    op3->parent = sys_3->get_base();
    op3->data = fleet5;

    e2->take_ownership(op3);
    e2->take_ownership(fleet5);

    e2->desired_empire_size = 5;
    e2->ally(player_empire);

    //empire* e2 = empire_manage.birth_empire_without_system_ownership(fleet_manage, sys_2, 2, 2);f

    //player_empire->become_hostile(e2);
    //player_empire->ally(e2);
    player_empire->positive_relations(e2, 0.5f);

    system_manage.generate_universe(100);

    empire_manage.birth_empires_random(fleet_manage, system_manage, 0.6f);
    empire_manage.birth_empires_without_ownership(fleet_manage, system_manage);
    #endif

    system_manage.set_viewed_system(base);

    construction_window_state window_state;

    popup_info popup;

    all_events_manager all_events;

    #ifndef ONLY_PLAYER
    game_event_manager* test_event = all_events.make_new(game_event_info::ANCIENT_PRECURSOR, tplanet, fleet_manage, system_manage);

    test_event->set_faction(derelict_empire);

    game_event_manager* test_event2 = all_events.make_new(game_event_info::LONE_DERELICT, sun, fleet_manage, system_manage);

    test_event2->set_faction(derelict_empire);
    #endif

    ///If empire colours are getting messed up on loading from a file (eventually) itll be this
    empire_manage.assign_colours_non_randomly();


    ship_customiser ship_customise;

    sf::Keyboard key;
    sf::Mouse mouse;

    box_selection box_selector;

    int state = 0;

    bool focused = true;

    bool fullscreen = false;

    vec2f mouse_last = {0,0};
    vec2f mpos = {0,0};

    /*for(int i=0; i<4; i++)
    {
        orbital* test_o = sys_3->make_fleet(fleet_manage, 100.f, 5.f, e2);

        ship_manager* sm = (ship_manager*)test_o->data;

        ship* s = sm->make_new_from(e2, make_mining_ship().duplicate());
    }*/

    /*for(orbital_system* sys : system_manage.systems)
    {
        sys->get_base()->release_ownership();

        for(orbital* o : sys->orbitals)
        {
            if(o->type == orbital_info::PLANET)
            {
                o->release_ownership();
            }
        }
    }*/

    //#define VIEW_ALL
    #ifdef VIEW_ALL
    for(orbital_system* sys : system_manage.systems)
    {
        sys->get_base()->viewed_by[player_empire] = true;
    }
    #endif // VIEW_ALL

    sf::Image img;
    img.loadFromFile("pics/particle_base.png");

    tonemap(img);

    sf::Texture tex;

    tex.loadFromImage(img);
    tex.setSmooth(true);

    networking_init();
    network_state net_state;

    network_updater net_update;

    ImGui::set_screen_dimensions({window.getSize().x, window.getSize().y});

    //empire_manage.explicit_register();
    //fleet_manage.explicit_register();
    //system_manage.explicit_register();
    //all_battles->explicit_register();

    bool mouse_locked = false;
    vec2f lock_mouse_pos;

    while(window.isOpen())
    {
        /*playing_music.tick(diff_s);

        if(once<sf::Keyboard::Add>())
        {
            playing_music.tg.make_cell_random();
        }*/
        sf::Event event;

        bool no_suppress_mouse = !ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow();

        float scrollwheel_delta = 0;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

            if(event.type == sf::Event::Closed)
                window.close();

            if(event.type == sf::Event::GainedFocus)
                focused = true;

            if(event.type == sf::Event::LostFocus)
                focused = false;

            ///Ok. Changing this to accept only one scrollwheel event because
            ///multiple in one frame can cause issues
            if(event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                scrollwheel_delta += event.mouseWheelScroll.delta;

            if(event.type == sf::Event::Resized)
            {
                int x = event.size.width;
                int y = event.size.height;

                window.create(sf::VideoMode(x, y), "Wowee", sf::Style::Default, settings);

                /*window.setSize({x, y});

                auto view = window.getView();

                view = sf::View(sf::FloatRect(0.f, 0.f, window.getSize().x, window.getSize().y));

                window.setView(view);*/
            }
        }

        mouse_last = mpos;
        mpos = {mouse.getPosition(window).x, mouse.getPosition(window).y};

        vec2f real_mpos = {mouse.getPosition().x, mouse.getPosition().y};

        if(scrollwheel_delta < 0)
            scrollwheel_delta = -1;
        if(scrollwheel_delta > 0)
            scrollwheel_delta = 1;

        if(focused && key_down(sf::Keyboard::LAlt) && ONCE_MACRO(sf::Keyboard::Return))
        {
            if(!fullscreen)
            {
                window.create(sf::VideoMode().getFullscreenModes()[0], "Wowee", sf::Style::Fullscreen, settings);
                fullscreen = true;
            }
            else
            {
                window.create(sf::VideoMode().getFullscreenModes()[0], "Wowee", sf::Style::Default, settings);
                fullscreen = false;
            }
        }

        ImGui::set_screen_dimensions({window.getSize().x, window.getSize().y});

        if(fullscreen)
        {
            auto win_pos = window.getPosition();
            auto win_dim = window.getSize();
            RECT r;
            r.left = win_pos.x;
            r.top = win_pos.y;
            r.right = win_pos.x + win_dim.x;
            r.bottom = win_pos.y + win_dim.y;

            ClipCursor(&r);
        }

        #ifdef DEV_ONLY
        if(focused && key_down(sf::Keyboard::F10))
        {
            window.close();
        }
        #endif // DEV_ONLY

        vec2f mouse_dir = mpos - mouse_last;

        vec2f cdir = {0,0};
        vec2f mdir = {0,0};

        if(key_down(sf::Keyboard::W))
            cdir.y() += 1;

        if(key_down(sf::Keyboard::S))
            cdir.y() -= 1;

        if(key_down(sf::Keyboard::A))
            cdir.x() += 1;

        if(key_down(sf::Keyboard::D))
            cdir.x() -= 1;
        if(key_down(sf::Keyboard::LShift))
            cdir = cdir * 5.f;

        if(ship_customise.text_input_going)
            cdir = 0.f;

        if(mouse.isButtonPressed(sf::Mouse::Middle) && focused)
        {
            mdir += mouse_dir;
        }

        if(!focused)
            cdir = 0.f;

        all_battles->tick(diff_s, system_manage, net_state, player_empire);

        if(ONCE_MACRO(sf::Keyboard::M) && focused && !ship_customise.text_input_going)
        {
            system_manage.enter_universe_view();
        }

        ///this is... well, quite a bit of a hack
        ///only context can suppress keyboard at the moment
        ImGui::suppress_keyboard = false;


        ///this hack is very temporary, after this make it so that the backup system is the
        ///system in which the battle takes place that we're viewing
        ///Ok so. If any system sets currently viewed to not be nullptr, but we're in the combat state
        ///it means terminate the combat state. This is actually pretty neat, and not too much of a hack
        ///backup system still feels pretty undesirable though
        if(system_manage.currently_viewed != nullptr)
        {
            system_manage.backup_system = system_manage.currently_viewed;

            if(state == 1)
            {
                all_battles->request_leave_battle_view = true;
            }
        }

        //if(ONCE_MACRO(sf::Keyboard::F1))

        if(all_battles->request_leave_battle_view || all_battles->request_enter_battle_view)
        {
            //state = (state + 1) % 2;

            if(all_battles->request_leave_battle_view)
            {
                state = 0;
            }

            if(all_battles->request_enter_battle_view)
            {
                state = 1;
            }

            if(all_battles->request_stay_in_battle_system && state == 0)
            {
                if(all_battles->request_stay_system != nullptr)
                {
                    system_manage.set_viewed_system(all_battles->request_stay_system);
                }
            }

            all_battles->request_enter_battle_view = false;
            all_battles->request_leave_battle_view = false;

            if(state == 0 && !all_battles->request_stay_in_battle_system)
            {
                orbital_system* found_system = system_manage.backup_system;

                #ifdef JUMP_TO_SYSTEM_AFTER_VIEWING_BATTLE
                if(all_battles->currently_viewing != nullptr)
                {
                    found_system = all_battles->currently_viewing->get_system_in();
                }
                #endif // JUMP_TO_SYSTEM_AFTER_VIEWING_BATTLE

                system_manage.set_viewed_system(found_system);
            }
            if(state == 1)
            {
                ///need way to view any battle
                //battle->set_view(system_manage);
                if(all_battles->get_currently_viewing() != nullptr)
                    all_battles->get_currently_viewing()->set_view(system_manage);

                ///for the moment, reset zoom
                all_battles->zoom_handle.set_zoom(1.f);

                system_manage.set_viewed_system(nullptr);
            }

            all_battles->request_stay_in_battle_system = false;
        }

        vec2f pan_dir = cdir * diff_s * 300 + mdir;

        if(state == 0)
            system_manage.pan_camera(pan_dir);
        if(state == 1)
            all_battles->battle_cam.pos += -pan_dir * all_battles->zoom_handle.get_zoom();

        if(state == 0)
        {
            if(no_suppress_mouse)
                system_manage.change_zoom(-scrollwheel_delta, mpos, window);
        }
        if(state == 1)
        {
            if(no_suppress_mouse)
            {
                all_battles->zoom_handle.offset_zoom(-scrollwheel_delta, window, mpos);

                //system_manage.change_zoom(-scrollwheel_delta, mpos, window);

                #ifdef ZOOM_OUT_OF_BATTLE
                ///need to set system camera to be location of battle
                if(all_battles->zoom_handle.get_zoom() > 20)
                {
                    all_battles->request_leave_battle_view = true;
                    all_battles->request_stay_in_battle_system = true;
                    all_battles->request_stay_system = nullptr;

                    system_manage.set_zoom(pow(sqrtf(2), -4));

                    if(all_battles->zoom_handle.current_time < all_battles->zoom_handle.destination_time)
                    {
                        system_manage.change_zoom(all_battles->zoom_handle.destination_zoom_level - all_battles->zoom_handle.zoom_level, mpos, window);
                        system_manage.zoom_handle.destination_zoom_level = round(system_manage.zoom_handle.destination_zoom_level);

                        system_manage.zoom_handle.current_time = all_battles->zoom_handle.current_time;
                        system_manage.zoom_handle.destination_time = all_battles->zoom_handle.destination_time;
                    }

                    if(all_battles->get_currently_viewing() != nullptr)
                    {
                        battle_manager* bm = all_battles->get_currently_viewing();

                        system_manage.universe_cam.pos = bm->get_avg_centre_global() + bm->get_system_in()->universe_pos * system_manage.universe_scale;
                        system_manage.system_cam.pos = bm->get_avg_centre_global();
                    }
                }
                #endif
            }
        }

        system_manage.tick_camera(diff_s);
        all_battles->zoom_handle.tick(diff_s);

        all_battles->battle_cam.pos += -all_battles->zoom_handle.get_camera_offset();

        handle_camera(window, system_manage, *all_battles, state == 1);


        bool lclick = ONCE_MACRO(sf::Mouse::Left) && no_suppress_mouse;
        bool rclick = ONCE_MACRO(sf::Mouse::Right) && no_suppress_mouse;

        /*if(lclick)
        {
            context_menu::set_item(nullptr);
        }*/

        //display_ship_info(*test_ship);

        //debug_menu({test_ship});

        //handle_camera(window, system_manage);

        debug_all_battles(*all_battles, window, lclick, system_manage, player_empire, state == 1);

        if(state == 1)
        {
            //debug_battle(*battle, window, lclick);


            all_battles->draw_viewing(window, system_manage);

            //battle->draw(window);
        }

        system_manage.tick(window, diff_s);

        for(orbital_system* os : system_manage.systems)
        {
            for(orbital* o : os->orbitals)
            {
                o->check_and_open_popup(popup);
            }
        }

        bool any_open = false;

        for(orbital_system* os : system_manage.systems)
        {
            for(orbital* o : os->orbitals)
            {
                if(o->type != orbital_info::FLEET)
                    continue;

                for(ship* s : o->data->ships)
                {
                    if(s->context_is_open)
                    {
                        any_open = true;
                        break;
                    }
                }

                if(o->data->context_is_open)
                {
                    any_open = true;
                    break;
                }

                if(any_open)
                    break;
            }
        }

        for(orbital_system* os : system_manage.systems)
        {
            for(orbital* o : os->orbitals)
            {
                if(o->type != orbital_info::FLEET)
                    continue;

                for(ship* s : o->data->ships)
                {
                    if(any_open)
                    {
                        s->context_request_open = false;
                    }

                    s->context_handle_menu(o, player_empire, fleet_manage, popup);
                }

                if(any_open)
                {
                    o->data->context_request_open = false;
                }

                o->data->context_handle_menu(o, player_empire, fleet_manage, popup);
            }
        }

        if(state == 0)
        {
            ///must happen before drawing system
            debug_system(system_manage, window, lclick, rclick, popup, player_empire, *all_battles, fleet_manage, all_events);
        }

        do_popup(popup, window, fleet_manage, system_manage, system_manage.currently_viewed, empire_manage, player_empire, all_events, *all_battles, rclick);

        if(state == 0)
        {
            ///this is slow
            ///dis kinda slow too
            system_manage.draw_universe_map(window, player_empire, popup);
            system_manage.draw_viewed_system(window, player_empire);
            system_manage.process_universe_map(window, lclick, player_empire);
            system_manage.draw_warp_radiuses(window, player_empire);
        }

        //if(system_manage.in_system_view())
            box_selector.tick(system_manage, system_manage.currently_viewed, window, popup, player_empire);

        //printf("ui\n");

        /*for(ship_manager* smanage : fleet_manage.fleets)
        {
            for(ship* s : smanage->ships)
            {
                if(s->display_ui)
                {
                    display_ship_info(*s, smanage->parent_empire, player_empire, player_empire, system_manage, fleet_manage, empire_manage, popup);
                }
            }
        }*/

        for(orbital_system* os : system_manage.systems)
        {
            for(orbital* o : os->orbitals)
            {
                if(o->type != orbital_info::FLEET)
                    continue;

                ship_manager* sm = o->data;

                for(ship* s : o->data->ships)
                {
                    if(s->display_ui)
                    {
                        display_ship_info(*s, sm->parent_empire, player_empire, player_empire, system_manage, fleet_manage, empire_manage, popup, o);
                    }
                }
            }
        }

        for(orbital_system* os : system_manage.systems)
        {
            for(int i=0; i<os->orbitals.size(); i++)
            {
                orbital* o = os->orbitals[i];

                if(o->parent_empire != player_empire)
                    continue;

                bool built = do_construction_window(o, player_empire, fleet_manage, window_state, ship_customise);

                //if(built)
                //    break;
            }
        }

        ImGui::BeginOverride("Test");

        ImGui::clamp_window_to_screen();

        /*if(ImGui::Button("Save"))
        {
            serialise ser;

            ser.handle_serialise(fleet_manage, ser);

            ser.save
        }*/


        auto handle_unprocessed = [&](){

            for(auto& unprocessed : serialise_data_helper::type_to_datas)
            {
                const size_t type = unprocessed.first;
                unhandled_types& objects = unprocessed.second;

                if(typeid(ship_manager*).hash_code() == type)
                {
                    //std::cout << "found orphaned ship" << std::endl;

                    for(serialisable* obj : objects.data)
                    {
                        ship_manager* sm = (ship_manager*)obj;

                        if(!fleet_manage.owns(sm))
                        {
                            fleet_manage.fleets.push_back(sm);
                        }

                        sm->handled_by_client = true;
                    }
                }
                else if(typeid(empire*).hash_code() == type)
                {
                    std::cout << "potential orphaned empire" << std::endl;

                    ///hmm
                    for(serialisable* obj : objects.data)
                    {
                        empire* e = (empire*)obj;

                        e->handled_by_client = true;

                        std::cout << e->name << std::endl;
                    }
                }
                else if(typeid(battle_manager*).hash_code() == type)
                {
                    //std::cout << "got battle" << std::endl;

                    for(serialisable* obj : objects.data)
                    {
                        all_battles->battles.insert((battle_manager*)obj);

                        //std::cout << "got battle\n";

                        obj->handled_by_client = true;
                    }
                }
            }

            for(auto& unprocessed : serialise_data_helper::type_to_datas)
            {
                const size_t type = unprocessed.first;
                unhandled_types& objects = unprocessed.second;

                if(typeid(projectile*).hash_code() == type)
                {
                    for(serialisable* obj : objects.data)
                    {
                        projectile* proj = (projectile*)obj;

                        //std::cout << proj << std::endl;
                        //std::cout << proj->owned_by << std::endl;

                        //std::cout << "bad_proj" << std::endl;

                        if(proj->owned_by == nullptr)
                        {
                            std::cout << "super bad proj" << std::endl;

                            //serialise_data_helper::host_to_id_to_pointer[proj->host_id][proj->serialise_id] = nullptr;
                            //delete proj;
                            continue;
                        }

                        proj->owned_by->projectile_manage.projectiles.insert(proj);

                        obj->handled_by_client = true;
                    }
                }

            }

            for(auto& unprocessed : serialise_data_helper::type_to_datas)
            {
                const size_t type = unprocessed.first;
                unhandled_types& objects = unprocessed.second;

                for(serialisable* obj : objects.data)
                {
                    if(!obj->handled_by_client)
                        std::cout << "Warning unhandled type " << objects.type_name << std::endl;
                }
            }

            fleet_manage.shuffle_networked_ships();

            all_battles->remove_bad_orbitals();
            popup.remove_scheduled();
            system_manage.destroy_cleanup(empire_manage);
            fleet_manage.destroy_cleanup(empire_manage);

            serialise_data_helper::type_to_datas.clear();
        };

        if(ImGui::Button("Save"))
        {
            serialise_data_helper::host_to_id_to_pointer.clear();

            serialise_data_helper::ref_mode = 1;
            serialise_data_helper::send_mode = 1;

            serialise ser;
            ser.default_owner = net_state.my_id;

            ser.handle_serialise(state, true);
            ser.handle_serialise(empire_manage, true);
            ser.handle_serialise(system_manage, true);
            ser.handle_serialise(fleet_manage, true);
            ser.handle_serialise(all_battles, true);

            ser.save("Game.save");
        }

        if(ImGui::Button("Load"))
        {
            /*serialise ser;

            ser.load("Test.txt");

            orbital* next_o;

            ser.handle_serialise(next_o, false);

            fleet_manage.fleets.push_back(next_o->data);

            next_o->parent = base->get_base();
            base->make_in_place(next_o);

            player_empire->take_ownership(next_o);
            player_empire->take_ownership(next_o->data);
            next_o->command_queue.cancel();*/

            ///:(
            popup.clear();

            serialise ser;
            ser.load("Game.save");

            empire_manage.erase_all();
            system_manage.erase_all();

            fleet_manage.erase_all();

            all_battles->erase_all();


            empire_manage = empire_manager();
            system_manage = system_manager();
            fleet_manage = fleet_manager();
            delete all_battles;
            all_battles = nullptr;
            //all_battles = new all_battles_manager();

            serialise_data_helper::ref_mode = 1;
            serialise_data_helper::send_mode = 1;

            serialise_data_helper::host_to_id_to_pointer.clear();
            serialise_data_helper::type_to_datas.clear();

            ser.handle_serialise(state, false);
            ser.handle_serialise(empire_manage, false);
            ser.handle_serialise(system_manage, false);
            ser.handle_serialise(fleet_manage, false);
            ser.handle_serialise(all_battles, false);

            /*for(ship_manager* sm : fleet_manage.fleets)
            {
                std::cout << "a " << sm << std::endl;

                for(ship* s : sm->ships)
                {
                    std::cout << s << std::endl;
                }
            }*/

            serialise_host_type host_id = -1;

            for(empire* e : empire_manage.empires)
            {
                e->network_take_ownership(net_state, net_state.my_id, false);
            }

            for(empire* e : empire_manage.empires)
            {
                if(e->is_player)
                {
                    player_empire = e;
                    e->is_claimed = true;

                    host_id = e->host_id;

                    break;
                }
            }

            std::cout << "HI THERE " << player_empire->name << std::endl;

            player_empire->network_take_ownership(net_state, net_state.my_id);

            serialise_data_helper::type_to_datas.clear();

            //system_manage.ensure_found_orbitals_handled();

            //serialise_data_helper::host_to_id_to_pointer.clear();
        }


        ImGui::End();

        if(net_state.connected())
        {
            for(network_data& i : net_state.available_data)
            {
                if(i.processed)
                    continue;

                int32_t internal_counter = i.data.internal_counter;

                int32_t send_mode = 0;

                i.data.handle_serialise(send_mode, false);

                if(send_mode != 1)
                {
                    i.data.internal_counter = internal_counter;

                    continue;
                }

                serialise_data_helper::host_to_id_to_pointer.clear();

                serialise_data_helper::send_mode = 1;
                serialise_data_helper::ref_mode = 1;

                std::cout << "got full gamestate" << std::endl;

                popup.clear();

                empire_manage.erase_all();
                system_manage.erase_all();

                fleet_manage.erase_all();

                all_battles->erase_all();

                serialise ser = i.data;

                ser.handle_serialise(empire_manage, false);
                ser.handle_serialise(system_manage, false);
                ser.handle_serialise(fleet_manage, false);
                ser.handle_serialise(all_battles, false);

                for(empire* e : empire_manage.empires)
                {
                    if(e->is_player)
                    {
                        player_empire = e;

                        break;
                    }
                }

                //system_manage.ensure_found_orbitals_handled();

                for(orbital_system* sys : system_manage.systems)
                {
                    for(orbital* o : sys->orbitals)
                    {
                        if(!o->owned_by_host)
                        {
                            o->command_queue.cancel();
                        }
                    }
                }

                i.set_complete();

                handle_unprocessed();
            }

            /*for(network_data& i : net_state.available_data)
            {
                if(i.processed)
                    continue;

                int32_t internal_counter = i.data.internal_counter;

                int32_t disk_mode = 0;

                i.data.handle_serialise(disk_mode, false);

                if(disk_mode != 2)
                {
                    i.data.internal_counter = internal_counter;

                    continue;
                }

                serialise_data_helper::disk_mode = 0;
                serialise_data_helper::send_mode = 0;

                serialise ser = i.data;

                ser.handle_serialise(empire_manage, false);
                ser.handle_serialise(system_manage, false);
                ser.handle_serialise(fleet_manage, false);
                ser.handle_serialise(all_battles, false);

                i.set_complete();
            }*/

            for(network_data& i : net_state.available_data)
            {
                if(i.processed)
                    continue;

                int32_t internal_counter = i.data.internal_counter;

                int32_t send_mode = 0;

                i.data.handle_serialise(send_mode, false);

                if(send_mode == 1)
                {
                    i.data.internal_counter = internal_counter;

                    continue;
                }

                serialise_host_type host_id;

                i.data.handle_serialise(host_id, false);

                //std::cout << "got mini packet" << std::endl;

                serialise_data_helper::send_mode = send_mode;
                serialise_data_helper::ref_mode = 0;

                /*if(send_mode == 2)
                {
                    printf("hi\n");
                }*/

                /*for(orbital_system* sys : system_manage.systems)
                {
                    for(orbital* o : sys->orbitals)
                    {
                        i.data.handle_serialise(serialise_data_helper::disk_mode, false);
                    }
                }*/

                //std::cout << i.object.owner_id << " " << i.object.serialise_id << std::endl;

                serialisable* found_s = net_state.get_serialisable(host_id, i.object.serialise_id);

                /*for(auto& i : serialise_data_helper::owner_to_id_to_pointer[i.object.owner_id])
                {
                    std::cout << i.first << std::endl;
                    std::cout << i.second << std::endl;
                }*/

                //if(send_mode == 2)
                {
                    //std::cout << "yay" << send_mode << std::endl;
                }


                if(found_s == nullptr)
                {
                    //std::cout << "nullptr" << std::endl;

                    i.set_complete();
                    continue;
                }
                //std::cout << "got" << std::endl;

                //std::cout << "doing mini packet of " << i.data.data.size() << std::endl;

                //found_s->do_serialise(i.data, false);

                ///because the next element is a pointer, we force the stream
                ///to decode the pointer data
                //i.data.allow_force = true;
                i.data.force_serialise(found_s, false);

                i.set_complete();

                handle_unprocessed();
            }
        }

        empire_manage.tick_network_take_ownership(net_state);

        for(empire* e : empire_manage.empires)
        {
            if(e->is_player && e->potential_owner == net_state.my_id)
            {
                player_empire = e;
            }
        }

        ///do immediately after networking
        ///not critical but useful
        system_manage.shuffle_networked_orbitals();

        ImGui::BeginOverride("Networking");

        ImGui::clamp_window_to_screen();

        //if(ImGui::Button("Join"))
        {
            net_state.try_join = true;
        }

        if(ImGui::Button("TRY MASSIVE PACKET") && net_state.my_id != -1)
        {
            serialise_data_helper::host_to_id_to_pointer.clear();

            serialise_data_helper::send_mode = 1;
            serialise_data_helper::ref_mode = 1;

            serialise ser;
            ser.default_owner = net_state.my_id;


            ser.handle_serialise(serialise_data_helper::send_mode, true);
            ser.handle_serialise(empire_manage, true);
            ser.handle_serialise(system_manage, true);
            ser.handle_serialise(fleet_manage, true);
            ser.handle_serialise(all_battles, true);

            network_object no_test;

            no_test.owner_id = net_state.my_id;
            no_test.serialise_id = -2;

            net_state.forward_data(no_test, ser);
        }

        if(key_down(sf::Keyboard::H))
            std::cout << "pid " << net_state.packet_id << std::endl;

        //if(ImGui::Button("Try mini packet") && net_state.my_id != -1)

        ///ok. This is fine. What we need to do now is mark serialisation strategies with priorities
        ///and then execute based on priority
        ///priority could be seconds between execution
        ///update objects in motion more frequently (ie currently transferring)
        #if 0
        if(net_state.my_id != -1)
        {
            serialise_data_helper::disk_mode = 0;

            int c_id = 0;
            static int m_id = 0;

            //std::cout << "hello " << std::endl;

            for(orbital_system* sys : system_manage.systems)
            {
                for(orbital* o : sys->orbitals)
                {
                    //o->owner_id = net_state.my_id;

                    //std::cout << o->owner_id << " " << o->serialise_id << std::endl;

                    if(o->owner_id != 1 && o->owner_id != net_state.my_id)
                    {
                        continue;
                    }

                    if(o->type != orbital_info::FLEET)
                    {
                        continue;
                    }

                    if(c_id != m_id)
                    {
                        c_id++;
                        continue;
                    }

                    c_id++;


                    serialise ser;
                    ///because o is a pointer, we allow the stream to force decode the pointer
                    ///if o were a non ptr with a do_serialise method, this would have to be false
                    ser.allow_force = true;
                    ser.default_owner = net_state.my_id;

                    ser.handle_serialise(serialise_data_helper::disk_mode, true);
                    ser.handle_serialise(o, true);

                    //std::cout << ser.data.size() << std::endl;

                    network_object no;
                    ///uuh. Ok it should be this but it wont work yet as we don't by default own stuff
                    ///need to patch ids or something if owner_id is set to default
                    //no.owner_id = o->owner_id;

                    if(o->owner_id == -1)
                        no.owner_id = net_state.my_id;
                    else
                        no.owner_id = o->owner_id;

                    no.serialise_id = o->serialise_id;

                    //std::cout << no.owner_id << " " << no.serialise_id << std::endl;

                    //std::cout << "packet_fragments " << get_packet_fragments(ser.data.size()) << std::endl;

                    net_state.forward_data(no, ser);

                }
            }

            //std::cout << c_id << std::endl;

            m_id++;

            if(c_id > 0)
                m_id %= c_id;
        }
        #endif

        ImGui::End();


        fleet_manage.cull_dead_deferred();
        system_manage.cull_empty_orbital_fleets_deferred(popup);

        net_update.tick(diff_s, net_state, empire_manage, system_manage, fleet_manage, *all_battles);

        net_state.tick_join_game(diff_s);
        net_state.tick();

        handle_unprocessed();

        fleet_manage.shuffle_networked_ships();

        all_battles->remove_bad_orbitals();
        popup.remove_scheduled();
        system_manage.destroy_cleanup(empire_manage);
        fleet_manage.destroy_cleanup(empire_manage);


        if(key_down(sf::Keyboard::N))
        {
            printf("%f\n", diff_s * 1000.f);
        }

        if(key_down(sf::Keyboard::B))
        {
            std::cout << system_manage.zoom_handle.get_zoom() << " " << all_battles->zoom_handle.get_zoom() << std::endl;
        }

        //printf("premanage\n");


        //printf("prepp\n");


        ///being post do_popup seems to fix some flickering
        ///i guess its an imgui ordering thing
        system_manage.draw_ship_ui(player_empire, popup);

        do_ownership_ui(empire_manage, net_state);

        //printf("precull\n");

        empire_manage.tick_cleanup_colonising();

        if(state != 1)
        {
            system_manage.draw_alerts(window, player_empire);
        }

        ///this is slow
        fleet_manage.tick_all(diff_s);

        //printf("predrawres\n");

        player_empire->resources.draw_ui(window, player_empire->last_income);
        //printf("Pregen\n");

        //std::cout << player_empire->culture_similarity << std::endl;

        empire_manage.generate_resources_from_owned(diff_s);
        empire_manage.draw_diplomacy_ui(player_empire, system_manage);
        empire_manage.draw_resource_donation_ui(player_empire);


        player_empire->draw_ui();
        ///this is slow
        //sf::Clock tclk;
        empire_manage.tick_all(diff_s, *all_battles, system_manage, fleet_manage, player_empire);
        //printf("%f dfdfdf\n", tclk.getElapsedTime().asMicroseconds() / 1000.f);

        empire_manage.tick_decolonisation();

        //test_event->tick(diff_s);
        //test_event->draw_ui();
        //test_event->set_interacting_faction(player_empire);

        all_events.draw_ui();
        all_events.tick(diff_s);

        //printf("Prerender\n");

        global_drag_and_drop.tick();

        /*context_menu::start();
        context_menu::tick();
        context_menu::stop();*/

        auto_timer::increment_all();
        auto_timer::dump_imgui();
        auto_timer::reduce();

        ship_customise.tick(scrollwheel_delta, lclick, mpos - mouse_last);

        top_bar::display({window.getSize().x, window.getSize().y});
        tooltip::set_clear_tooltip();

        ImGui::tick_suppress_frames();

        ImGui::DoFrosting(window);

        ImGui::Render();

        /*sf::Sprite spr(tex);
        spr.setScale(1.f, 5.f);
        window.draw(spr);*/

        //playing_music.debug(window);
        window.display();
        window.clear();

        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
