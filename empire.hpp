#ifndef EMPIRE_HPP_INCLUDED
#define EMPIRE_HPP_INCLUDED

#include "resource_manager.hpp"
#include <map>
#include "research.hpp"
#include <vec/vec.hpp>
#include "ai_empire.hpp"
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include "serialise.hpp"

struct orbital;
struct orbital_system;
struct ship_manager;
struct ship;
struct system_manager;
struct all_battles_manager;


struct faction_relations
{
    bool hostile = false;
    bool allied = false;
    float friendliness = 0.5f; ///0 = v bad, 1 = best
    bool have_passage_rights = false;

    float hostility = 0; ///For precursors, but also a generally 'pissed off' meter
    float positivity = 0; ///For precursors, but also a generally short term happiness meter
};

struct empire_manager;
struct fleet_manager;

namespace relations_info
{
    ///below means we unally
    constexpr float unally_threshold = 0.7f;
    ///below means hostile
    constexpr float hostility_threshold = 0.f;
    ///above means ally
    constexpr float ally_threshold = 1.f;
    ///above means make peace
    constexpr float unhostility_threshold = 0.1f;

    constexpr float fudge = 0.01f;

    constexpr vec3f base_col = {1, 1, 1}; ///used for owned objects
    constexpr vec3f hidden_col = {0.6, 0.6, 0.6};
    constexpr vec3f hostile_col = {1, 0.15f, 0.15f};
    constexpr vec3f friendly_col = {0.5, 1, 0.5};
    constexpr vec3f neutral_col = {1, 0.5, 0};
    constexpr vec3f alt_owned_col = {0.4f, 0.4f, 1.f};
}

struct empire : serialisable
{
    bool is_player = false;

    ai_empire ai_empire_controller;

    bool has_ai = false;
    vec3f colour = {1,1,1};
    bool given_colour = false;
    vec3f temporary_hsv;

    ///a two dimensional vector that we can use to determine our similarity to other empires
    ///used for determining tech and ship operating capability
    ///we need some way for getting less tech once we learn a lot from the culture
    ///maybe just adjust our culture similarity closer to theirs?
    ///that way we'd only get the 1 dimensional upgrade from the difference between our tech and theirs
    vec2f culture_similarity;

    research research_tech_level;
    resource_manager resources;

    int team_id = team_gid++;
    static int team_gid;

    std::string name;
    std::string ship_prefix;

    std::vector<orbital*> owned;
    std::vector<ship_manager*> owned_fleets;

    std::unordered_set<orbital_system*> calculated_owned_systems;

    std::unordered_map<empire*, faction_relations> relations_map;

    empire();

    void take_ownership(orbital* o);
    void release_ownership(orbital* o);

    void take_ownership(ship_manager* s);
    void release_ownership(ship_manager* s);

    ///for debugging
    void take_ownership_of_all(orbital_system* o);

    bool owns(orbital* o);

    void generate_resource_from_owned(float step_s);

    resource_manager last_income;
    resource_manager backup_income;
    std::deque<resource_manager> backup_income_list;
    std::deque<float> backup_dt_s;

    bool can_fully_dispense(resource::types type, float amount);
    bool can_fully_dispense(const resource_manager& res);
    bool can_fully_dispense(const std::map<resource::types, float>& res);
    void dispense_resources(resource::types type, float amount);
    void dispense_resources(const std::map<resource::types, float>& res);

    void add_resource(const resource_manager& res);
    void add_resource(resource::types type, float amount);

    float get_relations_shift_of_adding_resources(const resource_manager& res);

    void add_research(const research& r);

    void culture_shift(float culture_units, empire* destination);
    void positive_relations(empire* e, float amount);
    void negative_relations(empire* e, float amount);
    void offset_relations(empire* e, float amount);

    ///returns real amount
    void dispense_resource(const resource_manager& res);
    float dispense_resource(resource::types type, float requested);
    ///take fraction is fraction to actually take, frac_out is the proportion of initial resources we were able to take
    ///type is the amount of resources we are requesting
    std::map<resource::types, float> dispense_resources_proportionally(const std::map<resource::types, float>& type, float take_fraction, float& frac_out);

    void tick_system_claim();
    void tick(float step_s);
    void tick_ai(all_battles_manager& all_battles, system_manager& system_manage);
    void tick_high_level_ai(float dt_s, fleet_manager& fm, system_manager& sm);
    void draw_ui();

    float empire_culture_distance(empire* e);

    float get_research_level(research_info::types type);
    float get_military_strength();

    float available_scanning_power_on(orbital* o);
    float available_scanning_power_on(ship* s, system_manager& system_manage);
    float available_scanning_power_on(ship_manager* sm, system_manager& system_manage);

    ///does allying, not 'try' ally etc
    ///two way street
    ///when we implement proposing alliances, culture shift will be a thing
    ///if we're allied with another faction, culture shift
    ///maybe become_hostile and become_unhostile should be recursive?
    ///Or just make it a ticking relations drain (easier to implement)
    void become_hostile(empire* e);
    void become_unhostile(empire* e);
    void trade_space_access(empire* e, bool status);
    void trade_vision(empire* e);
    void ally(empire* e);
    void unally(empire* e);
    bool is_allied(empire* e);
    bool is_hostile(empire* e);
    bool can_make_peace(empire* e);
    bool can_traverse_space(empire* e); ///without diplomatic penalty

    float get_culture_modified_friendliness(empire* e);
    bool could_invade(empire* e);

    void update_vision_of_allies(orbital* o);

    //float constrain_relations(empire* e, float val, float lower, float upper) const;
    float get_relation_constraint_offset_upper(empire* e, float upper);
    float get_relation_constraint_offset_lower(empire* e, float lower);

    std::string get_relations_string(empire* e);
    ///alt owned colour is when we specifically don't want white as the owned colour
    vec3f get_relations_colour(empire* e, bool use_alt_owned_colour = false);
    std::string get_short_relations_str(empire* e);
    std::string get_single_digit_relations_str(empire* e);

    ///invasion mechanics conceptually
    ///find all spare ships leftover for invasions, group together and launch invasion
    ///if battles go poorly, pull invasion fleet out and cooldown/build more ships
    ///some ships will want to be tagged as defence ships
    ///lets create an empire_ai class so we can tag stuff and manage better
    ///reserve % of forces for defences, % of forces for general
    ///if we're invaded, repurpose general for defence
    ///if we're invading, repurpose general for attacking

    ///will also need colony ship mechanics

    ///make sure all relations have valid values
    void clamp_relations();

    void negative_interaction(empire* e);
    void positive_interaction(empire* e);

    ///we damage enemy which decreases relationship, enemies of enemy gets updated with +friendliness
    void propagage_relationship_modification_from_damaging_ship(empire* damaged);

    ///theoretically, not practically
    bool can_colonise(orbital* o);
    void tick_cleanup_colonising(); ///and claim
    void tick_decolonisation();
    void tick_relation_ship_occupancy_loss(float diff_s, system_manager& system_manage);
    void tick_relation_alliance_changes(empire* player_empire);
    void tick_relation_border_friction(float dt_s, system_manager& system_manage);

    void tick_calculate_owned_systems();

    bool has_vision(orbital_system* os);
    bool has_direct_vision(orbital_system* os); ///no allies

    empire_manager* parent = nullptr;

    std::vector<orbital_system*> get_unowned_system_with_my_fleets_in();
    float pirate_invasion_timer_s = 0.f; ///if this empire is pirates, time since we invaded
    float max_pirate_invasion_elapsed_time_s = 5 * 60.f;

    void tick_invasion_timer(float step_s, system_manager& system_manage, fleet_manager& fleet_manage);

    bool is_pirate = false;
    bool toggle_ui = false;
    bool is_derelict = false;

    bool toggle_systems_ui = true;

    uint16_t fleet_name_counter = 0;

    int desired_empire_size = 1;

    float accumulated_dt_s = 0.f;

    int frame_counter = 0;

    void do_serialise(serialise& s, bool ser) override;
};

struct fleet_manager;

struct empire_manager : serialisable
{
    std::vector<empire*> empires;
    std::vector<empire*> pirate_empires;

    ///NEVER USE FOR ANYTHING OTHER THAN COLOUR PICKING INSTANTLY BAD
    std::map<empire*, orbital_system*> initial_spawn_reference;
    ///ok so. if a pirate empire has a fleet alive in not an owned system, they can keep launching attacks

    empire* unknown_empire = nullptr;

    empire_manager()
    {
        unknown_empire = new empire;
        unknown_empire->name = "Unknown";
        unknown_empire->parent = this;
    }

    empire* make_new();

    void generate_resources_from_owned(float diff_s);

    void notify_removal(orbital* o);
    void notify_removal(ship_manager* s);

    void tick_all(float step_s, all_battles_manager& all_battles, system_manager& system_manage, fleet_manager& fleet_manage, empire* player_empire);

    void tick_cleanup_colonising();
    void tick_decolonisation();
    void tick_name_fleets();

    ///if system_size > 1, explores nearby uncolonised systems
    empire* birth_empire(system_manager& system_manage,fleet_manager& fleet_manage, orbital_system* os, int system_size = 1);
    empire* birth_empire_without_system_ownership(fleet_manager& fleet_manage, orbital_system* os, int fleets = 2, int ships_per_fleet = 2);

    void setup_relations();


    void birth_empires_random(fleet_manager& fleet_manage, system_manager& system_manage, float sys_frac);
    void birth_empires_without_ownership(fleet_manager& fleet_manage, system_manager& system_manage);

    void draw_diplomacy_ui(empire* viewer_empire, system_manager& system_manage);
    void draw_resource_donation_ui(empire* viewer_empire);

    void assign_colours_non_randomly();

    float get_spawn_empire_distance(empire* e1, empire* e2);

    bool confirm_break_alliance = false;
    bool confirm_declare_war = false;

    empire* offering_resources = nullptr;
    bool offer_resources_ui = false;
    bool giving_resources_ui_clicked = false;
    resource_manager offering;
    bool giving_are_you_sure = false;

    int frame_counter = 0;

    void do_serialise(serialise& s, bool ser) override;
};

#endif // EMPIRE_HPP_INCLUDED
