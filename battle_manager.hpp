#ifndef BATTLE_MANAGER_HPP_INCLUDED
#define BATTLE_MANAGER_HPP_INCLUDED

#include "ship.hpp"
#include <SFML/Graphics.hpp>

#include "serialise.hpp"

struct empire;
struct ship;

struct tonemap_options
{
    vec3f power_weights = {1, 1, 1};
};

struct projectile_options
{
    vec2f scale = {1,1};
    float overall_scale = 1.f;
    bool blur = false;
};

void tonemap(sf::Image& image, tonemap_options options = {{4, 4, 0.5}});
void premultiply(sf::Image& image);

///need to ensure loaded after serialisation
struct projectile : positional, serialisable
{
    projectile_options options;

    int type = 0;

    vec2f velocity;

    int pteam = 0;

    component base;

    sf::Image img;
    sf::Texture tex;

    void load(int type);

    empire* fired_by = nullptr;
    ship* ship_fired_by = nullptr;

    void do_serialise(serialise& s, bool ser) override;
};

struct battle_manager;
struct system_manager;

namespace sf
{
    struct RenderWindow;
}

struct projectile_manager : serialisable
{
    std::vector<projectile*> projectiles;

    projectile* make_new();

    void tick(battle_manager& manager, float step_s, system_manager& system_manage);

    void destroy(projectile* proj);
    void destroy_all();

    void draw(sf::RenderWindow& win);

    void do_serialise(serialise& s, bool ser) override;
};

struct all_battles_manager;
struct orbital_system;
struct system_manage;

struct battle_manager : serialisable
{
    projectile_manager projectile_manage;

    void keep_fleets_together(system_manager& system_manage);

    ///team -> ship
    //std::map<int, std::vector<ship*>> ships;

    std::vector<orbital*> ship_map;

    //std::map<empire*, std::vector<orbital*>> ship_map;
    //std::vector<std::pair<empire*, int>> slots_filled;
    //int num_slots = 0;


    void tick(float step_s, system_manager& system_manage);

    void draw(sf::RenderWindow& win);

    vec2f get_avg_centre_global();
    //void add_ship(ship* s);
    void add_fleet(orbital* o);

    ship* get_nearest_hostile(ship* s);

    ///in world coordinates... which atm is screenspace I WILL FORGET THIS
    ship* get_ship_under(vec2f pos);

    void set_view(system_manager& system_manage);

    bool can_disengage(empire* disengaging_empire);
    void do_disengage(empire* disengaging_empire); ///apply it to ships
    std::string get_disengage_str(empire* disengaging_empire);

    bool can_end_battle_peacefully(empire* leaving_empire);
    void end_battle_peacefully(); ///don't call on its own


    bool any_in_fleet_involved(ship_manager* sm);
    bool any_in_empire_involved(empire* e);

    void destructive_merge_into_me(battle_manager* bm, all_battles_manager& all_battles);

    orbital_system* get_system_in(system_manager& system_manage);

    ///to show the player why they can't disengage

    static uint32_t gid;
    uint32_t unique_battle_id = gid++;
    uint32_t frame_counter = 0;

    bool is_ui_opened = false;

    void do_serialise(serialise& s, bool ser) override;
};

struct orbital;

struct all_battles_manager : serialisable
{
    std::vector<battle_manager*> battles;
    battle_manager* currently_viewing = nullptr;

    battle_manager* make_new();

    void destroy(battle_manager* bm);

    void tick(float step_s, system_manager& system_manage);

    void draw_viewing(sf::RenderWindow& win);
    void set_viewing(battle_manager* bm, system_manager& system_manage, bool jump = false);

    battle_manager* make_new_battle(std::vector<orbital*> t1);

    ///disengaging empire can be nullptr
    void disengage(battle_manager* bm, empire* disengaging_empire);
    void end_battle_peacefully(battle_manager* bm);

    battle_manager* get_battle_involving(ship_manager* ship_manage);

    int request_stay_id = -1;
    bool request_stay_in_battle_system = false;
    bool request_enter_battle_view = false;
    bool request_leave_battle_view = false;

    void do_serialise(serialise& s, bool ser) override;

    void erase_all();
};

#endif // BATTLE_MANAGER_HPP_INCLUDED
