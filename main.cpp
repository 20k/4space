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

template<sf::Keyboard::Key k>
bool once()
{
    static bool last;

    sf::Keyboard key;

    if(key.isKeyPressed(k) && !last)
    {
        last = true;

        return true;
    }

    if(!key.isKeyPressed(k))
    {
        last = false;
    }

    return false;
}

template<sf::Mouse::Button b>
bool once()
{
    static bool last;

    sf::Mouse mouse;

    if(mouse.isButtonPressed(b) && !last)
    {
        last = true;

        return true;
    }

    if(!mouse.isButtonPressed(b))
    {
        last = false;
    }

    return false;
}


#include "ship_definitions.hpp"

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

struct button_element
{
    std::string name;
    bool pressed = false;
};

struct popup_element
{
    static int gid;
    int id = gid++;

    std::string header;
    std::vector<std::string> data;
    std::deque<bool> checked;
    bool mergeable = false;
    bool toggle_clickable = false;

    std::map<popup_element_type::types, button_element> buttons_map;

    void* element = nullptr;
};

int popup_element::gid;

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

    bool going = false;
};

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

std::string obfuscate(const std::string& str, bool should_obfuscate)
{
    if(!should_obfuscate)
        return str;

    std::string ret = str;

    for(int i=0; i<ret.length(); i++)
    {
        if(isalnum(ret[i]))
        {
            ret[i] = '?';
        }
    }

    return ret;
}

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

    std::string name_str = s.name;

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


    ImGui::Begin((name_str + "###" + s.name).c_str(), &s.display_ui, ImGuiWindowFlags_AlwaysAutoResize);

    std::vector<std::string> headers;
    std::vector<std::string> prod_list;
    std::vector<std::string> cons_list;
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

        //std::string res = header + ": " + display_str + "\n";

        //bool knows = s.is_known_with_scanning_power(id, known_information);

        bool obfuscd = primary_obfuscated[id];

        if(s.owned_by->parent_empire->is_allied(player_empire))
        {
            obfuscd = false;
        }

        header_str = obfuscate(header_str, obfuscd);
        prod_str = obfuscate(prod_str, obfuscd);
        cons_str = obfuscate(cons_str, obfuscd);
        store_max_str = obfuscate(store_max_str, obfuscd);


        headers.push_back(header_str);
        prod_list.push_back(prod_str);
        cons_list.push_back(cons_str);
        store_max_list.push_back(store_max_str);

        //ImGui::Text(res.c_str());
    }

    for(int i=0; i<headers.size(); i++)
    {
        std::string header_formatted = format(headers[i], headers);
        std::string prod_formatted = format(prod_list[i], prod_list);
        std::string cons_formatted = format(cons_list[i], cons_list);
        std::string store_max_formatted = format(store_max_list[i], store_max_list);

        std::string display = header_formatted + ": " + prod_formatted + " | " + cons_formatted + " | " + store_max_formatted;

        ImGui::Text(display.c_str());
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

    std::string cost_str = "Cost: " + std::to_string((int)cost);

    ImGui::Text(cost_str.c_str());

    if(ImGui::IsItemHovered() && s.fully_disabled())
    {
        ImGui::SetTooltip("Amount recovered through salvage");
    }
    ///have a recovery cost display?

    if(s.owned_by->parent_empire == player_empire)
    {
        ImGui::Text("(Upgrade to latest Tech)");

        ship c_cpy = s;

        if(s.owned_by->parent_empire != nullptr)
            c_cpy.set_tech_level_from_empire(s.owned_by->parent_empire);

        //c_cpy.intermediate_texture = nullptr;

        auto res_cost = c_cpy.resources_cost();

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

                s.set_tech_level_from_empire(owner);
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

            owner->release_ownership(parent_fleet);

            orbital_system* os = system_manage.get_by_element(parent_fleet);

            orbital* o = os->get_by_element(parent_fleet);

            assert(o);

            owner->release_ownership(o);
            claiming_empire->take_ownership(o);

            ship_manager* new_sm = fleet_manage.make_new();

            new_sm->steal(&s);

            claiming_empire->take_ownership(new_sm);

            o->data = new_sm;

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
    ImGui::Begin("Ongoing Battles");

    for(int i=0; i<all_battles.battles.size(); i++)
    {
        std::string id_str = std::to_string(i);

        battle_manager* bm = all_battles.battles[i];

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
                ImGui::SetTooltip("Warning, this fleet will take heavy damage!");
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

void debug_system(system_manager& system_manage, sf::RenderWindow& win, bool lclick, bool rclick, popup_info& popup, empire* player_empire, all_battles_manager& all_battles, fleet_manager& fleet_manage)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    sf::Keyboard key;

    bool lshift = key.isKeyPressed(sf::Keyboard::LShift);

    if(lclick && !lshift && system_manage.in_system_view())
    {
        popup.going = false;

        popup.elements.clear();
    }

    std::vector<orbital*> selected;

    bool first = true;

    for(orbital_system* sys : system_manage.systems)
    {
        //if(!system_manage.in_system_view())
        //    continue;

        bool term = false;

        for(orbital* orb : sys->orbitals)
        {
            ///not necessarily valid
            ship_manager* sm = (ship_manager*)orb->data;

            orb->clicked = false;

            if(system_manage.in_system_view() && orb->point_within({transformed.x, transformed.y}) && system_manage.currently_viewed == sys)
            {
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
                    popup.rem(orb);
                    break;
                }

                if(lclick && popup.fetch(orb) == nullptr)
                {
                    popup.going = true;

                    ///do buttons here
                    popup_element elem;
                    elem.element = orb;

                    ///disabling merging here and resupply invalides all fleet actions except moving atm
                    ///unexpected fix to fleet merging problem
                    ///disable resupply if in combat
                    if(orb->type == orbital_info::FLEET && (orb->parent_empire == player_empire || orb->parent_empire->is_allied(player_empire)))
                    {
                        if(orb->parent_empire == player_empire)
                            elem.mergeable = true;

                        elem.buttons_map[popup_element_type::RESUPPLY] = {"Resupply", false};
                    }

                    if(orb->type == orbital_info::FLEET)
                    {
                        elem.toggle_clickable = true;
                    }

                    popup.elements.push_back(elem);

                    term = true;
                }

                if(orb->type == orbital_info::STAR)
                {
                    ImGui::SetTooltip((orb->name + "\n" + orb->get_empire_str() + orb->description).c_str());
                }

                if(orb->is_resource_object)
                {
                    std::string res_first = orb->name;

                    res_first += "\n" + orb->get_empire_str();

                    if(orb->ever_viewed)
                        res_first += orb->produced_resources_ps.get_formatted_str();

                    ImGui::SetTooltip(res_first.c_str());
                }
            }

            ///dis me
            if(popup.fetch(orb) != nullptr)
            {
                orbital_system* parent_system = sys;

                popup_element& elem = *popup.fetch(orb);

                ///if orb not a fleet, this is empty
                std::vector<orbital*> hostile_fleets = parent_system->get_fleets_within_engagement_range(orb);

                ///atm battle just immediately ends if no enemies are hostile
                ///need a declare war button perhaps?
                if(hostile_fleets.size() > 0 && orb->parent_empire == player_empire && orb->type == orbital_info::FLEET && sm->can_engage() && !sm->any_in_combat())
                {
                    elem.buttons_map[popup_element_type::ENGAGE].name = "Engage Fleets";
                }
                else
                {
                    elem.buttons_map.erase(popup_element_type::ENGAGE);
                }

                if(orb->parent_empire != nullptr && !orb->parent_empire->is_hostile(player_empire) && orb->parent_empire != player_empire)
                {
                    elem.buttons_map[popup_element_type::DECLARE_WAR].name = "Declare War";
                }
                else
                {
                    elem.buttons_map.erase(popup_element_type::DECLARE_WAR);
                }

                if(orb->type == orbital_info::FLEET && !sm->can_engage() && sm->parent_empire == player_empire)
                {
                    elem.buttons_map[popup_element_type::ENGAGE_COOLDOWN].name = sm->get_engage_str();
                }
                else
                {
                    elem.buttons_map.erase(popup_element_type::ENGAGE_COOLDOWN);
                }

                if(orb->type == orbital_info::FLEET && (sm->any_in_combat() || sm->all_derelict()))
                {
                    elem.buttons_map.erase(popup_element_type::RESUPPLY);
                }

                if(orb->type == orbital_info::PLANET && player_empire->can_colonise(orb) && fleet_manage.nearest_free_colony_ship_of_empire(orb, player_empire) != nullptr)
                {
                    elem.buttons_map[popup_element_type::COLONISE].name = "Colonise";
                }
                else
                {
                    elem.buttons_map.erase(popup_element_type::COLONISE);
                }

                selected.push_back(orb);
            }

            if(term)
                break;
        }
    }

    if(selected.size() > 0 && popup.going)
    {
        if(system_manage.hovered_system != nullptr)
        {
            for(orbital* o : selected)
            {
                if(o->type != orbital_info::FLEET)
                    continue;

                orbital_system* parent = system_manage.get_parent(o);

                if(parent == system_manage.hovered_system || parent == nullptr)
                    continue;

                ship_manager* sm = (ship_manager*)o->data;

                if(!sm->can_warp(system_manage.hovered_system, parent, o))
                {
                    ImGui::SetTooltip("Cannot Warp");
                }
                else
                {
                    ImGui::SetTooltip("Right click to Warp");
                }

                if(rclick && sm->parent_empire == player_empire)
                {
                    sm->try_warp(system_manage.hovered_system, parent, o);
                }
            }
        }

        if(system_manage.in_system_view())
        {
            for(orbital* kk : selected)
            {
                popup_element* elem = popup.fetch(kk);

                if(elem == nullptr)
                    continue;

                bool do_obfuscate = false;

                if(kk->type == orbital_info::FLEET)
                {
                    if(player_empire->available_scanning_power_on((ship_manager*)kk->data, system_manage) <= 0 && !player_empire->is_allied(kk->parent_empire))
                    {
                        do_obfuscate = true;
                    }
                }

                elem->header = kk->name;

                if(elem->header == "")
                    elem->header = orbital_info::names[kk->type];

                if(do_obfuscate)
                {
                    elem->header = "Unknown";
                }

                if(!do_obfuscate && kk->parent_empire != nullptr)
                {
                    elem->header = elem->header + "\n" + kk->get_empire_str(false);
                }

                if(do_obfuscate && kk->parent_empire != nullptr)
                {
                    if(kk->parent_empire != nullptr)
                    {
                        elem->header = elem->header + "\nEmpire: Unknown";
                    }
                }

                elem->data = kk->get_info_str();

                if(kk->description != "" && kk->ever_viewed)
                    elem->data.push_back(kk->description);

                if(do_obfuscate)
                {
                    //elem->header = obfuscate(elem->header, true);

                    for(auto& i : elem->data)
                    {
                        i = obfuscate(i, true);
                    }
                }

                if(popup.going)
                {
                    kk->highlight = true;

                    ship_manager* sm = (ship_manager*)kk->data;

                    if(rclick && (kk->type == orbital_info::FLEET) && kk->parent_empire == player_empire && kk->parent_system == system_manage.currently_viewed && !sm->any_colonising())
                    {
                        kk->request_transfer({transformed.x, transformed.y});
                    }
                }
            }
        }
    }

    for(popup_element& elem : popup.elements)
    {
        //for(auto& map_element : elem.buttons_map)

        for(auto iter = elem.buttons_map.begin(); iter != elem.buttons_map.end(); iter++)
        {
            auto map_element = *iter;

            ///this seems the least hitler method we have so far to handle this
            if(map_element.first == popup_element_type::RESUPPLY && map_element.second.pressed)
            {
                orbital* o = (orbital*)elem.element;

                ship_manager* sm = (ship_manager*)o->data;

                sm->resupply(player_empire, false);
            }

            if(map_element.first == popup_element_type::ENGAGE && map_element.second.pressed)
            {
                orbital* o = (orbital*)elem.element;

                assert(o);

                ship_manager* sm = (ship_manager*)o->data;

                assert(sm);

                orbital_system* parent = o->parent_system;

                assert(parent);

                std::vector<orbital*> hostile_fleets = parent->get_fleets_within_engagement_range(o);

                hostile_fleets.push_back(o);

                all_battles.make_new_battle(hostile_fleets);
            }

            if(map_element.first == popup_element_type::COLONISE && map_element.second.pressed)
            {
                orbital* o = (orbital*)elem.element;

                assert(o);

                ship* nearest = fleet_manage.nearest_free_colony_ship_of_empire(o, player_empire);

                if(!nearest)
                    continue;

                nearest->colonising = true;
                nearest->colonise_target = o;
            }


            if(map_element.first == popup_element_type::DECLARE_WAR && map_element.second.pressed)
            {
                elem.buttons_map[popup_element_type::DECLARE_WAR_SURE].name = "Are you sure?";
            }

            bool declaring = false;

            if(map_element.first == popup_element_type::DECLARE_WAR_SURE && map_element.second.pressed)
            {
                declaring = true;

                orbital* o = (orbital*)elem.element;

                assert(o);

                empire* parent = o->parent_empire;

                if(!parent->is_hostile(player_empire))
                {
                    parent->become_hostile(player_empire);
                }
            }

            if(declaring)
            {
                elem.buttons_map.erase(popup_element_type::DECLARE_WAR_SURE);

                iter--;
            }
        }
    }
}

void do_popup(popup_info& popup, fleet_manager& fleet_manage, system_manager& all_systems, orbital_system* current_system, empire_manager& empires, empire* player_empire, all_events_manager& all_events)
{
    if(popup.elements.size() == 0)
        popup.going = false;

    if(!popup.going)
        return;

    ImGui::Begin(("Selected:###INFO_PANEL"), nullptr, ImVec2(0,0), -1.f, ImGuiWindowFlags_AlwaysAutoResize);

    std::set<ship*> potential_new_fleet;

    sf::Keyboard key;

    int g_elem_id = 0;

    ///remember we'll need to make an orbital associated with the new fleet
    ///going to need the ability to drag and drop these
    ///nah use checkboxes
    for(popup_element& i : popup.elements)
    {
        int id = i.id;

        ImGui::Text(i.header.c_str());

        int num = 0;

        i.checked.resize(i.data.size());

        //for(std::string& str : i.data)
        for(int kk=0; kk < i.data.size(); kk++)
        {
            const std::string& str = i.data[kk];

            ImGui::Text(str.c_str());

            bool lshift = key.isKeyPressed(sf::Keyboard::LShift);

            bool shift_clicked = ImGui::IsItemClicked() && lshift;
            bool non_shift_clicked = ImGui::IsItemClicked() && !lshift;
            bool hovered = ImGui::IsItemHovered();

            bool can_open_window = i.mergeable;

            if(i.toggle_clickable)
            {
                orbital* o = (orbital*)i.element;

                ship_manager* smanage = (ship_manager*)o->data;

                ship* s = smanage->ships[num];

                if(s->fully_disabled())
                {
                    can_open_window = true;
                }

                float scanning_capacity = player_empire->available_scanning_power_on(s, all_systems);

                ///um. We probably want to adjust the scanner thing to return levels, not rando floats
                if(scanning_capacity > 0.4f)
                {
                    can_open_window = true;
                }

                if(smanage->parent_empire->is_allied(player_empire))
                {
                    can_open_window = true;
                }
            }

            if(i.toggle_clickable)
            {
                ImGui::SameLine();
                //ImGui::Checkbox(("###" + std::to_string(id) + std::to_string(num)).c_str(), &i.checked[kk]);

                std::string label_str = "";

                if(i.checked[kk])
                    label_str += "+";

                ImGui::Text((label_str).c_str());

                if(((ImGui::IsItemClicked() && lshift) || shift_clicked) && i.mergeable)
                {
                    i.checked[kk] = !i.checked[kk];
                }

                ///and sufficient information
                if(non_shift_clicked && can_open_window)
                {
                    orbital* o = (orbital*)i.element;

                    ship_manager* smanage = (ship_manager*)o->data;

                    smanage->ships[num]->display_ui = !smanage->ships[num]->display_ui;
                }

                if(ImGui::IsItemHovered() || hovered)
                {
                    if(can_open_window)
                        ImGui::SetTooltip("Left Click to view fleet");
                    if(i.mergeable)
                        ImGui::SetTooltip("Shift-Click to add to fleet");
                }

                if(i.checked[kk])
                {
                    orbital* o = (orbital*)i.element;

                    ship_manager* smanage = (ship_manager*)o->data;

                    potential_new_fleet.insert(smanage->ships[num]);
                }

                num++;
            }
        }

        int kk = 0;

        for(auto& map_element : i.buttons_map)
        {
            map_element.second.pressed = ImGui::Button((map_element.second.name + "##" + std::to_string(kk) + std::to_string(g_elem_id)).c_str());

            kk++;
        }

        orbital* o = (orbital*)i.element;

        if(o->has_quest_alert)
        {
            orbital* o = (orbital*)i.element;

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

        g_elem_id++;
    }

    if(potential_new_fleet.size() > 0)
    {
        if(ImGui::Button("Make New Fleet"))
        {
            bool bad = false;

            ship* test_ship = (*potential_new_fleet.begin());

            orbital_system* test_parent = all_systems.get_by_element(test_ship->owned_by);

            for(ship* i : potential_new_fleet)
            {
                if(all_systems.get_by_element(i->owned_by) != test_parent)
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

                    ns->steal(i);
                }

                orbital* associated = current_system->make_new(orbital_info::FLEET, 5.f);
                associated->parent = current_system->get_base();
                associated->set_orbit(fleet_pos);
                associated->data = ns;

                parent->take_ownership(associated);
                parent->take_ownership(ns);

                popup.elements.clear();
                popup.going = false;
            }
            else
            {
                printf("Tried to create fleets cross systems\n");
            }
        }
    }

    all_systems.cull_empty_orbital_fleets(empires);
    fleet_manage.cull_dead(empires);
    all_systems.cull_empty_orbital_fleets(empires);
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

                ship* new_ship = new_fleet->make_new_from(player_empire->team_id, test_ship);
                new_ship->name = "SS Toimplement name generation";

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

    view.zoom(system_manage.zoom_level);
    view.setCenter({system_manage.camera.x(), system_manage.camera.y()});

    window.setView(view);
}

int main()
{
    randf_s();
    randf_s();

    //music playing_music;

    empire_manager empire_manage;

    empire* player_empire = empire_manage.make_new();
    player_empire->name = "Glorious Azerbaijanian Conglomerate";

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

    ship* test_ship = fleet1->make_new_from(player_empire->team_id, make_default());
    ship* test_ship3 = fleet1->make_new_from(player_empire->team_id, make_default());

    ship* test_ship2 = fleet2->make_new_from(hostile_empire->team_id, make_default());

    test_ship2->set_tech_level_from_empire(hostile_empire);

    ship* test_ship4 = fleet3->make_new_from(player_empire->team_id, make_default());

    ship* derelict_ship = fleet4->make_new_from(hostile_empire->team_id, make_default());

    ship* scout_ship = fleet3->make_new_from(player_empire->team_id, make_colony_ship());
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

    window.create(sf::VideoMode(1200, 900),"Wowee", sf::Style::Default, settings);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

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

    int state = 0;

    bool focused = true;

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

            if(event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
                scrollwheel_delta += event.mouseWheelScroll.delta;

            if(event.type == sf::Event::Resized)
            {
                int x = event.size.width;
                int y = event.size.height;

                window.create(sf::VideoMode(x, y), "Wowee", sf::Style::Default, settings);
            }
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

        if(!focused)
            cdir = 0.f;

        system_manage.pan_camera(cdir * diff_s * 300);

        if(no_suppress_mouse)
            system_manage.change_zoom(scrollwheel_delta);

        ///BATTLE MANAGER IS NO LONGER TICKING SHIP COMPONENTS, IT IS ASSUMED TO BE DONE GLOBALLY WHICH WE WONT WANT
        ///WHEN BATTLES ARE SEPARATED FROM GLOBAL TIME
        /*if(key.isKeyPressed(sf::Keyboard::Num1))
        {

        }*/

        all_battles.tick(diff_s, system_manage);

        handle_camera(window, system_manage);

        if(once<sf::Keyboard::M>() && focused)
        {
            system_manage.enter_universe_view();
        }

        if(once<sf::Keyboard::F1>())
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

        if(once<sf::Keyboard::F3>())
        {
            system_manage.set_viewed_system(sys_2);
            state = 0;
        }

        /*if(once<sf::Keyboard::F2>())
        {
            test_ship->resupply(*player_empire);
        }*/

        bool lclick = once<sf::Mouse::Left>() && no_suppress_mouse;
        bool rclick = once<sf::Mouse::Right>() && no_suppress_mouse;

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
            debug_system(system_manage, window, lclick, rclick, popup, player_empire, all_battles, fleet_manage);

            ///this is slow
            system_manage.draw_viewed_system(window, player_empire);
            ///dis kinda slow too
            system_manage.draw_universe_map(window, player_empire);
            system_manage.process_universe_map(window, lclick);
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

        do_popup(popup, fleet_manage, system_manage, system_manage.currently_viewed, empire_manage, player_empire, all_events);

        //printf("precull\n");

        empire_manage.tick_cleanup_colonising();
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);
        system_manage.cull_empty_orbital_fleets(empire_manage);
        fleet_manage.cull_dead(empire_manage);


        if(state != 1)
            system_manage.draw_alerts(window, player_empire);

        ///this is slow
        fleet_manage.tick_all(diff_s);

        //printf("predrawres\n");

        player_empire->resources.draw_ui(window);
        //printf("Pregen\n");


        empire_manage.generate_resources_from_owned(diff_s);


        player_empire->draw_ui();
        ///this is slow
        //sf::Clock tclk;
        empire_manage.tick_all(diff_s, all_battles, system_manage, fleet_manage);
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
