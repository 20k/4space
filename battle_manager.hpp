#ifndef BATTLE_MANAGER_HPP_INCLUDED
#define BATTLE_MANAGER_HPP_INCLUDED

#include "ship.hpp"
#include <SFML/Graphics.hpp>

struct projectile : positional
{
    int type = 0;

    vec2f velocity;

    int pteam = 0;

    float damage = 0;

    sf::Image img;
    sf::Texture tex;

    void load(int type);
};

struct battle_manager;

namespace sf
{
    struct RenderWindow;
}

struct projectile_manager
{
    std::vector<projectile*> projectiles;

    projectile* make_new();

    void tick(battle_manager& manager, float step_s);

    void destroy(projectile* proj);

    void draw(sf::RenderWindow& win);
};

struct battle_manager
{
    projectile_manager projectile_manage;

    ///team -> ship
    std::map<int, ship*> ships;

    void tick(float step_s);

    void draw(sf::RenderWindow& win);

    void add_ship(ship* s);

    ship* get_nearest_hostile(ship* s);
};

#endif // BATTLE_MANAGER_HPP_INCLUDED
