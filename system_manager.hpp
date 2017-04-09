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
        MOON,
        ASTEROID,
        FLEET,
        MISC,
        NONE,
    };

    static std::vector<std::string> names =
    {
        "STAR",
        "PLANET",
        "MOON",
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
struct system_manager;
struct orbital_system;

struct orbital
{
    std::string name;
    std::string description;

    int resource_type_for_flavour_text = 0;

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

    void draw(sf::RenderWindow& win, empire* viewer_empire);

    void center_camera(system_manager& system_manage);

    bool point_within(vec2f pos);

    std::vector<std::string> get_info_str();
    std::string get_empire_str(bool newline = true);

    void transfer(float new_rad, float new_angle);
    void transfer(vec2f pos);

    ///only transfers if some underlying thing says yes
    ///eg for fleets, only if they have fuel
    void request_transfer(vec2f pos);

    vec3f col = {1,1,1};

    ///change to produces resources so we can apply to planets
    bool is_resource_object = false;
    resource_manager produced_resources_ps;

    void make_random_resource_asteroid(float max_ps);
    void make_random_resource_planet(float max_ps);

    bool can_dispense_resources();

    empire* parent_empire = nullptr;

    void draw_alerts(sf::RenderWindow& win, empire* viewer_empire, system_manager& system_manage);

    orbital_system* parent_system = nullptr;

    bool has_quest_alert = false;
    bool clicked = false; ///for quest alert system
    bool dialogue_open = false;

    bool can_construct_ships = true;
    bool construction_ui_open = false;
};

struct orbital_system
{
    vec2f universe_pos = {0,0};

    orbital* get_base();

    std::vector<orbital*> orbitals;
    std::vector<orbital*> total_orbitals; ///including useless ones

    orbital* make_new(orbital_info::type type, float rad, int num_verts = 5);

    void tick(float step_s);

    void destroy(orbital*);
    ///non destructively reparent
    void steal(orbital*, orbital_system* s);

    ///currently viewed empire is drawn differently, see
    void draw(sf::RenderWindow& win, empire* viewer_empire);

    void cull_empty_orbital_fleets(empire_manager& empire_manage);

    orbital* get_by_element(void* element);

    void generate_asteroids_new(int n, int num_belts, int num_resource_asteroids);
    void generate_asteroids_old(int n, int num_belts, int num_resource_asteroids);
    void generate_planet_resources(float max_ps);

    void draw_alerts(sf::RenderWindow& win, empire* viewing_empire, system_manager& system_manage);

    void generate_random_system(int planets, int num_asteroids, int num_belts, int num_resource_asteroids);
    void generate_full_random_system();

    bool highlight = false;

    ///hostile fleets
    std::vector<orbital*> get_fleets_within_engagement_range(orbital* me);

    bool can_engage(orbital* me, orbital* them);

    void make_asteroid_orbital(orbital* o);
};

struct system_manager
{
    std::vector<orbital_system*> systems;
    orbital_system* currently_viewed = nullptr;
    orbital_system* hovered_system = nullptr;

    orbital_system* make_new();

    ///perf issue?
    orbital_system* get_parent(orbital* o);
    orbital_system* get_by_element(void* ptr);

    void tick(float step_s);

    void destroy(orbital_system* s);

    void repulse_fleets();

    void cull_empty_orbital_fleets(empire_manager& empire_manage);


    void draw_alerts(sf::RenderWindow& win, empire* viewing_empire);

    void draw_viewed_system(sf::RenderWindow& win, empire* viewer_empire);

    void set_viewed_system(orbital_system* s, bool reset_zoom = true);

    void draw_universe_map(sf::RenderWindow& win, empire* viewer_empire);
    void process_universe_map(sf::RenderWindow& win, bool lclick);

    ///camera. Set here because zoom will be useful
    ///Camera panning should also probably go here
    void change_zoom(float zoom);
    void set_zoom(float zoom, bool auto_enter_system = false);
    void pan_camera(vec2f dir);

    bool in_system_view();
    void enter_universe_view();
    orbital_system* get_nearest_to_camera();

    float zoom_level = 1.f;
    vec2f camera;
    float universe_scale = 100.f;
    float sun_universe_rad = 200;

    ///also generate empires in universe, in a separate function
    void generate_universe(int num);
};

#endif // SYSTEM_MANAGER_HPP_INCLUDED
