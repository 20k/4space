#ifndef EMPIRE_HPP_INCLUDED
#define EMPIRE_HPP_INCLUDED

#include "resource_manager.hpp"
#include <map>
#include "research.hpp"
#include <vec/vec.hpp>

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
    float hostility = 0; ///For precursors, but also a generally 'pissed off' meter
    float positivity = 0; ///For precursors, but also a generally short term happiness meter
};

struct empire
{
    bool has_ai = false;
    vec3f colour = {1,1,1};

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

    std::vector<orbital*> owned;
    std::vector<ship_manager*> owned_fleets;

    std::map<empire*, faction_relations> relations_map;

    empire();

    void take_ownership(orbital* o);
    void release_ownership(orbital* o);

    void take_ownership(ship_manager* s);
    void release_ownership(ship_manager* s);

    ///for debugging
    void take_ownership_of_all(orbital_system* o);

    bool owns(orbital* o);

    void generate_resource_from_owned(float step_s);

    bool can_fully_dispense(const std::map<resource::types, float>& res);
    void dispense_resources(const std::map<resource::types, float>& res);

    void add_resource(resource::types type, float amount);

    void add_research(const research& r);

    void culture_shift(float culture_units, empire* destination);

    ///returns real amount
    float dispense_resource(resource::types type, float requested);
    ///take fraction is fraction to actually take, frac_out is the proportion of initial resources we were able to take
    ///type is the amount of resources we are requesting
    std::map<resource::types, float> dispense_resources_proportionally(const std::map<resource::types, float>& type, float take_fraction, float& frac_out);

    void tick_system_claim();
    void tick(float step_s);
    void tick_ai(all_battles_manager& all_battles);
    void draw_ui();

    float empire_culture_distance(empire* e);

    float get_research_level(research_info::types type);

    float available_scanning_power_on(orbital* o);
    float available_scanning_power_on(ship* s, system_manager& system_manage);

    ///does allying, not 'try' ally etc
    ///two way street
    ///when we implement proposing alliances, culture shift will be a thing
    ///if we're allied with another faction, culture shift
    void become_hostile(empire* e);
    void become_unhostile(empire* e);
    void ally(empire* e);
    void unally(empire* e);
    bool is_allied(empire* e);
    bool is_hostile(empire* e);

    ///make sure all relations have valid values
    void clamp_relations();

    void negative_interaction(empire* e);
    void positive_interaction(empire* e);

    ///theoretically, not practically
    bool can_colonise(orbital* o);
    void tick_cleanup_colonising(); ///and claim
    void tick_decolonisation();
};

struct fleet_manager;

struct empire_manager
{
    std::vector<empire*> empires;

    empire* make_new();

    void generate_resources_from_owned(float diff_s);

    void notify_removal(orbital* o);
    void notify_removal(ship_manager* s);

    void tick_all(float step_s, all_battles_manager& all_battles);

    void tick_cleanup_colonising();
    void tick_decolonisation();

    ///if system_size > 1, explores nearby uncolonised systems
    empire* birth_empire(fleet_manager& fleet_manage, orbital_system* os, int system_size = 1);
    empire* birth_empire_without_system_ownership(fleet_manager& fleet_manage, orbital_system* os, int fleets = 2, int ships_per_fleet = 2);

};

#endif // EMPIRE_HPP_INCLUDED
