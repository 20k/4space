#ifndef EMPIRE_HPP_INCLUDED
#define EMPIRE_HPP_INCLUDED

#include "resource_manager.hpp"
#include <map>
#include "research.hpp"
#include <vec/vec.hpp>

struct orbital;
struct orbital_system;
struct ship_manager;

struct empire
{
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

    void add_resource(resource::types type, float amount);

    void add_research(const research& r);

    ///returns real amount
    float dispense_resource(resource::types type, float requested);
    ///take fraction is fraction to actually take, frac_out is the proportion of initial resources we were able to take
    ///type is the amount of resources we are requesting
    std::map<resource::types, float> dispense_resources_proportionally(const std::map<resource::types, float>& type, float take_fraction, float& frac_out);

    void tick(float step_s);
    void draw_ui();

    float empire_culture_distance(empire* e);

    float get_research_level(research_info::types type);
};

struct empire_manager
{
    std::vector<empire*> empires;

    empire* make_new();

    void notify_removal(orbital* o);
    void notify_removal(ship_manager* s);

    void tick_all(float step_s);
};

#endif // EMPIRE_HPP_INCLUDED
