#ifndef SYSTEM_MANAGER_HPP_INCLUDED
#define SYSTEM_MANAGER_HPP_INCLUDED

#include <vector>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>
#include "resource_manager.hpp"

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

    static std::vector<std::string> names =
    {
        "STAR",
        "PLANET",
        "ASTEROID",
        "FLEET",
        "FLEET",
        "NONE",
    };

    static std::vector<int> render_type =
    {
        0,
        0,
        0,
        1,
        1,
        0,
    };

    static std::vector<std::string> load_strs =
    {
        "",
        "",
        "",
        "pics/fleet.png",
        "",
        ""
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

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, vec3f col = {1,1,1});
};

struct sprite_renderable
{
    sf::Image img;
    sf::Texture tex;

    void load(const std::string& str);

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, vec3f col = {1,1,1}, bool highlight = false);
};

struct empire;
struct empire_manager;

struct orbital
{
    float internal_time_s = 0.f;

    bool highlight = false;

    orbital_simple_renderable simple_renderable;
    sprite_renderable sprite;

    int render_type = 0;

    void* data = nullptr;

    vec2f absolute_pos;

    //float angular_velocity_ps = 0.f;
    float rotation_velocity_ps = 0;

    float rotation = 0;
    float orbital_angle = 0.f;
    float orbital_length = 0;
    float rad = 0.f;

    float old_rad = 0;
    float old_angle = 0;
    float new_rad = 0;
    float new_angle = 0;
    bool transferring = false;
    float start_time_s = 0;

    orbital* parent = nullptr;

    orbital_info::type type = orbital_info::NONE;

    void set_orbit(float ang, float len);
    void set_orbit(vec2f pos);

    void tick(float step_s);

    void draw(sf::RenderWindow& win);

    void center_camera(sf::RenderWindow& win);

    bool point_within(vec2f pos);

    std::vector<std::string> get_info_str();

    void transfer(float new_rad, float new_angle);
    void transfer(vec2f pos);

    vec3f col = {1,1,1};

    ///change to produces resources so we can apply to planets
    bool is_resource_object = false;
    resource_manager produced_resources_ps;

    void make_random_resource_asteroid(float max_ps);
    void make_random_resource_planet(float max_ps);

    bool can_dispense_resources();

    empire* parent_empire = nullptr;
};

struct orbital_system
{
    orbital* get_base();

    std::vector<orbital*> orbitals;

    orbital* make_new(orbital_info::type type, float rad, int num_verts = 5);

    void tick(float step_s);

    void destroy(orbital*);

    void draw(sf::RenderWindow& win);

    void cull_empty_orbital_fleets(empire_manager& empire_manage);

    orbital* get_by_element(void* element);

    void generate_asteroids(int n, int num_belts, int num_resource_asteroids);
    void generate_planet_resources(float max_ps);
};

struct system_manager
{
    std::vector<orbital_system*> systems;

    orbital_system* make_new();

    void tick(float step_s);

    void destroy(orbital_system* s);

    void repulse_fleets();

    void cull_empty_orbital_fleets(empire_manager& empire_manage);
};

#endif // SYSTEM_MANAGER_HPP_INCLUDED
