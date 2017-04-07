#ifndef SHIP_HPP_INCLUDED
#define SHIP_HPP_INCLUDED

#include <map>
#include <vector>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>
#include "resource_manager.hpp"
#include "research.hpp"

#define FLOAT_BOUND 0.00000001f

struct empire;

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
        //REPAIR, ///this is dumb, just make crew produce hp
        RAILGUN,
        TORPEDO,
        PLASMAGUN,
        COILGUN,
        NONE,
    };

    ///we could just take the inverse of cooling_potential when displaying
    ///might be more friendly for the player
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
        //"REPAIR",
        "RAILGUN",
        "TORPEDO",
        "PLASMAGUN",
        "COILGUN",
    };

    static std::vector<std::string> short_name
    {
        "CL",
        "EN",
        "OX",
        "AM",
        "FU",
        "CA",
        "SH",
        "AR",
        "HP",
        "EG",
        "WP",
        "SN",
        "CM",
        "ST",
        "RP",
        "WR",
        "WT",
        "WP",
        "WC",
    };

    static std::vector<float> base_cost_of_component_with_this_primary_attribute
    {
        3.f,
        15.f,
        5.f,
        2.f,
        1.f,
        1.f,
        20.f,
        4.f,
        0.5f,
        10.f,
        30.f,
        5.f,
        2.f,
        80.f,
        30.f,
        30.f,
        25.f,
        20.f
    };

    ///Ok... we might want more research types
    static std::vector<research_info::types> component_element_to_research_type
    {
        research_info::MATERIALS,
        research_info::MATERIALS,
        research_info::MATERIALS,
        research_info::MATERIALS,
        research_info::MATERIALS,
        research_info::MATERIALS,

        research_info::MATERIALS,
        research_info::MATERIALS,
        research_info::MATERIALS,

        research_info::PROPULSION,
        research_info::PROPULSION,

        research_info::SCANNERS,

        research_info::MATERIALS,
        research_info::MATERIALS,

        research_info::WEAPONS,
        research_info::WEAPONS,
        research_info::WEAPONS,
        research_info::WEAPONS,
    };

    std::map<resource::types, float> component_storage_to_resources(const types& type);
}

namespace combat_variables
{
    static float mandatory_combat_time_s = 30.f;
    static float disengagement_time_s = 30.f;
}

///misc tags
namespace component_tag
{
    enum tag
    {
        DAMAGE = 1,
        SPEED = 2,
        SCALE = 4, ///render scale
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

    float tech_level = 0.f;

    float add_amount(float amount);
    bool can_use();
    void use();
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

    void set_tech_level_from_empire(int type, empire* e);
    void set_tech_level(float level);
    float get_tech_level();

private:
    float currently_drained = 0.f;
};

std::map<ship_component_element, float> merge_diffs(const std::map<ship_component_element, float>& one, const std::map<ship_component_element, float>& two);

///ie what can things do
///this is a ship entity for the moment.. but could likely describe a character as well
///float get_current_functionality
///maybe component attributes should not have a tech level, but components overall
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
    component_attribute get_element(const ship_component_element& type);

    ///ie calculate all offsets
    std::map<ship_component_element, float> get_timestep_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_production_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_consumption_diff(float step_s);
    std::map<ship_component_element, float> get_stored();
    std::map<ship_component_element, float> get_stored_max();
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

    ///BLANKET SETS ALL SUB COMPONENTS TO TECH LEVEL
    void set_tech_level(float tech_level);
    void set_tech_level_from_empire(empire* e);
    void set_tech_level_of_element(ship_component_elements::types type, float tech_level);
    float get_tech_level_of_element(ship_component_elements::types type);

    float get_tech_level_of_primary();
    float get_base_component_cost(); ///static
    float get_component_cost(); ///static including tech
    ///hp is done FRACTIONALLY not in real terms
    float get_real_component_cost(); ///above including HP

    ///not including armour
    ///we need a get_real_resource_cost etc as well for current resource cost base
    ///not including HP
    std::map<resource::types, float> get_resource_cost();
    ///ie how much they're worth
    std::map<resource::types, float> resources_received_when_scrapped();
    std::map<resource::types, float> resources_needed_to_repair();

    ///how much research would empire emp get if they could
    research_category get_research_base_for_empire(empire* owner, empire* claiming_empire);
    research_category get_research_real_for_empire(empire* owner, empire* claiming_empire);

    ///for ui stuff. Its better to keep this internally in case we add new components
    bool clicked = false;

    bool skip_in_derelict_calculations = false;
    bool repair_this_when_recrewing = false; ///when you recrew a ship, repair this component

    ship_component_elements::types primary_attribute = ship_component_elements::NONE;

    ///does in * hp_cur / hp_max safely
    float safe_hp_frac_modify(float in);
};

struct projectile;
struct ship_manager;
struct empire;
struct orbital;

struct ship : positional
{
    std::string name;

    int id = gid++;
    static int gid;

    int team = 0;

    std::vector<component> entity_list;

    ///returns extra resources for us to handle
    std::map<ship_component_element, float> tick_all_components(float step_s);
    void tick_other_systems(float step_s);
    research tick_drain_research_from_crew(float step_s);

    void tick_combat(float step_s);
    void enter_combat();
    void leave_combat();
    bool in_combat();

    research get_crewing_research(float step_s);

    //std::map<ship_component_element, float> last_left_over;

    std::map<ship_component_element, float> get_available_capacities();
    /*std::map<ship_component_element, float> get_needed_resources();*/
    std::map<ship_component_element, float> get_produced_resources(float time_s);

    std::map<ship_component_element, float> get_stored_and_produced_resources(float time_s);
    std::map<ship_component_element, float> get_needed_resources(float time_s);
    std::map<ship_component_element, float> get_stored_resources();
    std::map<ship_component_element, float> get_max_resources();

    void distribute_resources(std::map<ship_component_element, float> res);
    void add_negative_resources(std::map<ship_component_element, float> res);

    bool can_use(component& c);
    void use(component& c);

    std::vector<component> fire();
    bool can_use_warp_drives();
    void use_warp_drives();

    void add(const component& c);

    void hit(projectile* p);

    void check_load(vec2i dim);
    void generate_image(vec2i dim);

    ~ship();

    sf::Image img;
    sf::Texture tex;

    bool is_loaded = false;

    bool highlight = false;

    ship_manager* owned_by = nullptr;

    bool display_ui = false;

    void resupply(empire& emp, int num = 1);

    bool can_move_in_system();
    float get_move_system_speed();

    void apply_disengage_penalty();
    bool can_disengage();
    bool can_engage();

    float disengage_clock_s = 9999.f;
    bool is_disengaging = false;

    float time_in_combat_s = 0.f;
    bool currently_in_combat = false;

    ///what does this represent? That the crew is dead?
    ///For the moment it means that the ship won't autorepair
    bool fully_disabled();
    void force_fully_disabled(bool disabled);
    void test_set_disabled();

    bool is_fully_disabled = false;

    void set_tech_level_of_component(int component_offset, float tech_level);
    void set_tech_level_from_empire(empire* e);

    ///damages the ship until derelict
    void randomise_make_derelict();

    float get_total_cost();
    float get_real_total_cost();

    std::map<resource::types, float> resources_needed_to_recrew_total();
    std::map<resource::types, float> resources_needed_to_repair_total();
    ///calculate research separately as it needs empire, both sides
    std::map<resource::types, float> resources_received_when_scrapped();

    ///requires raw research, not tech currency
    research get_research_base_for_empire(empire* owner, empire* claiming);
    research get_research_real_for_empire(empire* owner, empire* claiming);

    research get_recrew_potential_research(empire* claiming);

    void recrew_derelict(empire* owner, empire* claiming);
    bool can_recrew(empire* claiming);

    bool cleanup = false;

    ///If we crew a ship, we get research gradually from the experience
    research research_left_from_crewing;
    bool is_alien = false;
    float crew_effectiveness = 1.f;

    empire* original_owning_race = nullptr;

    std::map<empire*, research> past_owners_research_left;

    ///1 = total information, 0 = absolutely none
    float get_scanning_power_on(orbital* o);
    float get_scanning_power_on_ship(ship* s);

private:
    sf::RenderTexture* intermediate_texture = nullptr;

    ///?
    //void fire();
};

struct orbital_system;
struct orbital;

///can be used as a fleet
struct ship_manager
{
    std::vector<ship*> ships;

    ship* make_new(int team);
    ship* make_new_from(int team, const ship& s);

    void destroy(ship* s);

    std::vector<std::string> get_info_strs();

    void merge_into_me(ship_manager& other);

    ///from other ship manager
    void steal(ship* const s);

    empire* parent_empire = nullptr;

    void resupply();

    void tick_all(float step_s);

    bool can_move_in_system();
    float get_move_system_speed();

    void draw_alerts(sf::RenderWindow& win, vec2f abs_pos);

    void try_warp(orbital_system* fin, orbital_system* cur, orbital* o);
    bool can_warp(orbital_system* fin, orbital_system* cur, orbital* o);

    void leave_combat();
    void enter_combat();
    bool any_in_combat();
    bool can_engage(); ///and be engaged

    void apply_disengage_penalty();

    void cull_invalid();

    std::string get_engage_str();
};

struct empire_manager;

///manages fleets
struct fleet_manager
{
    std::vector<ship_manager*> fleets;

    ship_manager* make_new();

    void destroy(ship_manager*);

    void cull_dead(empire_manager& empire_manage);

    void tick_all(float step_s);
};

#endif // SHIP_HPP_INCLUDED
