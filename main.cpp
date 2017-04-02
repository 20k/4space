#include <iostream>
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include "../../render_projects/imgui/imgui.h"
#include "../../render_projects/imgui/imgui-SFML.h"
#include <iomanip>
#include "battle_manager.hpp"

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

#include "ship_definitions.hpp"

///vertical list of attributes, going horizontally with separators between components
///we'll also want a systems level version of this, ie overall not specific
std::vector<std::string> get_components_display_string(ship& s)
{
    std::vector<component> components = s.entity_list;

    std::vector<std::string> display_str;

    for(component& c : components)
    {
        std::string component_str = c.name + ":\n";

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

            std::string use_string = "" + to_string_with_precision(net_usage, 3) + "/s";
            std::string per_use_string = "" + to_string_with_precision(net_per_use, 3) + "/use";

            std::string time_str = "Time Between Uses (s): " + to_string_with_precision(time_between_uses, 3);
            std::string left_str = "Time Till Next Use (s): " + to_string_with_precision(time_until_next_use, 3);

            //std::string mamount_str = "Max Storage: " + to_string_with_precision(max_amount, 3);
            //std::string camount_str = "Current Storage: " + to_string_with_precision(cur_amount, 3);

            std::string storage_str = "Storage: " + to_string_with_precision(cur_amount, 5) + "/" + to_string_with_precision(max_amount, 5);

            std::string efficiency_str = "Efficiency %%: " + to_string_with_precision(attr.cur_efficiency*100.f, 3);

            component_str += header;

            if(net_usage != 0)
                component_str += "\n" + use_string;

            if(net_per_use != 0)
                component_str += "\n" + per_use_string;

            if(time_between_uses > 0)
                component_str += "\n" + time_str;

            if(time_until_next_use > 0)
                component_str += "\n" + left_str;

            /*if(max_amount > 0)
                component_str += "\n" + mamount_str;*/

            ///not a typo
            /*if(max_amount > 0)
                component_str += "\n" + camount_str;*/

            if(max_amount > 0)
                component_str += "\n" + storage_str;

            if(attr.cur_efficiency < 0.99999f)
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

void debug_battle(battle_manager& battle)
{
    ImGui::Begin("Battle DBG");

    if(ImGui::Button("Step Battle 1s"))
    {
        battle.tick(1.f);
    }

    ImGui::End();
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

    sf::RenderWindow window;

    window.create(sf::VideoMode(1024, 768),"Wowee");

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    sf::Clock clk;

    float last_time_s = 0.f;
    float diff_s = 0.f;

    battle_manager battle;

    battle.add_ship(&test_ship);
    battle.add_ship(&test_ship2);

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

        if(key.isKeyPressed(sf::Keyboard::Num1))
        {
            battle.tick(diff_s);

            printf("Timestep %f\n", diff_s);
        }

        sf::Time t = sf::microseconds(diff_s * 1000.f * 1000.f);
        ImGui::SFML::Update(t);

        display_ship_info(test_ship);

        debug_menu({&test_ship});
        debug_battle(battle);
        battle.draw(window);

        ImGui::Render();
        window.display();
        window.clear();

        diff_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f - last_time_s;
        last_time_s = clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f;
    }

    //std::cout << test_ship.get_available_capacities()[ship_component_element::ENERGY] << std::endl;

    return 0;
}
