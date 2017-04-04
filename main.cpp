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

///so display this (ish) on mouseover for a component
std::string get_component_display_string(component& c)
{
    std::string component_str = c.name + ":\n";

    int num = 0;

    std::string eff = "";

    float efficiency = 1.f;

    if(c.components.begin() != c.components.end())
        efficiency = c.components.begin()->second.cur_efficiency * 100.f;

    if(efficiency < 99.9f)
        eff = "Efficiency %%: " + to_string_with_precision(efficiency, 3) + "\n";

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

        std::string use_string = "" + to_string_with_precision(net_usage, 3) + "/s";
        std::string per_use_string = "" + to_string_with_precision(net_per_use, 3) + "/use";

        //std::string time_str = "Time Between Uses (s): " + to_string_with_precision(time_between_uses, 3);
        //std::string left_str = "Time Till Next Use (s): " + to_string_with_precision(time_until_next_use, 3);

        std::string fire_time_remaining = "Time Left (s): " + to_string_with_precision(time_until_next_use, 3) + "/" + to_string_with_precision(time_between_uses, 3);

        //std::string mamount_str = "Max Storage: " + to_string_with_precision(max_amount, 3);
        //std::string camount_str = "Current Storage: " + to_string_with_precision(cur_amount, 3);

        std::string storage_str = "(" + to_string_with_enforced_variable_dp(cur_amount) + "/" + to_string_with_variable_prec(max_amount) + ")";

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


void display_ship_info(ship& s)
{
    auto produced = s.get_produced_resources(1.f); ///modified by efficiency, ie real amount consumed
    auto consumed = s.get_needed_resources(1.f); ///not actually consumed, but requested
    auto stored = s.get_stored_resources();
    auto max_res = s.get_max_resources();

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

    ImGui::Begin(s.name.c_str(), &s.display_ui);

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

        ImGui::TextColored({ccol.x(), ccol.y(), ccol.z(), 1.f}, name.c_str());

        if(ImGui::IsItemClicked())
        {
            c.clicked = !c.clicked;
        }

        if(c.clicked)
        {
            ImGui::Indent();

            ImGui::Text(get_component_display_string(c).c_str());
            ImGui::Unindent();
        }

        /*if(ImGui::CollapsingHeader(name.c_str()))
        {
            ImGui::Text(get_component_display_string(c).c_str());
        }*/

        c_id++;
    }

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

struct popup_element
{
    static int gid;
    int id = gid++;

    std::string header;
    std::vector<std::string> data;
    std::deque<bool> checked; ///screw you idiotic language
    bool mergeable = false;

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

void debug_menu(const std::vector<ship*>& ships)
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
}

void debug_battle(battle_manager& battle, sf::RenderWindow& win, bool lclick)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    ship* s = battle.get_ship_under({transformed.x, transformed.y});

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
        battle.tick(1.f);
    }

    ImGui::End();
}

void debug_system(system_manager& system_manage, sf::RenderWindow& win, bool lclick, bool rclick, popup_info& popup)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    sf::Keyboard key;

    bool lshift = key.isKeyPressed(sf::Keyboard::LShift);

    if(lclick && !lshift)
    {
        popup.going = false;

        popup.elements.clear();
    }

    std::vector<orbital*> selected;

    bool first = true;

    for(orbital_system* sys : system_manage.systems)
    {
        bool term = false;

        for(orbital* orb : sys->orbitals)
        {
            if(orb->point_within({transformed.x, transformed.y}))
            {
                if(first)
                {
                    orb->highlight = true;
                    first = false;
                }

                if(lclick && popup.fetch(orb))
                {
                    popup.rem(orb);
                    break;
                }

                if(lclick && popup.fetch(orb) == nullptr)
                {
                    popup.going = true;

                    popup_element elem;
                    elem.element = orb;

                    if(orb->type == orbital_info::FLEET)
                    {
                        elem.mergeable = true;
                    }

                    popup.elements.push_back(elem);

                    term = true;
                }
            }

            if(popup.fetch(orb) != nullptr)
            {
                selected.push_back(orb);
            }

            if(term)
                break;
        }
    }

    if(selected.size() > 0 && popup.going)
    {
        for(auto& kk : selected)
        {
            popup_element* elem = popup.fetch(kk);

            if(elem == nullptr)
                continue;

            elem->header = orbital_info::names[kk->type];

            elem->data = kk->get_info_str();

            if(popup.going)
            {
                kk->highlight = true;

                if(rclick && (kk->type == orbital_info::FLEET))
                {
                    kk->transfer({transformed.x, transformed.y});
                }
            }
        }

    }
}

void do_popup(popup_info& popup, fleet_manager& fleet_manage, system_manager& all_systems, orbital_system* current_system)
{
    if(popup.elements.size() == 0)
        popup.going = false;

    if(!popup.going)
        return;

    ImGui::Begin(("Selected:###INFO_PANEL"), nullptr, ImVec2(0,0), -1.f, ImGuiWindowFlags_AlwaysAutoResize);

    bool any_checkbox = false;

    std::set<ship*> potential_new_fleet;

    sf::Keyboard key;

    ///remember we'll need to make an orbital associated with the new fleet
    ///going to need the ability to drag and drop these
    ///nah use checkboxes
    for(auto& i : popup.elements)
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

            if(i.mergeable)
            {
                ImGui::SameLine();
                //ImGui::Checkbox(("###" + std::to_string(id) + std::to_string(num)).c_str(), &i.checked[kk]);

                std::string label_str = "";

                if(i.checked[kk])
                    label_str += "+";

                ImGui::Text((label_str).c_str());

                if((ImGui::IsItemClicked() && lshift) || shift_clicked)
                {
                    i.checked[kk] = !i.checked[kk];
                }

                if(non_shift_clicked)
                {
                    orbital* o = (orbital*)i.element;

                    ship_manager* smanage = (ship_manager*)o->data;

                    smanage->ships[num]->display_ui = !smanage->ships[num]->display_ui;
                }

                if(ImGui::IsItemHovered() || hovered)
                {
                    ImGui::SetTooltip("Shift-Click to add to fleet");
                }

                if(i.checked[kk])
                {
                    any_checkbox = true;

                    orbital* o = (orbital*)i.element;

                    ship_manager* smanage = (ship_manager*)o->data;

                    potential_new_fleet.insert(smanage->ships[num]);
                }

                num++;
            }

        }
    }

    if(potential_new_fleet.size() > 0)
    {
        if(ImGui::Button("Make New Fleet"))
        {
            ship_manager* ns = fleet_manage.make_new();

            vec2f fleet_pos;

            float fleet_angle = 0;
            float fleet_length = 200;

            for(ship* i : potential_new_fleet)
            {
                orbital* real = current_system->get_by_element((void*)i->owned_by);

                if(real != nullptr)
                {
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

            popup.elements.clear();
            popup.going = false;
        }
    }

    all_systems.cull_empty_orbital_fleets();
    fleet_manage.cull_dead();

    ImGui::End();
}

int main()
{
    empire player_empire;
    player_empire.name = "Glorious Azerbaijanian Conglomerate";

    fleet_manager fleet_manage;

    ship_manager* fleet1 = fleet_manage.make_new();
    ship_manager* fleet2 = fleet_manage.make_new();
    ship_manager* fleet3 = fleet_manage.make_new();

    //ship test_ship = make_default();
    //ship test_ship2 = make_default();

    ship* test_ship = fleet1->make_new_from(0, make_default());
    ship* test_ship3 = fleet1->make_new_from(0, make_default());

    ship* test_ship2 = fleet2->make_new_from(1, make_default());

    ship* test_ship4 = fleet3->make_new_from(0, make_default());

    test_ship->name = "SS Icarus";
    test_ship2->name = "SS Buttz";
    test_ship3->name = "SS Duplicate";
    test_ship4->name = "SS Secondary";

    /*test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);*/

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window;

    window.create(sf::VideoMode(1024, 768),"Wowee", sf::Style::Default, settings);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    sf::Clock clk;

    float last_time_s = 0.f;
    float diff_s = 0.f;

    battle_manager battle;

    battle.add_ship(test_ship);
    battle.add_ship(test_ship2);

    system_manager system_manage;

    orbital_system* base = system_manage.make_new();

    orbital* sun = base->make_new(orbital_info::STAR, 10.f, 10);
    orbital* planet = base->make_new(orbital_info::PLANET, 5.f, 10);
    planet->parent = sun;

    sun->absolute_pos = {500, 500};
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

    base->generate_asteroids(100, 3, 5);
    base->generate_planet_resources(2.f);

    player_empire.take_ownership_of_all(base);
    player_empire.take_ownership(fleet1);
    player_empire.take_ownership(fleet3);


    popup_info popup;

    sf::Keyboard key;

    int state = 0;

    while(window.isOpen())
    {
        sf::Event event;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

            if(event.type == sf::Event::Closed)
                window.close();
        }

        if(key.isKeyPressed(sf::Keyboard::Num1))
        {
            battle.tick(diff_s);

            //printf("Timestep %f\n", diff_s);
        }

        if(once<sf::Keyboard::F1>())
        {
            state = (state + 1) % 2;
        }

        bool lclick = once<sf::Mouse::Left>() && !ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow();
        bool rclick = once<sf::Mouse::Right>() && !ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow();

        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        //display_ship_info(*test_ship);

        debug_menu({test_ship});

        if(state == 1)
        {
            battle.set_view(window);
            debug_battle(battle, window, lclick);
            battle.draw(window);
        }
        if(state == 0)
        {
            sun->center_camera(window);
            debug_system(system_manage, window, lclick, rclick, popup);
            base->draw(window);
        }

        for(ship_manager* smanage : fleet_manage.fleets)
        {
            for(ship* s : smanage->ships)
            {
                if(s->display_ui)
                {
                    display_ship_info(*s);
                }
            }
        }

        system_manage.tick(diff_s);

        do_popup(popup, fleet_manage, system_manage, base);

        system_manage.cull_empty_orbital_fleets();
        fleet_manage.cull_dead();

        player_empire.resources.draw_ui(window);
        player_empire.generate_resource_from_owned(diff_s);

        ImGui::Render();
        window.display();
        window.clear();

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
