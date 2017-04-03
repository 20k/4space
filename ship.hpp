#ifndef SHIP_HPP_INCLUDED
#define SHIP_HPP_INCLUDED

#include <map>
#include <vector>
#include <vec/vec.hpp>

#define FLOAT_BOUND 0.00000001f

struct positional
{
    ///game map etc, temporary atm
    vec2f world_pos;
    float world_rot = 0;

    ///battles
    vec2f local_pos;
    float local_rot = 0;

    vec2f dim;
};

///we need components to require other components to function
///we need a tag system, for eg RECOIL or DAMAGE or ARC_OF_FIRE, ie isolated attributes
namespace ship_component_elements
{
    enum types
    {
        COOLING_POTENTIAL, ///active heat management, but we'll also lose heat into space proportionally to the temperature difference
        ENERGY,
        OXYGEN,
        AMMO,
        FUEL,
        CARGO,
        SHIELD_POWER, ///ie a system can produce shield power, or scanning power
        ARMOUR, ///second layer
        HP, ///last layer, raw health for components
        ENGINE_POWER,
        WARP_POWER,
        SCANNING_POWER,
        //PROJECTILE, ///outputting damage. Have a receive damage as well component? Integrity AND hp?
        COMMAND, ///ie the ability for the ship to control itself, limiter on the complexity of stuff in it
        STEALTH,
        REPAIR, ///this is dumb, just make crew produce hp
        RAILGUN,
        TORPEDO,
        PLASMAGUN,
        COILGUN,
        NONE,
    };

    static std::vector<std::string> display_strings
    {
        "COOLING_POTENTIAL",
        "ENERGY",
        "OXYGEN",
        "AMMO",
        "FUEL",
        "CARGO",
        "SHIELD_POWER",
        "ARMOUR",
        "HP",
        "ENGINE_POWER",
        "WARP_POWER",
        "SCANNING_POWER",
        //"PROJECTILE",
        "COMMAND",
        "STEALTH",
        "REPAIR",
        "RAILGUN",
        "TORPEDO",
        "PLASMAGUN",
        "COILGUN",
    };
}

///misc tags
namespace component_tag
{
    enum tag
    {
        DAMAGE = 1,
        SPEED = 2,
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

    float cur_efficiency = 1.f;

    float add_amount(float amount);
    bool can_use();
    float get_available_capacity();
    float get_total_capacity(float step_s); ///including drain

    float get_produced_amount(float step_s);

    //float get_net();

    void update_time(float step_s); ///resets drained
    //float drain(float amount); ///returns leftover, sets efficiency

    ///returns amount consumed
    float consume_from(component_attribute& other, float max_proportion, float step_s);
    float consume_from_amount_available(component_attribute& other, float amount, float step_s);
    float consume_from_amount_stored(component_attribute& other, float amount, float step_s);

    ///temporary storage so that other components can nab from me
    float available_for_consumption = 0;

    float consume_max(float amount);
    float consume_max_available(float amount);
    float consume_max_stored(float amount);

    void calculate_efficiency(float step_s);

private:
    float currently_drained = 0.f;
};

std::map<ship_component_element, float> merge_diffs(const std::map<ship_component_element, float>& one, const std::map<ship_component_element, float>& two);

///ie what can things do
///this is a ship entity for the moment.. but could likely describe a character as well
///float get_current_functionality
struct component
{
    /*component_attribute heat;
    component_attribute energy;
    component_attribute oxygen;
    component_attribute scanning_power;
    component_attribute engine_power;
    component_attribute damage;
    component_attribute */

    std::string name = "";

    std::map<ship_component_element, component_attribute> components;
    std::map<component_tag::tag, float> tag_list;

    bool has_element(const ship_component_element& type);

    ///ie calculate all offsets
    std::map<ship_component_element, float> get_timestep_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_production_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_consumption_diff(float step_s);
    std::map<ship_component_element, float> get_stored();
    std::map<ship_component_element, float> get_use_diff();

    ///returns a pair of new component, extra resources left over
    //std::pair<component, std::map<ship_component_element, float>> apply_diff(const std::map<ship_component_element, float>& diff);

    std::map<ship_component_element, float> apply_diff(const std::map<ship_component_element, float>& available);

    void update_time(float step_s);

    ///how much *more* we can take
    std::map<ship_component_element, float> get_available_capacities();
    //std::map<ship_component_element, float> get_needed_resources(float time_s);
    std::map<ship_component_element, float> get_stored_and_produced_resources(float time_s);

    void add(ship_component_element element, const component_attribute& attr);

    float calculate_total_efficiency(float step_s);
    void propagate_total_efficiency(float step_s);

    ship_component_element get_weapon_type();

    bool has_tag(component_tag::tag tag);

    float get_tag(component_tag::tag tag);

    void set_tag(component_tag::tag tag, float val);
};

struct projectile;

struct ship : positional
{
    int team = 0;

    std::vector<component> entity_list;

    ///returns extra resources for us to handle
    std::map<ship_component_element, float> tick_all_components(float step_s);

    //std::map<ship_component_element, float> last_left_over;

    std::map<ship_component_element, float> get_available_capacities();
    /*std::map<ship_component_element, float> get_needed_resources();*/
    std::map<ship_component_element, float> get_produced_resources(float time_s);

    std::map<ship_component_element, float> get_stored_and_produced_resources(float time_s);
    std::map<ship_component_element, float> get_needed_resources(float time_s);
    std::map<ship_component_element, float> get_stored_resources();

    void distribute_resources(std::map<ship_component_element, float> res);
    void add_negative_resources(std::map<ship_component_element, float> res);

    bool can_use(component& c);
    void use(component& c);

    std::vector<component> fire();

    void add(const component& c);


    void hit(projectile* p);


    ///?
    //void fire();
};

#endif // SHIP_HPP_INCLUDED
