#ifndef BATTLE_MANAGER_HPP_INCLUDED
#define BATTLE_MANAGER_HPP_INCLUDED

#include "ship.hpp"

struct projectile : positional
{
    int type = 0;

    vec2f velocity;
};

struct projectile_manager
{
    std::vector<projectile*> projectiles;

    projectile* make_new();

    void tick();

    void destroy(projectile* proj);
};

struct battle_manager
{
    ///team -> ship
    std::map<int, ship*> ships;

    void tick(float time_s);

    void draw(sf::RenderWindow& win);
};

#endif // BATTLE_MANAGER_HPP_INCLUDED
