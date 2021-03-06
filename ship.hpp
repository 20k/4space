#ifndef SHIP_HPP_INCLUDED
#define SHIP_HPP_INCLUDED

#include <map>
#include <vector>
#include <array>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>
#include "resource_manager.hpp"
#include "research.hpp"
#include "ai_fleet.hpp"
#include "ai_empire.hpp"
#include "serialise.hpp"
#include "ui_util.hpp"

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

    virtual ~positional(){}
};

struct component_attribute;
struct component;

///we need components to require other components to function
///we need a tag system, for eg RECOIL or DAMAGE or ARC_OF_FIRE, ie isolated attributes
namespace ship_component_elements
{
    void generate_element_infos();

    enum tech_type
    {
        NONE_TECH = 0,
        LOW = 1,
        MEDIUM = 2,
        HIGH = 4,
        SUPER_HIGH = 8, ///whatever

        COMMON = 16,///iron + copper
        RARE = 32, /// + titanium
        HARD_RARE = 64, ///higher mix of rare elements
        LOW_VOLUME = 128, ///+uranium, does NOT imply hard rare or rare
        ALL_COMMON = 256,
    };

    ///hmm. This should really be reordered, its a shame I did this wrong initially
    ///as I'm not 100% sure its kosher to change htis
    enum types
    {
        HP, ///last layer, raw health for components
        ARMOUR, ///second layer
        SHIELD_POWER, ///ie a system can produce shield power, or scanning power
        COMMAND, ///ie the ability for the ship to control itself, limiter on the complexity of stuff in it
        ENERGY,
        COOLING_POTENTIAL, ///active heat management, but we'll also lose heat into space proportionally to the temperature difference
        ENGINE_POWER,
        AMMO,
        FUEL,
        CARGO,
        WARP_POWER,
        SCANNING_POWER,
        //PROJECTILE, ///outputting damage. Have a receive damage as well component? Integrity AND hp?
        STEALTH,
        //REPAIR, ///this is dumb, just make crew produce hp
        RAILGUN,
        TORPEDO,
        PLASMAGUN,
        COILGUN,
        COLONISER,
        RESOURCE_PRODUCTION,
        RESOURCE_STORAGE,
        SHIPYARD,
        MASS_ANCHOR, ///can be tied to a planetary body, sun, or asteroid etc
        ORE_HARVESTER, ///does not actually harvest ore, simply stores information about it. Do this through resource production
        RESOURCE_PULLER, ///pulls resources from empire at rate of produced_per_s

        ///RESOURCES SHOULD BE ENTIRELY MIGRATED TO SHIP COMPONENT ELEMENTS
        OXYGEN,
        COPPER,
        HYDROGEN,
        IRON,
        TITANIUM,
        URANIUM,
        RESEARCH,
        NONE,
    };

    struct component_element_info
    {
        float resource_rarity = 0.f; ///1 means common, 0 means cannot be found
        resource::types resource_type = resource::COUNT;
        bool negative_is_bad = false;
        bool is_weapon = false;
        bool skippable_in_display_defence = false;
        bool allowed_skip_in_repair = false;

        bool combat_view_mandatory = false;
        bool combat_view_if_damaged = true;
        bool combat_view_if_not_full = false;

        std::string display_name;

        //float base_cost = 1.f;

        research_info::types research_type = research_info::COUNT;

        std::map<resource::types, float> construction_cost_ratio;
    };

    extern std::vector<component_element_info> element_infos;

    static std::vector<types> weapons_map_def
    {
        RAILGUN,
        TORPEDO,
        PLASMAGUN,
        COILGUN,
    };

    static std::vector<types> skippable_in_display_def
    {
        //COLONISER,
        RAILGUN,
        TORPEDO,
        PLASMAGUN,
        COILGUN,
        RESOURCE_PRODUCTION,
        RESOURCE_STORAGE,
        SHIPYARD,
        MASS_ANCHOR,
    };

    static std::vector<types> repair_priorities_in_combat_def
    {
        OXYGEN,
        COMMAND,
        ENERGY,
        SHIELD_POWER,
        COOLING_POTENTIAL,
        COILGUN,
        TORPEDO,
        RAILGUN,
        PLASMAGUN,
        WARP_POWER,
        STEALTH,
    };

    static std::vector<types> repair_priorities_out_combat_def
    {
        OXYGEN,
        COMMAND,
        ENERGY,
        WARP_POWER,
        SHIELD_POWER,
        STEALTH,
    };

    extern std::vector<types> allowed_skip_repair_def;

    inline
    std::vector<int> generate_repair_priorities(const std::vector<types>& vec)
    {
        std::vector<int> ret;

        ///delibarately includes none
        for(int i=0; i<=(int)NONE; i++)
        {
            int found_val = -1;

            for(int kk=0; kk<vec.size(); kk++)
            {
                if(vec[kk] == i)
                {
                    found_val = kk;
                    break;
                }
            }

            ret.push_back(found_val);
        }

        return ret;
    }

    extern std::vector<int> repair_in_combat_map;
    extern std::vector<int> repair_out_combat_map;
    extern std::vector<int> allowed_skip_repair;
    extern std::vector<int> skippable_in_display;
    extern std::vector<int> weapons_map;

    extern std::vector<std::string> display_strings;
    //extern std::vector<float> base_cost_of_component_with_this_primary_attribute;

    static float construction_cost_mult = 0.1f;

    extern std::vector<research_info::types> component_element_to_research_type;

    std::map<resource::types, float> component_storage_to_resources(const types& type);

    ///not cost, but if we multiply by cost we get the end cost
    //std::map<resource::types, float> component_base_construction_ratio(const types& type, component& c);

    static float tech_upgrade_effectiveness = 1.2f;
    static float tech_cooldown_upgrade_effectiveness = 1.1f;

    void upgrade_component(component_attribute& in, int type, float old_tech, float new_tech);

    float upgrade_value(float value, float old_tech, float new_tech);

    static float max_components_total_size = 9.f;
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
        WARP_DISTANCE = 8,
        DAMAGED_WITHOUT_O2 = 16,
        OXYGEN_STARVATION = 32,
    };
}

using ship_component_element = ship_component_elements::types;

///tech level?
///use recharge time?
///need burst fire and group fire just because it looks sweet
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

    bool present = false;

    float add_amount(float amount);
    bool can_use();
    void use();
    float get_available_capacity() const;
    float get_total_capacity(float step_s); ///including drain
    float get_drain_capacity(float step_s); ///not including storage

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
    float get_efficiency(float step_s);

    ///perform actual upgrade
    void upgrade_tech_level(int type, float from, float to);

    void set_max_tech_level_from_empire_and_component_attribute(int type, empire* e);
    void set_tech_level_from_empire(int type, empire* e);
    void set_tech_level_from_research(int type, research& r);
    void set_tech_level(float level);
    float get_tech_level();

    void upgrade_size(float old_size, float new_size);

    /*std::map<resource::types, float> resources_ratio_produced;

    std::map<resource::types, float> resources_ratio_stored;
    std::map<resource::types, float> resources_cur_stored;*/

//private:
    float currently_drained = 0.f;
};

std::map<ship_component_element, float> merge_diffs(std::map<ship_component_element, float> one, const std::map<ship_component_element, float>& two);

struct component_tag_type
{
    component_tag::tag first = (component_tag::tag)0;
    float second = 0.f;
};

namespace component_category_info
{
    enum types
    {
        CONTROL,
        CORE,
        ENGINE,
        WEAPON,
        DEFENCE,
        SUPPORT,
        EMPIRE,
        NONE
    };

    static inline
    std::vector<std::string> names
    {
        "Control",
        "Core",
        "Engines",
        "Weapons",
        "Defence",
        "Support",
        "Empire",
        "Err",
    };

    ///Time to move into the modern era and use arrays!
    inline
    std::array<bool, types::NONE> is_active;
}

using component_category_t = component_category_info::types;

///ie what can things do
///this is a ship entity for the moment.. but could likely describe a character as well
///float get_current_functionality
///maybe component attributes should not have a tech level, but components overall
struct component : serialisable
{
    component_category_t ui_category = component_category_info::NONE;
    ship_component_elements::tech_type tech_type = ship_component_elements::NONE_TECH;
    float current_size = 1;
    float ship_size = 1;

    std::string name = "";

    std::vector<component_attribute> components;

    std::vector<component_tag_type> tag_list;

    ///for ui stuff. Its better to keep this internally in case we add new components
    bool clicked = false;

    float scanning_difficulty = randf_s(0.f, 1.f);

    bool skip_in_derelict_calculations = false;
    bool repair_this_when_recrewing = false; ///when you recrew a ship, repair this component
    float cost_mult = 1.f;

    ship_component_elements::types primary_attribute = ship_component_elements::NONE;

    void set_tech_type(int tt);

    float get_hp_frac();

    void upgrade_size(float old_size, float new_size);
    void set_size(float new_size);
    void set_ship_size(float ship_size);
    component with_size(float new_size);

    component();

    inline
    bool has_element(const ship_component_element& type)
    {
        return components[type].present;
    }

    component_attribute get_element(const ship_component_element& type);

    ///ie calculate all offsets
    std::map<ship_component_element, float> get_timestep_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_production_diff(float step_s);
    std::map<ship_component_element, float> get_timestep_consumption_diff(float step_s);
    std::map<ship_component_element, float> get_stored();
    std::map<ship_component_element, float> get_stored_max();
    std::map<ship_component_element, float> get_use_diff();

    float get_use_frac();

    ///returns a pair of new component, extra resources left over
    //std::pair<component, std::map<ship_component_element, float>> apply_diff(const std::map<ship_component_element, float>& diff);

    std::map<ship_component_element, float> apply_diff(const std::map<ship_component_element, float>& available);
    void apply_diff_single(const ship_component_element& elem, float amount);

    void update_time(float step_s);

    ///how much *more* we can take
    //std::map<ship_component_element, float> get_available_capacities();
    std::vector<std::pair<ship_component_element, float>> get_available_capacities_vec();
    std::vector<float> get_available_capacities_linear_vec();

    //std::map<ship_component_element, float> get_needed_resources(float time_s);
    std::map<ship_component_element, float> get_stored_and_produced_resources(float time_s);

    void add(ship_component_element element, const component_attribute& attr);

    float calculate_total_efficiency(float step_s);
    void propagate_total_efficiency(float step_s);

    ship_component_element get_weapon_type();

    int get_tag_offset(component_tag::tag tag);
    bool has_tag(component_tag::tag tag);

    float get_tag(component_tag::tag tag);

    void set_tag(component_tag::tag tag, float val);

    bool is_weapon();

    ///BLANKET SETS ALL SUB COMPONENTS TO TECH LEVEL
    void set_tech_level(float tech_level);
    void set_max_tech_level_from_empire_and_component(empire* e);
    void set_tech_level_from_empire(empire* e);
    void set_tech_level_from_research(research& r);
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
    std::map<resource::types, float> resources_cost();
    ///ie how much they're worth
    std::map<resource::types, float> resources_received_when_scrapped();
    std::map<resource::types, float> resources_needed_to_repair();

    std::map<resource::types, float> base_resources_cost;
    std::map<resource::types, float> extra_resources_ratio;

    ///how much research would empire emp get if they could
    research_category get_research_base_for_empire(empire* owner, empire* claiming_empire);
    research_category get_research_real_for_empire(empire* owner, empire* claiming_empire);

    ///does in * hp_cur / hp_max safely
    float safe_hp_frac_modify(float in);

    bool test_if_can_use_in_ship_customisation = false;

    void do_serialise(serialise& s, bool ser);

    virtual ~component(){}
};

struct projectile;
struct ship_manager;
struct empire;
struct orbital;

namespace ship_info
{
    constexpr float ship_obfuscation_level = 0.99f;
    constexpr float accessory_information_obfuscation_level = 0.f;
    constexpr float misc_resources_obfuscation_level = 0.5f;
};

/*struct attribute_info
{
    int component_offset = 0;
    int attribute_offset = 0;
};*/

namespace ship_type
{
    enum types
    {
        MINING = 0,
        COLONY,
        SCOUT,
        MILITARY,
        COUNT,
    };

    static std::map<types, std::string> type_to_png
    {
        {MINING, "pics/fleet_chevron_ore.png"},
        {COLONY, "pics/fleet_chevron_colony.png"},
        {SCOUT, "pics/fleet_chevron_scout.png"},
        {MILITARY, "pics/fleet_chevron_military.png"},
    };

    static std::map<types, std::string> type_to_name
    {
        {MINING, "Mining"},
        {COLONY, "Colony"},
        {SCOUT, "Scout"},
        {MILITARY, "Military"},
    };
}

struct popup_info;

struct ship_base_context_menu : has_context_menu
{
    bool context_are_you_sure_war = false;
    bool context_are_you_sure_scrap = false;
    bool context_renaming = false;

    virtual void context_handle_menu(orbital* associated, empire* player_empire, fleet_manager& fleet_manage, popup_info& popup) {};
};

struct ship;

std::string get_component_attribute_display_string(const component& c, int type);
std::string get_component_display_string(component& c);
//std::vector<std::string> get_components_display_string(ship& s);

struct ship : positional, serialisable, ship_base_context_menu
{
    ship_type::types ai_fleet_type = ship_type::COUNT;
    ///for ui purposes
    ship_type::types estimated_type = ship_type::MILITARY;

    std::string name;

    std::vector<component_attribute> cached_fully_merged;

    uint32_t id = gid++;
    static uint32_t gid;

    static uint32_t get_new_id()
    {
        return gid++;
    }

    ship duplicate() const;

    ship();

    void sort_components();

    ship_type::types estimate_ship_type();

    virtual void context_handle_menu(orbital* associated, empire* player_empire, fleet_manager& fleet_manage, popup_info& popup) override;

    int team = 0;

    std::vector<std::vector<int>> type_to_component_offsets;

    std::vector<component> entity_list;

    ///returns extra resources for us to handle
    void tick_all_components(float step_s);
    void tick_other_systems(float step_s);
    research tick_drain_research_from_crew(float step_s);

    void tick_combat(float step_s);
    void enter_combat();
    void leave_combat();
    bool in_combat();

    research get_crewing_research(float step_s);

    //std::map<ship_component_element, float> last_left_over;

    std::vector<float> get_produced(float step_s);
    std::vector<component_attribute> get_fully_merged(float step_s);
    component_attribute get_fully_merged_single(const ship_component_element& type, float step_s);
    std::vector<component_attribute> get_fully_merged_no_efficiency_with_hpfrac(float step_s);

    std::map<ship_component_element, float> get_available_capacities();
    std::vector<std::pair<ship_component_element, float>> get_available_capacities_vec();
    std::vector<float> get_available_capacities_linear_vec();
    /*std::map<ship_component_element, float> get_needed_resources();*/
    std::map<ship_component_element, float> get_produced_resources(float time_s);

    std::map<ship_component_element, float> get_stored_and_produced_resources(float time_s);
    std::map<ship_component_element, float> get_needed_resources(float time_s);
    std::map<ship_component_element, float> get_stored_resources();
    std::map<ship_component_element, float> get_max_resources();

    void distribute_resources(std::map<ship_component_element, float> res);
    void add_negative_resources(std::map<ship_component_element, float> res);

    ///for ship customisation
    void refill_all_components();

    std::map<ship_component_element, float> get_use_frac(component& c);
    float get_min_use_frac(component& c);
    bool can_use(component& c);
    void use(component& c);

    component* get_component_with_primary(ship_component_elements::types type);
    component* get_component_with(ship_component_elements::types type);

    std::vector<component> fire();
    bool can_use_warp_drives();
    void use_warp_drives();
    float get_warp_distance(); ///ignores practicalities, purely base distance

    void add(const component& c);

    std::vector<int> has_element;

    //std::map<empire*, float> damage_taken;

    void hit(projectile* p, orbital* associated);
    ///last 3 parameters are optional, allow for faction relation mechanics degradation
    void hit_raw_damage(float val, empire* hit_by, ship* s_hit_by, orbital* associated);

    void check_load(vec2i dim);
    void generate_image(vec2i dim);

    virtual ~ship();

    sf::Image img;
    sf::Texture tex;

    bool is_loaded = false;

    bool highlight = false;

    ship_manager* owned_by = nullptr;

    bool confirming_scrap = false;
    bool display_ui = false;
    bool display_popout = false;
    bool display_weapon = false;

    ///fallback on nullptr emp is to force resupply
    void resupply_elements(empire* emp, const std::vector<ship_component_element>& to_resupply, int num = 1);
    void refill_resources(empire* emp, int num = 1);
    void resupply(empire* emp, int num = 1);
    void repair(empire* emp, int num = 1);

    bool damaged();

    bool can_move_in_system();
    float get_move_system_speed();

    void apply_disengage_penalty();
    bool can_disengage();
    bool can_engage();
    bool should_be_removed_from_combat();

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

    bool can_be_upgraded();
    void set_tech_level_of_component(int component_offset, float tech_level);
    void set_max_tech_level_from_empire_and_ship(empire* e); ///takes max(ship, empire) tech
    void set_tech_level_from_empire(empire* e);
    void set_tech_level_from_research(research& r);

    ///damages the ship until derelict
    void randomise_make_derelict();
    void random_damage(float frac);

    float get_total_cost();
    float get_real_total_cost();
    float get_tech_adjusted_military_power();

    std::map<resource::types, float> resources_needed_to_recrew_total();
    std::map<resource::types, float> resources_needed_to_repair_total();
    ///calculate research separately as it needs empire, both sides
    std::map<resource::types, float> resources_received_when_scrapped();
    std::map<resource::types, float> resources_cost();

    int number_of_times_can_fully_dispense(const std::map<resource::types, float>& resources);
    bool can_fully_dispense(const std::map<resource::types, float>& resources);
    void fully_dispense(const std::map<resource::types, float>& resources);

    void empty_resources();

    ///requires raw research, not tech currency
    research get_research_base_for_empire(empire* owner, empire* claiming);
    research get_research_real_for_empire(empire* owner, empire* claiming);

    research get_recrew_potential_research(empire* claiming);

    std::string get_resource_str(const ship_component_element& type);

    float get_fuel_frac();
    float get_min_warp_use_frac();
    //std::map<ship_component_element, float> get_warp_use_frac();

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
    float get_scanning_power_on(orbital* o, int difficulty_modifier = 0);
    float get_scanning_power_on_ship(ship* s, int difficulty_modifier = 0);

    float get_drive_signature();
    float get_scanning_ps();

    bool can_colonise();
    bool is_military();

    float get_total_storage_of_components_with_this_primary(ship_component_element primary, ship_component_element resource_to_get);
    float get_max_storage_of_components_with_this_primary(ship_component_element primary, ship_component_element resource_to_get);

    float get_total_components_size();
    bool is_ship_design_valid();

    static sf::Texture* get_world_texture(ship_type::types type);
    sf::Texture* get_world_texture();

    static sf::RenderTexture* intermediate;

    //sf::RenderTexture* intermediate_texture = nullptr;

    ///starting to apply hacks to get around the lack of a command queue
    ///sign that we may need the command queue
    bool colonising = false;
    orbital* colonise_target = nullptr;

    bool shift_clicked = false;

    float current_size = 1.f;

    float get_current_size();
    float get_visual_scale();

    void set_size(float new_size);

    float editor_size_storage = current_size;

    void do_serialise(serialise& s, bool ser);

    ///?
    //void fire();
};

struct orbital_system;
struct orbital;

struct battle_manager;

///can be used as a fleet
struct ship_manager : serialisable, ship_base_context_menu
{
    //int32_t this_fight_id = -1;
    //int32_t last_fight_id = -1;

    //bool requesting_or_in_battle = false;
    bool cleanup = false;

    ai_fleet ai_controller;

    std::vector<ship*> ships;

    bool auto_resupply = true;
    bool auto_harvest_ore = true;
    bool auto_colonise = true;

    //ship* make_new(int team);
    //ship* make_new_from(int team, const ship& s);
    ship* make_new_from(empire* e, const ship& s);

    void destroy(ship* s);

    std::vector<std::string> get_info_strs();
    std::vector<std::string> get_info_strs_with_info_warfare(empire* viewing, orbital* my_orbital, bool full_detail);

    virtual void context_handle_menu(orbital* associated, empire* player_empire, fleet_manager& fleet_manage, popup_info& popup) override;

    void merge_into_me(ship_manager& other);

    ///from other ship manager
    void steal(ship* const s);

    empire* parent_empire = nullptr;

    void refill_resources(empire* from);
    void resupply(empire* from, bool can_resupply_derelicts = true);
    void resupply_from_nobody();
    void repair(empire* from);
    ///for the ai to use
    bool should_resupply_base(const std::vector<ship_component_element>& to_test);
    bool should_resupply();
    bool should_repair();

    ///Am i able to be repaired
    ///This is defined as any system which is allied, owned by me, or I have passage rights
    ///If there are orbitals hostile to me in my system, we cannot repair
    ///If any ship is derelict we cannot repair
    ///If in combat cannot repair
    bool has_access_to_repair(orbital_system* sys);

    void tick_all(float step_s);

    bool can_move_in_system();
    float get_move_system_speed();

    void draw_alerts(sf::RenderWindow& win, vec2f abs_pos);
    ///if we need just one texture for a whole fleet
    ship_type::types get_most_common_ship_type();
    sf::Texture* get_universe_texture();

    void force_warp(orbital_system* fin, orbital_system* cur, orbital* o);
    void try_warp(orbital_system* fin, orbital* o);
    bool can_warp(orbital_system* fin, orbital_system* cur, orbital* o);
    bool within_warp_distance(orbital_system* fin, orbital* o);
    float get_min_warp_distance(); ///ignores practicalities, purely base distance
    bool can_use_warp_drives();
    float get_overall_warp_drive_use_frac();

    bool any_with_element(ship_component_element elem);
    bool all_with_element(ship_component_element elem);

    void leave_combat();
    void enter_combat();
    bool any_in_combat();
    bool can_engage(); ///and be engaged
    bool should_be_removed_from_combat();

    void random_damage_ships(float frac);

    void apply_disengage_penalty();

    std::string get_engage_str();
    std::string get_fuel_message();

    float get_min_fuel_frac();

    bool any_colonising();

    bool any_derelict();
    bool all_derelict();

    bool any_damaged();

    int number_of_times_can_fully_dispense(const std::map<resource::types, float>& resources);
    bool can_fully_dispense(const std::map<resource::types, float>& resources);
    void fully_dispense(const std::map<resource::types, float>& resources);

    float get_tech_adjusted_military_power();
    bool is_military();
    bool majority_of_type(ship_type::types type);

    float accumulated_dt = 0;

    ///for stealth
    bool decolonising = false;
    bool toggle_fleet_ui = false;
    bool to_close_ui = false;

    bool can_merge = false;

    bool in_friendly_territory = false;


    static int gid;
    int my_id = gid++;

    ///need to differentiate disk and network serialisation
    void do_serialise(serialise& s, bool ser) override;

    virtual ~ship_manager(){}

    //fleet_manager* fleet_manage = nullptr;
};

struct empire_manager;

///manages fleets
struct fleet_manager : serialisable
{
    std::vector<ship_manager*> fleets;

    ship_manager* make_new();
    bool owns(ship_manager* sm);

    void destroy(ship_manager*);

    void cull_dead_deferred();
    void destroy_cleanup(empire_manager& empire_manage);

    void tick_all(float step_s);


    ship* nearest_free_colony_ship_of_empire(orbital* o, empire* e);

    //void tick_cleanup_colonising();

    ///if this becomes a perf issue, move into empire
    ///shouldn't though
    //void draw_ui(empire* viewing_empire, system_manager& system_manage);

    int internal_counter = 0;

    void do_serialise(serialise& s, bool ser) override;

    void erase_all();

    void shuffle_networked_ships();

    virtual ~fleet_manager(){}
};

#endif // SHIP_HPP_INCLUDED
