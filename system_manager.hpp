#ifndef SYSTEM_MANAGER_HPP_INCLUDED
#define SYSTEM_MANAGER_HPP_INCLUDED

#include <vector>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>

namespace orbital_info
{
    enum type
    {
        STAR,
        PLANET,
        ASTEROID,
        FLEET,
        MISC,
        NONE,
    };
}

namespace sf
{
    struct RenderWindow;
}

struct orbital_simple_renderable
{
    std::vector<float> vert_dist;

    void init(int n, float min_rad, float max_rad);

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos);
};

struct sprite_renderable
{
    sf::Image img;
    sf::Texture tex;

    void load(const std::string& str);

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos);
};

struct orbital
{
    orbital_simple_renderable simple_renderable;
    sprite_renderable sprite;

    int render_type = 0;

    void* data = nullptr;

    vec2f absolute_pos;

    float angular_velocity_ps = 0.f;
    float rotation_velocity_ps = 0;

    float rotation = 0;
    float orbital_angle = 0.f;
    float orbital_length = 0;
    float rad = 0.f;

    orbital* parent = nullptr;

    orbital_info::type type = orbital_info::NONE;

    void set_orbit(float ang, float len, float ang_vel_s);

    void tick(float step_s);

    void draw(sf::RenderWindow& win);

    void center_camera(sf::RenderWindow& win);
};

struct orbital_system
{
    std::vector<orbital*> orbitals;

    orbital* make_new(orbital_info::type type, float rad);

    void tick(float step_s);

    void destroy(orbital*);

    void draw(sf::RenderWindow& win);
};

struct system_manager
{
    std::vector<orbital_system*> systems;

    orbital_system* make_new();

    void tick(float step_s);

    void destroy(orbital_system* s);
};

#endif // SYSTEM_MANAGER_HPP_INCLUDED
