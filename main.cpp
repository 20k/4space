#include <iostream>
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include "../../render_projects/imgui/imgui.h"
#include "../../render_projects/imgui/imgui-SFML.h"
#include <iomanip>
#include "battle_manager.hpp"
#include <set>
#include "system_manager.hpp"

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

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

template<typename T>
std::string to_string_with_variable_prec(const T a_value)
{
    int n = ceil(log10(a_value)) + 1;

    if(a_value <= 0.0001f)
        n = 2;

    if(n < 2)
        n = 2;

    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
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

        std::string storage_str = "(" + to_string_with_variable_prec(cur_amount) + "/" + to_string_with_variable_prec(max_amount) + ")";

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

std::string format(std::string to_format, const std::vector<std::string>& all_strings)
{
    int len = 0;

    for(auto& i : all_strings)
    {
        if(i.length() > len)
            len = i.length();
    }

    for(int i=to_format.length(); i<len; i++)
    {
        to_format = to_format + " ";
    }

    return to_format;
}

void display_ship_info(ship& s, float step_s)
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

    ImGui::Begin("Test");

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

        std::string prod_str = "+" + to_string_with_precision(prod, 3);
        std::string cons_str = "-" + to_string_with_precision(cons, 3);

        std::string store_max_str;

        if(maximum > 0)
            store_max_str += "(" + to_string_with_variable_prec(store) + "/" + to_string_with_variable_prec(maximum) + ")";

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

    ImGui::Begin("Test");

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

void debug_battle(battle_manager& battle, sf::RenderWindow& win)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    ship* s = battle.get_ship_under({x, y});

    if(s)
    {
        s->highlight = true;
    }

    ImGui::Begin("Battle DBG");

    if(ImGui::Button("Step Battle 1s"))
    {
        battle.tick(1.f);
    }

    ImGui::End();
}

void debug_system(system_manager& system_manage, sf::RenderWindow& win)
{

}

int main()
{
    ship test_ship = make_default();
    ship test_ship2 = make_default();

    test_ship.team = 0;
    test_ship2.team = 1;

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

    battle.add_ship(&test_ship);
    battle.add_ship(&test_ship2);

    system_manager system_manage;

    orbital_system* base = system_manage.make_new();

    orbital* sun = base->make_new(orbital_info::STAR, 20.f);
    orbital* planet = base->make_new(orbital_info::PLANET, 5.f);
    planet->parent = sun;

    sun->absolute_pos = {500, 500};
    sun->rotation_velocity_ps = 2*M_PI/60.f;

    planet->orbital_length = 150.f;
    planet->angular_velocity_ps = 2*M_PI / 100.f;
    planet->rotation_velocity_ps = 2*M_PI / 200.f;

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

        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        display_ship_info(test_ship, diff_s);

        debug_menu({&test_ship});

        if(state == 1)
        {
            debug_battle(battle, window);
            battle.draw(window);
        }
        if(state == 0)
        {
            debug_system(system_manage, window);
            base->draw(window);
        }

        system_manage.tick(diff_s);

        ImGui::Render();
        window.display();
        window.clear();

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
