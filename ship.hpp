#ifndef SHIP_HPP_INCLUDED
#define SHIP_HPP_INCLUDED

#include <map>
#include <vector>

namespace ship_component_elements
{
    enum types
    {
        HEAT, ///active heat management, but we'll also lose heat into space proportionally to the temperature difference
        ENERGY,
        OXYGEN,
        AMMO,
        FUEL,
        CARGO,
        SHIELD_POWER, ///ie a system can produce shield power, or scanning power
        ENGINE_POWER,
        WARP_POWER,
        SCANNING_POWER,
        DAMAGE, ///outputting damage. Have a receive damage as well component? Integrity AND hp?
        COMMAND, ///ie the ability for the ship to control itself, limiter on the complexity of stuff in it
        STEALTH,
        REPAIR,
    };
}

using ship_component_element = ship_component_elements::types;

///tech level?
///use recharge time?
struct component_attribute
{
    float produced_per_s = 0;
    float produced_per_use = 0;

    float drained_per_s = 0;
    float drained_per_use = 0;

    float time_between_uses_s = 0;
    float time_last_used_s = 0;
    float current_time_s = 0;

    float max_amount = 0;
    float cur_amount = 0;

    float add_amount(float amount);
    bool can_use();
    float get_available_capacity();

    void update_time(float step_s);
};

std::map<ship_component_element, float> merge_diffs(const std::map<ship_component_element, float>& one, const std::map<ship_component_element, float>& two);



///ie what can things do
///this is a ship entity for the moment.. but could likely describe a character as well
struct component
{
    /*component_attribute heat;
    component_attribute energy;
    component_attribute oxygen;
    component_attribute scanning_power;
    component_attribute engine_power;
    component_attribute damage;
    component_attribute */

    std::map<ship_component_element, component_attribute> components;

    bool has_element(const ship_component_element& type);

    ///ie calculate all offsets
    std::map<ship_component_element, float> get_timestep_diff(float step_s);
    std::map<ship_component_element, float> get_use_diff();

    ///returns a pair of new component, extra resources left over
    std::pair<component, std::map<ship_component_element, float>> apply_diff(const std::map<ship_component_element, float>& diff);

    void update_time(float step_s);

    ///how much *more* we can take
    std::map<ship_component_element, float> get_available_capacities();

    void add(ship_component_element element, const component_attribute& attr);
};

struct ship
{
    std::vector<component> entity_list;

    ///returns extra resources for us to handle
    std::map<ship_component_element, float> tick_all_components(float step_s);

    std::map<ship_component_element, float> get_available_capacities();

    void add(const component& c);

    ///?
    //void fire();
};

#endif // SHIP_HPP_INCLUDED
