#include <iostream>
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include "../../render_projects/imgui/imgui.h"
#include "../../render_projects/imgui/imgui-SFML.h"
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


///so display this (ish) on mouseover for a component
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

/*std::string obfuscate(const std::string& str, bool should_obfuscate)
{
    if(!should_obfuscate)
        return str;

    return "??/??";
}*/


///claiming_empire for salvage, can be nullptr
void display_ship_info(ship& s, empire* owner, empire* claiming_empire, empire* player_empire, system_manager& system_manage, fleet_manager& fleet_manage, empire_manager& empire_manage, popup_info& popup)
{
    auto produced = s.get_produced_resources(1.f); ///modified by efficiency, ie real amount consumed
    auto consumed = s.get_needed_resources(1.f); ///not actually consumed, but requested
    auto stored = s.get_stored_resources();
    auto max_res = s.get_max_resources();

    float known_information = player_empire->available_scanning_power_on(&s, system_manage);

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


    ImGui::Begin((name_str + "###" + s.name).c_str(), &s.display_ui, ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    std::vector<std::string> headers;
    std::vector<std::string> prod_list;
    std::vector<std::string> cons_list;
    std::vector<std::string> net_list;
    std::vector<std::string> store_max_list;

    for(const ship_component_element& id : elements)
    {
        float prod = produced[id];
        float cons = consumed[id];
        float store = stored[id];
        float maximum = max_res[id];

        //std::string display_str;

        //display_str += "+" + to_string_with_precision(prod, 3) + " | -" + to_string_with_precision(cons, 3) + " | ";

        std::string prod_str = "+" + to_string_with_enforced_variable_dp(prod, 1);
        std::string cons_str = "-" + to_string_with_enforced_variable_dp(cons, 1);

        std::string store_max_str;

        if(maximum > 0)
            store_max_str += "(" + to_string_with_enforced_variable_dp(store) + "/" + to_string_with_variable_prec(maximum) + ")";

        std::string header_str = ship_component_elements::display_strings[id];

        std::string net_str = to_string_with_enforced_variable_dp(prod - cons, 1);

        if(prod - cons >= 0)
        {
            net_str = "+" + net_str;
        }

        //std::string res = header + ": " + display_str + "\n";

        //bool knows = s.is_known_with_scanning_power(id, known_information);

        bool obfuscd = primary_obfuscated[id];

        ///none of these systems have a component with them as a primary
        ///weird and hardcoded. Armour will when ships carry armour
        ///ammo too, hp and fuel only real special case
        if(id == ship_component_element::AMMO ||
           id == ship_component_element::ARMOUR ||
           id == ship_component_element::HP ||
           id == ship_component_element::FUEL)
        {
            if(known_information < 0.5f)
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


        headers.push_back(header_str);
        prod_list.push_back(prod_str);
        cons_list.push_back(cons_str);
        net_list.push_back(net_str);
        store_max_list.push_back(store_max_str);

        //ImGui::Text(res.c_str());
    }

    for(int i=0; i<headers.size(); i++)
    {
        std::string header_formatted = format(headers[i], headers);
        std::string prod_formatted = format(prod_list[i], prod_list);
        std::string cons_formatted = format(cons_list[i], cons_list);
        std::string net_formatted = format(net_list[i], net_list);
        std::string store_max_formatted = format(store_max_list[i], store_max_list);

        //std::string display = header_formatted + ": " + prod_formatted + " | " + cons_formatted + " | " + store_max_formatted;

        //std::string display = header_formatted + " : " + net_formatted + " | " + store_max_formatted;

        //ImGui::Text(display.c_str());

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

    //static std::map<int, std::map<int, bool>> ui_click_state;

    ///ships need ids so the ui can work
    int c_id = 0;

    for(component& c : s.entity_list)
    {
        float hp = 1.f;

        if(c.has_element(ship_component_element::HP))
            hp = c.get_stored()[ship_component_element::HP] / c.get_stored_max()[ship_component_element::HP];

        vec3f max_col = {1.f, 1.f, 1.f};
        vec3f min_col = {1.f, 0.f, 0.f};

        vec3f ccol = max_col * hp + min_col * (1.f - hp);

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

        if(ImGui::IsItemClicked())
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

        /*if(ImGui::CollapsingHeader(name.c_str()))
        {
            ImGui::Text(get_component_display_string(c).c_str());
        }*/

        c_id++;
    }

    float scanning_power = player_empire->available_scanning_power_on(&s, system_manage);

    std::string scanning_str = "Scanning Power: " + std::to_string((int)(scanning_power * 100.f)) + "%%";

    ImGui::Text(scanning_str.c_str());


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
    ///have a recovery cost display?

    if(s.owned_by->parent_empire == player_empire && !s.owned_by->any_in_combat())
    {
        ImGui::Text("(Upgrade to latest Tech)");

        ship c_cpy = s;

        if(s.owned_by->parent_empire != nullptr)
            c_cpy.set_max_tech_level_from_empire_and_ship(s.owned_by->parent_empire);

        ///this doesn't include armour, so we're kind of getting it for free atm
        auto repair_resources = c_cpy.resources_needed_to_repair_total();

        //c_cpy.intermediate_texture = nullptr;

        auto res_cost = c_cpy.resources_cost();

        for(auto& i : repair_resources)
        {
            res_cost[i.first] += i.second;
        }

        auto current_resources = s.resources_cost();

        auto res_remaining = res_cost;

        for(auto& i : current_resources)
        {
            res_remaining[i.first] -= i.second;
        }

        ///setting tech level currently does not have a cost associated with it
        if(ImGui::IsItemClicked())
        {
            if(owner->can_fully_dispense(res_remaining))
            {
                owner->dispense_resources(res_remaining);

                s.set_max_tech_level_from_empire_and_ship(s.owned_by->parent_empire);
            }
        }

        resource_manager rm;
        rm.add(res_remaining);

        for(auto& i : rm.resources)
        {
            i.amount = -i.amount;
        }

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(rm.get_formatted_str(true).c_str());
        }
    }

    if(s.fully_disabled() && claiming_empire != nullptr && s.can_recrew(claiming_empire) && !s.owned_by->any_in_combat())
    {
        ImGui::Text("(Recrew)");

        auto res = s.resources_needed_to_recrew_total();

        ///crew research has 0 bound, scrapped research has minimum bound
        float recrew_research_currenct = s.get_recrew_potential_research(claiming_empire).units_to_currency(false);

        resource_manager rm;

        for(auto& i : res)
        {
            rm.resources[i.first].amount = -i.second;
        }

        std::string rstr = rm.get_formatted_str(true);

        rstr += "Potential Re: " + std::to_string((int)recrew_research_currenct);

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(rstr.c_str());
        }

        ///if originating empire is not the claiming empire, get some tech
        if(ImGui::IsItemClicked())
        {
            ///depletes resources
            ///should probably pull the resource stuff outside of here as there might be other sources of recrewing
            s.recrew_derelict(owner, claiming_empire);

            ship_manager* parent_fleet = s.owned_by;

            orbital_system* os = system_manage.get_by_element(parent_fleet);

            orbital* o = os->get_by_element(parent_fleet);

            assert(o);

            ///?
            /*if(parent_fleet->ships.size() == 1)
            {
                owner->release_ownership(parent_fleet);
                owner->release_ownership(o);
            }*/

            //claiming_empire->take_ownership(o);

            ship_manager* new_sm = fleet_manage.make_new();

            orbital* new_orbital = os->make_new(orbital_info::FLEET, 5.f);
            new_orbital->orbital_angle = o->orbital_angle;
            new_orbital->orbital_length = o->orbital_length;
            new_orbital->parent = o->parent;
            new_orbital->data = new_sm;

            claiming_empire->take_ownership(new_orbital);

            new_sm->steal(&s);

            claiming_empire->take_ownership(new_sm);

            //o->data = new_sm;

            parent_fleet->toggle_fleet_ui = false;

            popup.going = false;
            popup.elements.clear();
        }
    }


    if(s.fully_disabled() && claiming_empire != nullptr && !s.owned_by->any_in_combat())
    {
        ///should this be originating empire?
        research research_raw = s.get_research_real_for_empire(s.owned_by->parent_empire, claiming_empire);

        auto res = s.resources_received_when_scrapped();

        res[resource::RESEARCH] = research_raw.units_to_currency(true);

        resource_manager rm;

        for(auto& i : res)
        {
            rm.resources[i.first].amount = i.second;
        }

        std::string text = "(Scrap for resources)";

        ImGui::Text(text.c_str());

        std::string rstr = rm.get_formatted_str(true);

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(rstr.c_str());
        }

        if(ImGui::IsItemClicked())
        {
            ///destroy ship?

            for(auto& i : res)
            {
                claiming_empire->add_resource(i.first, i.second);
            }

            s.cleanup = true;

            popup.going = false;
            popup.elements.clear();
        }
    }

    ///if derelict SALAGE BBZ or recapture YEAAAAAH
    ///recapturing will take some resources to prop up the crew and some necessary systems
    ///or just... fully repair? Maybe make a salvage literally just a resupply + empire change?

    ImGui::End();
}

void display_ship_info_old(ship& s, float step_s)
{
    auto display_strs = get_components_display_string(s);

    ImGui::Begin(s.name.c_str());

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
}


/*void debug_menu(const std::vector<ship*>& ships)
{
    ImGui::Begin("Debug");

    if(ImGui::Button("Tick 1s"))
    {
        for(auto& i : ships)
        {
            i->tick_all_components(1.f);
        }
    }

    ImGui::End();
}*/

void debug_battle(battle_manager* battle, sf::RenderWindow& win, bool lclick, system_manager& system_manage)
{
    if(battle == nullptr)
        return;

    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    ship* s = battle->get_ship_under({transformed.x, transformed.y});

    if(s)
    {
        s->highlight = true;

        if(lclick)
        {
            s->display_ui = !s->display_ui;
        }
    }

    ImGui::Begin("Battle DBG");

    if(ImGui::Button("Step Battle 1s"))
    {
        battle->tick(1.f, system_manage);
    }

    ImGui::End();
}

void debug_all_battles(all_battles_manager& all_battles, sf::RenderWindow& win, bool lclick, system_manager& system_manage, empire* player_empire)
{
    if(!top_bar::get_active(top_bar_info::BATTLES))
        return;

    ImGui::Begin("Ongoing Battles", &top_bar::active[top_bar_info::BATTLES], IMGUI_WINDOW_FLAGS);

    for(int i=0; i<all_battles.battles.size(); i++)
    {
        std::string id_str = std::to_string(i);

        battle_manager* bm = all_battles.battles[i];

        if(!bm->any_in_empire_involved(player_empire))
            continue;

        for(auto& i : bm->ships)
        {
            for(ship* kk : i.second)
            {
                std::string name = kk->name;
                std::string team = std::to_string(kk->team);
                float damage = kk->get_stored_resources()[ship_component_elements::HP];
                float damage_max = kk->get_max_resources()[ship_component_elements::HP];

                std::string damage_str = "(" + to_string_with_enforced_variable_dp(damage) + "/" + to_string_with_enforced_variable_dp(damage_max) + ")";

                ImGui::Text((team + " | " + name + " " + damage_str).c_str());

                if(ImGui::IsItemClicked())
                {
                    kk->display_ui = !kk->display_ui;
                }
            }
        }

        ///will be incorrect for a frame when combat changes
        std::string jump_to_str = "(Jump To)";

        ImGui::PushID((jump_to_str + id_str).c_str());
        ImGui::Text(jump_to_str.c_str());
        ImGui::PopID();

        if(ImGui::IsItemClicked())
        {
            all_battles.set_viewing(bm, system_manage);
        }

        if(!bm->can_end_battle_peacefully(player_empire))
        {
            std::string disengage_str = bm->get_disengage_str(player_empire);

            if(bm->can_disengage(player_empire))
            {
                disengage_str = "(Emergency Disengage!)";
            }

            ImGui::PushID((disengage_str + id_str).c_str());
            ImGui::Text(disengage_str.c_str());
            ImGui::PopID();

            if(ImGui::IsItemClicked())
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
            std::string leave_str = "(Leave Battle)";

            ImGui::PushID((leave_str + id_str).c_str());
            ImGui::Text(leave_str.c_str());
            ImGui::PopID();

            if(ImGui::IsItemClicked())
            {
                all_battles.end_battle_peacefully(bm);
            }
        }
    }

    debug_battle(all_battles.currently_viewing, win, lclick, system_manage);

    ImGui::End();
}

///this function is one of the worst in the code, there's a lot of duplication that's gradually being exposed
///new functionality is however forcing this to be refactored to be less dumb
void debug_system(system_manager& system_manage, sf::RenderWindow& win, bool lclick, bool rclick, popup_info& popup, empire* player_empire, all_battles_manager& all_battles, fleet_manager& fleet_manage, all_events_manager& all_events)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    sf::Keyboard key;

    bool lshift = key.isKeyPressed(sf::Keyboard::LShift);

    ///this is where we click away fleets
    if(lclick && !lshift && (system_manage.hovered_system == nullptr || system_manage.in_system_view()))
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

    if(system_manage.currently_viewed != nullptr && system_manage.in_system_view())
    {
        for(orbital* orb : system_manage.currently_viewed->orbitals)
        {
            if(orb->point_within({transformed.x, transformed.y}))
            {
                valid_selection_targets.push_back(orb);

                break;
            }
        }
    }

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

        if(lclick)
        {
            orb->clicked = true;
        }

        if(lclick && popup.fetch(orb))
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
            ImGui::SetTooltip((orb->name + "\n" + orb->get_empire_str() + orb->description).c_str());
        }

        if(orb->is_resource_object)
        {
            std::string res_first = orb->name;

            res_first += "\n" + orb->get_empire_str();

            if(orb->viewed_by[player_empire])
                res_first += orb->produced_resources_ps.get_formatted_str();

            ImGui::SetTooltip(res_first.c_str());
        }
    }
}

void do_popup(popup_info& popup, sf::RenderWindow& win, fleet_manager& fleet_manage, system_manager& system_manage, orbital_system* current_system, empire_manager& empires, empire* player_empire, all_events_manager& all_events, all_battles_manager& all_battles, bool rclick)
{
    if(popup.elements.size() == 0)
        popup.going = false;

    if(!popup.going)
        return;

    sf::Mouse mouse;
    sf::Keyboard key;

    bool lctrl = key.isKeyPressed(sf::Keyboard::LControl);

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    global_drag_and_drop.begin_drag_section("INFO_PANEL");

    ImGui::Begin(("Selected###INFO_PANEL"), &popup.going, ImVec2(0,0), -1.f, ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    //global_drag_and_drop.begin_dragging(nullptr, drag_and_drop_info::ORBITAL);

    std::set<ship*> potential_new_fleet;

    std::map<empire*, std::vector<orbital*>> orbitals_grouped_by_empire;

    for(popup_element& elem : popup.elements)
    {
        orbital* orb = (orbital*)elem.element;

        empire* cur_empire = orb->parent_empire;

        bool do_obfuscate_misc = false;

        if(orb->type == orbital_info::FLEET && player_empire->available_scanning_power_on((ship_manager*)orb->data, system_manage) <= ship_info::accessory_information_obfuscation_level)
        {
            do_obfuscate_misc = true;
        }

        if(do_obfuscate_misc)
        {
            cur_empire = nullptr;
        }

        orbitals_grouped_by_empire[cur_empire].push_back(orb);
    }

    bool first_orbital = true;

    for(auto& grouped_orbitals : orbitals_grouped_by_empire)
    {
        empire* current_empire = grouped_orbitals.first;

        std::string empire_name;

        if(current_empire != nullptr)
        {
            empire_name = "Empire: " + current_empire->name;
        }
        else
        {
            empire_name = "Empire: Unknown Empire";
        }

        vec3f col = player_empire->get_relations_colour(current_empire);

        ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), empire_name.c_str());

        ImGui::Indent();

        for(orbital* orb : grouped_orbitals.second)
        {
            if(orb->type != orbital_info::FLEET && !first_orbital)
                ImGui::NewLine();

            first_orbital = false;

            orbital* o = orb;

            ship_manager* sm = (ship_manager*)orb->data;

            {
                bool do_obfuscate_name = false;

                if(o->type == orbital_info::FLEET && !player_empire->is_allied(o->parent_empire))
                {
                    if(player_empire->available_scanning_power_on((ship_manager*)o->data, system_manage) <= ship_info::ship_obfuscation_level)
                    {
                        do_obfuscate_name = true;
                    }
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

                if(rclick && (o->type == orbital_info::FLEET) && o->parent_empire == player_empire && o->parent_system == system_manage.currently_viewed && !sm->any_colonising() && system_manage.in_system_view())
                {
                    o->request_transfer({transformed.x, transformed.y});
                }

                float hp_frac = 1.f;

                ImGui::Text(name.c_str());

                if(global_drag_and_drop.currently_dragging == drag_and_drop_info::SHIP && global_drag_and_drop.let_go_on_item())
                {
                    ship* s = (ship*)global_drag_and_drop.data;

                    ship_manager* found_sm = s->owned_by;

                    orbital* my_o = system_manage.get_by_element_orbital(found_sm);

                    if(my_o != nullptr && my_o->type == orbital_info::FLEET && sm != found_sm)
                    {
                        if(found_sm->ships.size() == 1)
                        {
                            popup.rem(my_o);
                            //popup.elements.clear();
                        }

                        sm->steal(s);
                    }

                    printf("hi\n");
                }

                /*if(popup.elements.size() > 1)
                {
                    ImGui::SameLine();

                    ImGui::Text("(Select)");
                }

                if(ImGui::IsItemClicked())
                {
                    popup.rem_all_but(o);
                }*/

                std::vector<std::string> data = o->get_info_str(player_empire, true);

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

                    float hp_frac = 1.f;

                    vec3f full_col = {1,1,1};
                    vec3f damaged_col = {1, 0.1, 0.1};

                    vec3f draw_col = full_col;

                    if(o->type == orbital_info::FLEET)
                    {
                        cur_ship = sm->ships[i];

                        hp_frac = cur_ship->get_stored_resources()[ship_component_element::HP] / cur_ship->get_max_resources()[ship_component_element::HP];

                        draw_col = mix(damaged_col, full_col, hp_frac*hp_frac);
                    }

                    ImGui::TextColored(ImVec4(draw_col.x(), draw_col.y(), draw_col.z(), 1), str.c_str());

                    if(o->type == orbital_info::FLEET)
                    {
                        bool shift = key.isKeyPressed(sf::Keyboard::LShift);

                        bool can_open_window = true;

                        if(can_open_window && ImGui::IsItemHovered())
                            ImGui::SetTooltip("Left Click to view ship");
                        if(sm->can_merge && ImGui::IsItemHovered())
                            ImGui::SetTooltip("Shift-Click to add to fleet");

                        if(ImGui::IsItemClicked() && !shift && !lctrl && can_open_window)
                        {
                            cur_ship->display_ui = !cur_ship->display_ui;
                        }

                        if(ImGui::IsItemClicked() && shift && !lctrl && sm->can_merge)
                        {
                            cur_ship->shift_clicked = !cur_ship->shift_clicked;
                        }

                        if(ImGui::IsItemClicked() && lctrl && sm->can_merge)
                        {
                            global_drag_and_drop.begin_dragging(cur_ship, drag_and_drop_info::SHIP, cur_ship->name);
                        }

                        /*if(global_drag_and_drop.let_go_on_item())
                        {
                            printf("dropped\n");
                        }*/
                    }
                }

                if(o->type == orbital_info::FLEET)
                    ImGui::Unindent();
            }

            //ImGui::SameLine();

            ImGui::Text("");

            if(popup.elements.size() > 1)
            {
                ImGui::SameLine(0.f, 0.f);

                ImGui::Text("(Select) ");

                if(ImGui::IsItemClicked())
                {
                    popup.rem_all_but(o);
                }
            }

            if(orb->type == orbital_info::FLEET)
            {
                bool can_resupply = orb->type == orbital_info::FLEET && (orb->parent_empire == player_empire || orb->parent_empire->is_allied(player_empire)) && !(sm->any_in_combat() || sm->all_derelict());

                if(can_resupply)
                {
                    ImGui::SameLine(0.f, 0.f);

                    ImGui::GoodText("(Resupply) ");

                    if(ImGui::IsItemClicked())
                    {
                        ship_manager* sm = (ship_manager*)o->data;

                        sm->resupply(player_empire, false);
                    }
                }

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
                ImGui::SameLine(0.f, 0.f);

                ImGui::BadText("(Engage Fleets) ");

                if(ImGui::IsItemClicked())
                {
                    assert(parent_system);

                    std::vector<orbital*> hostile_fleets = parent_system->get_fleets_within_engagement_range(o, true);

                    hostile_fleets.push_back(o);

                    all_battles.make_new_battle(hostile_fleets);
                }
            }

            bool can_declare_war = orb->parent_empire != nullptr && !orb->parent_empire->is_hostile(player_empire) && orb->parent_empire != player_empire;

            if(can_declare_war)
            {
                ImGui::SameLine(0.f, 0.f);

                ImGui::BadText("(Declare War) ");

                if(ImGui::IsItemClicked() && popup.fetch(orb) != nullptr)
                    popup.fetch(orb)->declaring_war = true;

                if(popup.fetch(orb) != nullptr && popup.fetch(orb)->declaring_war)
                {
                    ImGui::SameLine(0.f, 0.f);

                    ImGui::BadText("(Are you sure?) ");

                    if(ImGui::IsItemClicked())
                    {
                        orbital* o = orb;

                        assert(o);

                        empire* parent = o->parent_empire;

                        if(!parent->is_hostile(player_empire))
                        {
                            parent->become_hostile(player_empire);
                        }
                    }
                }
            }

            if(orb->type == orbital_info::FLEET)
            {
                bool enable_engage_cooldown = !sm->can_engage() && sm->parent_empire == player_empire;

                if(enable_engage_cooldown)
                {
                    ImGui::Text(sm->get_engage_str().c_str());
                }
            }

            bool can_colonise = orb->type == orbital_info::PLANET && player_empire->can_colonise(orb) && fleet_manage.nearest_free_colony_ship_of_empire(orb, player_empire) != nullptr;

            if(can_colonise)
            {
                ImGui::SameLine(0.f, 0.f);

                ImGui::GoodText("(Colonise) ");

                if(ImGui::IsItemClicked())
                {
                    assert(o);

                    ship* nearest = fleet_manage.nearest_free_colony_ship_of_empire(o, player_empire);

                    if(nearest)
                    {
                        nearest->colonising = true;
                        nearest->colonise_target = o;
                    }
                }
            }

            ///for drawing warp radiuses, but will take anything and might be extended later
            system_manage.add_selected_orbital(orb);

            if(o->has_quest_alert)
            {
                game_event_manager* event = all_events.orbital_to_game_event(o);

                //ship_manager* nearest_fleet = event->get_nearest_fleet(player_empire);

                if(event != nullptr && event->interacting_faction != nullptr && event->interacting_faction != player_empire)
                {
                    ImGui::Text("(Another empire has already claimed this event)");
                }
                else if(event != nullptr && !event->can_interact(player_empire))
                {
                    ImGui::Text("(Requires nearby fleet to view alert)");

                    event->set_dialogue_state(false);
                }
                else
                {
                    if(o->dialogue_open)
                        ImGui::Text("(Hide Alert)");
                    else
                        ImGui::Text("(View Alert)");

                    if(ImGui::IsItemClicked())
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

            if(o->type == orbital_info::PLANET && o->can_construct_ships && o->parent_empire == player_empire)
            {
                if(o->construction_ui_open)
                    ImGui::Text("(Hide Construction Window)");
                else
                    ImGui::Text("(Show Construction Window)");

                if(ImGui::IsItemClicked())
                {
                    o->construction_ui_open = !o->construction_ui_open;
                }
            }

            if(o->type == orbital_info::PLANET && o->decolonise_timer_s > 0.0001f)
            {
                std::string decolo_time = "Time until decolonised " + to_string_with_enforced_variable_dp(orbital_info::decolonise_time_s - o->decolonise_timer_s);

                ImGui::Text(decolo_time.c_str());
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

                orbital_system* parent = system_manage.get_parent(o);

                if(parent == system_manage.hovered_system || parent == nullptr)
                    continue;

                if(o->parent_empire != player_empire)
                    continue;

                ship_manager* sm = (ship_manager*)o->data;

                if(!sm->can_warp(system_manage.hovered_system, parent, o))
                {
                    //ImGui::SetTooltip("Cannot Warp");
                    tooltip::add("Cannot Warp");
                }
                else
                {
                    //ImGui::SetTooltip("Right click to Warp");
                    tooltip::add("Right click to Warp");
                }

                if(rclick && sm->parent_empire == player_empire)
                {
                    sm->try_warp(system_manage.hovered_system, parent, o);
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

    if(potential_new_fleet.size() > 0)
    {
        ImGui::Text("(Make New Fleet)");

        if(ImGui::IsItemClicked())
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

                float fleet_angle = 0;
                float fleet_length = 200;
                empire* parent = nullptr;

                for(ship* i : potential_new_fleet)
                {
                    ///we don't want to free associated orbital if it still exists from empire
                    ///only an issue if we cull, which is why ownership is culled there
                    orbital* real = current_system->get_by_element((void*)i->owned_by);

                    if(real != nullptr)
                    {
                        parent = real->parent_empire;

                        fleet_pos = real->absolute_pos;

                        fleet_angle = real->orbital_angle;
                        fleet_length = real->orbital_length;
                    }

                    i->owned_by->toggle_fleet_ui = false;
                    i->shift_clicked = false;

                    ///if we're going to be removed from popup
                    if(real != nullptr && i->owned_by->ships.size() == 1)
                        popup.rem(real);

                    ns->steal(i);
                }

                orbital* associated = current_system->make_new(orbital_info::FLEET, 5.f);
                associated->parent = current_system->get_base();
                associated->set_orbit(fleet_pos);
                associated->data = ns;
                associated->tick(0.f);

                parent->take_ownership(associated);
                parent->take_ownership(ns);

                //popup.clear();
                //popup.elements.clear();
                //popup.going = false;

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

    system_manage.cull_empty_orbital_fleets(empires);
    fleet_manage.cull_dead(empires);
    system_manage.cull_empty_orbital_fleets(empires);
    fleet_manage.cull_dead(empires);

    ImGui::End();
}

struct construction_window_state
{
    research picked_research_levels;
};

void do_construction_window(orbital* o, empire* player_empire, fleet_manager& fleet_manage, construction_window_state& window_state)
{
    if(!o->construction_ui_open)
        return;

    ImGui::Begin("Ship Construction", &o->construction_ui_open, ImGuiWindowFlags_AlwaysAutoResize);

    for(int i=0; i<window_state.picked_research_levels.categories.size(); i++)
    {
        research_category& category = window_state.picked_research_levels.categories[i];

        std::string text = format(research_info::names[i], research_info::names);

        ImGui::Text((text + " | ").c_str());

        ImGui::SameLine();

        ImGui::Text("(-)");

        if(ImGui::IsItemClicked())
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

        if(ImGui::IsItemClicked())
        {
            category.amount += 1.f;

            category.amount = clamp(category.amount, 0.f, player_empire->research_tech_level.categories[i].amount);

            category.amount = floor(category.amount);
        }
    }

    std::vector<ship> ships
    {
        make_default(),
        make_scout(),
        make_colony_ship()
    };

    for(auto& test_ship : ships)
    {
        test_ship.set_tech_level_from_research(window_state.picked_research_levels);

        auto cost = test_ship.resources_cost();

        resource_manager rm;
        rm.add(cost);

        std::string str_resources = rm.get_formatted_str(true);

        std::string str = test_ship.name;

        ImGui::Text(("(Make " + str + " Ship)").c_str());

        bool clicked = ImGui::IsItemClicked();

        ImGui::Text(str_resources.c_str());

        if(clicked)
        {
            if(player_empire->can_fully_dispense(cost))
            {
                player_empire->dispense_resources(cost);

                orbital_system* os = o->parent_system;

                ship_manager* new_fleet = fleet_manage.make_new();

                ship* new_ship = new_fleet->make_new_from(player_empire, test_ship);
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
            }
        }
    }

    ImGui::End();
}

void handle_camera(sf::RenderWindow& window, system_manager& system_manage)
{
    sf::View view = window.getDefaultView();

    view.setSize(window.getSize().x * system_manage.zoom_level, window.getSize().y * system_manage.zoom_level);
    //view.zoom(system_manage.zoom_level);
    view.setCenter({system_manage.camera.x(), system_manage.camera.y()});

    window.setView(view);
}

int main()
{
    randf_s();
    randf_s();

    top_bar::active[top_bar_info::ECONOMY] = true;
    top_bar::active[top_bar_info::RESEARCH] = true;
    top_bar::active[top_bar_info::BATTLES] = true;
    top_bar::active[top_bar_info::FLEETS] = true;

    //music playing_music;

    empire_manager empire_manage;

    empire* player_empire = empire_manage.make_new();
    player_empire->name = "Glorious Azerbaijanian Conglomerate";
    player_empire->ship_prefix = "SS";

    player_empire->resources.resources[resource::IRON].amount = 5000.f;
    player_empire->resources.resources[resource::COPPER].amount = 5000.f;
    player_empire->resources.resources[resource::TITANIUM].amount = 5000.f;
    player_empire->resources.resources[resource::URANIUM].amount = 5000.f;
    player_empire->resources.resources[resource::RESEARCH].amount = 8000.f;
    player_empire->resources.resources[resource::HYDROGEN].amount = 8000.f;
    player_empire->resources.resources[resource::OXYGEN].amount = 8000.f;

    empire* hostile_empire = empire_manage.make_new();
    hostile_empire->name = "Irate Uzbekiztaniaite Spacewombles";
    hostile_empire->has_ai = true;

    hostile_empire->research_tech_level.categories[research_info::MATERIALS].amount = 3.f;
    hostile_empire->trade_space_access(player_empire, true);


    hostile_empire->resources.resources[resource::IRON].amount = 5000.f;
    hostile_empire->resources.resources[resource::COPPER].amount = 5000.f;
    hostile_empire->resources.resources[resource::TITANIUM].amount = 5000.f;
    hostile_empire->resources.resources[resource::URANIUM].amount = 5000.f;
    hostile_empire->resources.resources[resource::RESEARCH].amount = 8000.f;
    hostile_empire->resources.resources[resource::HYDROGEN].amount = 8000.f;
    hostile_empire->resources.resources[resource::OXYGEN].amount = 8000.f;

    //player_empire->ally(hostile_empire);
    //player_empire->become_hostile(hostile_empire);

    empire* derelict_empire = empire_manage.make_new();
    derelict_empire->name = "Test Ancient Faction";
    derelict_empire->is_derelict = true;

    ///manages FLEETS, not SHIPS
    ///this is fine. This is a global thing, the highest level of storage for FLEETS of ships
    ///FLEEEEETS
    ///Should have named this something better
    fleet_manager fleet_manage;

    ship_manager* fleet1 = fleet_manage.make_new();
    ship_manager* fleet2 = fleet_manage.make_new();
    ship_manager* fleet3 = fleet_manage.make_new();
    ship_manager* fleet4 = fleet_manage.make_new();
    //ship_manager* fleet5 = fleet_manage.make_new();

    //ship test_ship = make_default();
    //ship test_ship2 = make_default();

    ship* test_ship = fleet1->make_new_from(player_empire, make_default());
    ship* test_ship3 = fleet1->make_new_from(player_empire, make_default());

    ship* test_ship2 = fleet2->make_new_from(hostile_empire, make_default());

    test_ship2->set_tech_level_from_empire(hostile_empire);

    ship* test_ship4 = fleet3->make_new_from(player_empire, make_default());

    ship* derelict_ship = fleet4->make_new_from(hostile_empire, make_default());

    ship* scout_ship = fleet3->make_new_from(player_empire, make_colony_ship());
    //ship* scout_ship2 = fleet5->make_new_from(player_empire->team_id, make_colony_ship());

    test_ship->name = "SS Icarus";
    test_ship2->name = "SS Buttz";
    test_ship3->name = "SS Duplicate";
    test_ship4->name = "SS Secondary";

    derelict_ship->name = "SS Dereliction";

    scout_ship->name = "SS Scout";
    //scout_ship2->name = "SS Scout2";

    /*test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);*/

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window;

    window.create(sf::VideoMode(1500, 900),"Wowee", sf::Style::Default, settings);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildWindowRounding = 2;

    sf::Clock clk;

    float last_time_s = 0.f;
    float diff_s = 0.f;

    all_battles_manager all_battles;

    //battle_manager* battle = all_battles.make_new();

    //battle->add_ship(test_ship);
    //battle->add_ship(test_ship2);

    system_manager system_manage;

    orbital_system* base = system_manage.make_new();

    orbital* sun = base->make_new(orbital_info::STAR, 10.f, 10);
    orbital* planet = base->make_new(orbital_info::PLANET, 5.f, 10);
    planet->parent = sun;

    sun->rotation_velocity_ps = 2*M_PI/60.f;

    planet->orbital_length = 150.f;
    planet->rotation_velocity_ps = 2*M_PI / 200.f;

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

    orbital* tplanet = base->make_new(orbital_info::PLANET, 3.f);
    tplanet->orbital_length = 50.f;
    tplanet->parent = sun;

    base->generate_asteroids_old(500, 5, 5);
    //base->generate_planet_resources(2.f);


    /*orbital* oscout = base->make_new(orbital_info::FLEET, 5.f);
    oscout->orbital_angle = 0.f;
    oscout->orbital_length = 270;
    oscout->parent = sun;
    oscout->data = fleet5;*/


    player_empire->take_ownership_of_all(base);
    player_empire->take_ownership(fleet1);
    player_empire->take_ownership(fleet3);
    //player_empire->take_ownership(fleet5);

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

    //orbital_system* sys_2 = system_manage.make_new();
    //sys_2->generate_random_system(3, 100, 3, 5);

    orbital_system* sys_2 = system_manage.make_new();
    sys_2->generate_full_random_system();
    sys_2->universe_pos = {10, 10};

    empire* e2 = empire_manage.birth_empire(system_manage, fleet_manage, sys_2);
    //empire* e2 = empire_manage.birth_empire_without_system_ownership(fleet_manage, sys_2, 2, 2);

    //player_empire->become_hostile(e2);
    player_empire->ally(e2);
    player_empire->positive_relations(e2, 0.5f);

    system_manage.generate_universe(100);

    empire_manage.birth_empires_random(fleet_manage, system_manage);
    empire_manage.birth_empires_without_ownership(fleet_manage, system_manage);

    system_manage.set_viewed_system(base);

    construction_window_state window_state;

    popup_info popup;

    all_events_manager all_events;

    game_event_manager* test_event = all_events.make_new(game_event_info::ANCIENT_PRECURSOR, tplanet, fleet_manage, system_manage);

    test_event->set_faction(derelict_empire);

    game_event_manager* test_event2 = all_events.make_new(game_event_info::LONE_DERELICT, sun, fleet_manage, system_manage);

    test_event2->set_faction(derelict_empire);

    sf::Keyboard key;
    sf::Mouse mouse;

    int state = 0;

    bool focused = true;

    bool fullscreen = false;

    while(window.isOpen())
    {
        /*playing_music.tick(diff_s);

        if(once<sf::Keyboard::Add>())
        {
            playing_music.tg.make_cell_random();
        }*/

        vec2f mpos = {mouse.getPosition(window).x, mouse.getPosition(window).y};

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

            if(event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                scrollwheel_delta += event.mouseWheelScroll.delta;

            if(event.type == sf::Event::Resized)
            {
                int x = event.size.width;
                int y = event.size.height;

                window.setSize({x, y});
            }
        }

        if(focused && key.isKeyPressed(sf::Keyboard::LAlt) && ONCE_MACRO(sf::Keyboard::Return))
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

        if(focused && key.isKeyPressed(sf::Keyboard::F10))
        {
            window.close();
        }

        vec2f cdir = {0,0};

        if(key.isKeyPressed(sf::Keyboard::W))
            cdir.y() += 1;

        if(key.isKeyPressed(sf::Keyboard::S))
            cdir.y() -= 1;

        if(key.isKeyPressed(sf::Keyboard::A))
            cdir.x() += 1;

        if(key.isKeyPressed(sf::Keyboard::D))
            cdir.x() -= 1;

        if(key.isKeyPressed(sf::Keyboard::LShift))
            cdir = cdir * 5.f;

        if(!focused)
            cdir = 0.f;

        system_manage.pan_camera(cdir * diff_s * 300);

        if(no_suppress_mouse)
            system_manage.change_zoom(-scrollwheel_delta, mpos, window);

        ///BATTLE MANAGER IS NO LONGER TICKING SHIP COMPONENTS, IT IS ASSUMED TO BE DONE GLOBALLY WHICH WE WONT WANT
        ///WHEN BATTLES ARE SEPARATED FROM GLOBAL TIME
        /*if(key.isKeyPressed(sf::Keyboard::Num1))
        {

        }*/

        all_battles.tick(diff_s, system_manage);

        handle_camera(window, system_manage);

        if(ONCE_MACRO(sf::Keyboard::M) && focused)
        {
            system_manage.enter_universe_view();
        }

        if(ONCE_MACRO(sf::Keyboard::F1))
        {
            state = (state + 1) % 2;

            if(state == 0)
            {
                system_manage.set_viewed_system(base);
            }
            if(state == 1)
            {
                ///need way to view any battle
                //battle->set_view(system_manage);
                if(all_battles.currently_viewing != nullptr)
                    all_battles.currently_viewing->set_view(system_manage);

                system_manage.set_viewed_system(nullptr);
            }
        }

        if(ONCE_MACRO(sf::Keyboard::F3))
        {
            system_manage.set_viewed_system(sys_2);
            state = 0;
        }

        /*if(once<sf::Keyboard::F2>())
        {
            test_ship->resupply(*player_empire);
        }*/

        bool lclick = ONCE_MACRO(sf::Mouse::Left) && no_suppress_mouse;
        bool rclick = ONCE_MACRO(sf::Mouse::Right) && no_suppress_mouse;

        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        //display_ship_info(*test_ship);

        //debug_menu({test_ship});

        handle_camera(window, system_manage);

        debug_all_battles(all_battles, window, lclick, system_manage, player_empire);

        if(state == 1)
        {
            //debug_battle(*battle, window, lclick);


            all_battles.draw_viewing(window);

            //battle->draw(window);
        }

        system_manage.tick(diff_s);

        if(state == 0)
        {
            debug_system(system_manage, window, lclick, rclick, popup, player_empire, all_battles, fleet_manage, all_events);
        }

        do_popup(popup, window, fleet_manage, system_manage, system_manage.currently_viewed, empire_manage, player_empire, all_events, all_battles, rclick);

        if(state == 0)
        {
            ///this is slow
            system_manage.draw_viewed_system(window, player_empire);
            ///dis kinda slow too
            system_manage.draw_universe_map(window, player_empire, popup);
            system_manage.process_universe_map(window, lclick, player_empire);
            system_manage.draw_warp_radiuses(window, player_empire);
        }


        //printf("ui\n");

        for(ship_manager* smanage : fleet_manage.fleets)
        {
            for(ship* s : smanage->ships)
            {
                if(s->display_ui)
                {
                    display_ship_info(*s, smanage->parent_empire, player_empire, player_empire, system_manage, fleet_manage, empire_manage, popup);
                }
            }
        }


        for(orbital_system* os : system_manage.systems)
        {
            for(orbital* o : os->orbitals)
            {
                if(o->parent_empire != player_empire)
                    continue;

                do_construction_window(o, player_empire, fleet_manage, window_state);
            }
        }


        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);


        if(key.isKeyPressed(sf::Keyboard::N))
        {
            printf("%f\n", diff_s * 1000.f);
        }

        //printf("premanage\n");


        //printf("prepp\n");


        ///being post do_popup seems to fix some flickering
        ///i guess its an imgui ordering thing
        system_manage.draw_ship_ui(player_empire, popup);

        //printf("precull\n");

        empire_manage.tick_cleanup_colonising();
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);


        if(state != 1)
        {
            system_manage.draw_alerts(window, player_empire);
        }

        ///this is slow
        fleet_manage.tick_all(diff_s);

        //printf("predrawres\n");

        player_empire->resources.draw_ui(window);
        //printf("Pregen\n");


        empire_manage.generate_resources_from_owned(diff_s);
        empire_manage.draw_diplomacy_ui(player_empire, system_manage);
        empire_manage.draw_resource_donation_ui(player_empire);


        player_empire->draw_ui();
        ///this is slow
        //sf::Clock tclk;
        empire_manage.tick_all(diff_s, all_battles, system_manage, fleet_manage, player_empire);
        //printf("%f dfdfdf\n", tclk.getElapsedTime().asMicroseconds() / 1000.f);

        empire_manage.tick_decolonisation();

        //test_event->tick(diff_s);
        //test_event->draw_ui();
        //test_event->set_interacting_faction(player_empire);

        all_events.draw_ui();
        all_events.tick(diff_s);

        ///um ok. This is correct if slightly stupid
        ///we cull empty orbital fleets, then we cull the dead fleet itself
        ///ships might have been culled in cull_dead, so we then need to cull any fleets with nothing in them
        ///and then actually remove those ships in the next cull dead
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);

        //printf("Prerender\n");

        global_drag_and_drop.tick();

        top_bar::display();
        tooltip::set_clear_tooltip();

        ImGui::Render();
        //playing_music.debug(window);
        window.display();
        window.clear();

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
