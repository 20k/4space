#include <iostream>
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include "../../render_projects/imgui/imgui.h"
#include "../../render_projects/imgui/imgui-SFML.h"
#include <iomanip>

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

component make_default_crew()
{
    component_attribute command;
    command.produced_per_s = 1.f;

    component_attribute oxygen;
    oxygen.drained_per_s = 0.5f;
    oxygen.max_amount = 5.f;

    component_attribute repair;
    repair.produced_per_s = 0.2f;

    component crew;
    crew.add(ship_component_element::COMMAND, command);
    crew.add(ship_component_element::OXYGEN, oxygen);
    crew.add(ship_component_element::REPAIR, repair);

    return crew;
}

component make_default_life_support()
{
    component_attribute oxygen;
    oxygen.produced_per_s = 10.f;

    component_attribute power;
    power.drained_per_s = 5.f;
    power.max_amount = 10.f;

    component life_support;
    life_support.add(ship_component_element::OXYGEN, oxygen);
    life_support.add(ship_component_element::ENERGY, power);

    return life_support;
}

component make_default_ammo_store()
{
    component_attribute ammo;
    ammo.max_amount = 1000.f;

    component ammo_store;
    ammo_store.add(ship_component_element::AMMO, ammo);

    return ammo_store;
}

component make_default_shields()
{
    component_attribute shield;
    shield.produced_per_s = 1.f;
    shield.max_amount = 50.f;

    component_attribute power;
    power.drained_per_s = 20.f;

    component shields;
    shields.add(ship_component_element::SHIELD_POWER, shield);
    shields.add(ship_component_element::ENERGY, power);

    return shields;
}

component make_default_power_core()
{
    component_attribute power;
    power.produced_per_s = 100.f;
    power.max_amount = 20.f;

    component core;
    core.add(ship_component_element::ENERGY, power);

    return core;
}

ship make_default()
{
    /*ship test_ship;

    component_attribute powerplanet_core;
    powerplanet_core.produced_per_s = 1;

    component_attribute energy_sink;
    energy_sink.drained_per_s = 0.1f;
    energy_sink.max_amount = 50.f;

    component_attribute energy_sink2;
    energy_sink2.drained_per_s = 0.2f;
    energy_sink2.max_amount = 50.f;

    component_attribute powerplant_core2;
    powerplant_core2.produced_per_s = 2;

    component powerplant;
    powerplant.add(ship_component_element::ENERGY, powerplanet_core);

    component powerplant2;
    powerplant2.add(ship_component_element::ENERGY, powerplant_core2);

    component battery;
    battery.add(ship_component_element::ENERGY, energy_sink);

    component battery2;
    battery2.add(ship_component_element::ENERGY, energy_sink2);

    test_ship.add(powerplant);
    test_ship.add(battery);
    test_ship.add(battery2);
    test_ship.add(powerplant2);*/

    ship test_ship;
    test_ship.add(make_default_crew());
    test_ship.add(make_default_life_support());
    test_ship.add(make_default_ammo_store());
    test_ship.add(make_default_shields());
    test_ship.add(make_default_power_core());

    return test_ship;
}

///vertical list of attributes, going horizontally with separators between components
///we'll also want a systems level version of this, ie overall not specific
std::vector<std::string> get_components_display_string(ship& s)
{
    std::vector<component> components = s.entity_list;

    std::vector<std::string> display_str;

    for(component& c : components)
    {
        std::string component_str;

        int num = 0;

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

            std::string use_string = "Net Available (s): " + to_string_with_precision(net_usage, 3);
            std::string per_use_string = "Net Available (per use): " + to_string_with_precision(net_per_use, 3);

            std::string time_str = "Time Between Uses (s): " + to_string_with_precision(time_between_uses, 3);
            std::string left_str = "Time Till Next Use (s): " + to_string_with_precision(time_until_next_use, 3);

            std::string mamount_str = "Max Storage: " + to_string_with_precision(max_amount, 3);
            std::string camount_str = "Current Storage: " + to_string_with_precision(cur_amount, 3);

            std::string efficiency_str = "Curr Efficiency %: " + to_string_with_precision(attr.cur_efficiency, 3);

            component_str += header;

            if(net_usage != 0)
                component_str += "\n" + use_string;

            if(net_per_use != 0)
                component_str += "\n" + per_use_string;

            if(time_between_uses > 0)
                component_str += "\n" + time_str;

            if(time_until_next_use > 0)
                component_str += "\n" + left_str;

            if(max_amount > 0)
                component_str += "\n" + mamount_str;

            ///not a typo
            if(max_amount > 0)
                component_str += "\n" + camount_str;

            component_str += "\n" + efficiency_str;


            if(num != c.components.size() - 1)
            {
                component_str += "\n";
            }

            num++;
        }

        display_str.push_back(component_str);
    }

    return display_str;
}

void display_ship_info(ship& s)
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

int main()
{
    ship test_ship = make_default();

    /*test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);
    test_ship.tick_all_components(1.f);*/

    sf::RenderWindow window;

    window.create(sf::VideoMode(800, 600),"Wowee");

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    sf::Clock clk;

    float last_time_s = 0.f;
    float diff_s = 0.f;

    sf::Keyboard key;

    while(window.isOpen())
    {
        sf::Event event;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

            if(event.type == sf::Event::Closed)
                window.close();
        }


        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        display_ship_info(test_ship);

        debug_menu({&test_ship});

        ImGui::Render();
        window.display();
        window.clear();

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
