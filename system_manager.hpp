#ifndef SYSTEM_MANAGER_HPP_INCLUDED
#define SYSTEM_MANAGER_HPP_INCLUDED

#include <vector>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>
#include "resource_manager.hpp"
#include "object_command_queue.hpp"
#include <set>
#include <unordered_map>
#include "context_menu.hpp"
#include "serialise.hpp"

namespace orbital_info
{
    ///determines order in popup
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

    static float decolonise_time_s = 60.f;
    static float engagement_radius = 40.f;
}

struct empire;

struct empire_popup
{
    empire* e = nullptr;
    orbital_info::type type = orbital_info::NONE;
    int id = 0;
    bool hidden = false;
    bool is_player = false;
    bool is_allied;
};

bool operator<(const empire_popup& e1, const empire_popup& e2);

namespace sf
{
    struct RenderWindow;
}

struct orbital_simple_renderable : serialisable
{
    std::vector<float> vert_dist;

    void init(int n, float min_rad, float max_rad);

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, bool force_high_quality, bool draw_outline, const std::string& tag, vec3f col = {1,1,1}, bool show_detail = false, orbital* o = nullptr);

    void main_rendering(sf::RenderWindow& win, float rotation, vec2f absolute_pos, float scale, vec3f col);

    void do_serialise(serialise& s, bool ser) override;
};

struct sprite_renderable
{
    bool loaded = false;
    sf::Image img;
    sf::Texture tex;

    void load(const std::string& str);

    void draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, vec3f col = {1,1,1}, bool highlight = false);
};

struct empire;
struct empire_manager;
struct system_manager;
struct orbital_system;

struct position_history_element
{
    static double max_history_s;

    vec2f pos;
    double time_s = 0.f;
};


struct orbital : serialisable
{
    std::set<orbital*> in_combat_with;
    //std::set<orbital*> last_in_combat_with;

    bool cleanup = false;

    std::deque<position_history_element> multiplayer_position_history;

    object_command_queue command_queue;

    static int gid;
    int unique_id = gid++;

    int num_verts = 3;

    std::string name;
    std::string description;
    float star_temperature_fraction = 0.f;

    int resource_type_for_flavour_text = 0;

    double internal_time_s = 0.f;

    bool highlight = false;
    bool was_highlight = false;
    bool was_hovered = false;

    orbital_simple_renderable simple_renderable;
    sprite_renderable sprite;

    int render_type = 0;

    ship_manager* data = nullptr;

    int num_moons = 0;

    vec2f absolute_pos;
    vec2f universe_view_pos;

    //float angular_velocity_ps = 0.f;
    float rotation_velocity_ps = 0;

    float rotation = 0;
    float orbital_angle = 0.f;
    float orbital_length = 0;
    float rad = 0.f;

    /*float old_rad = 0;
    float old_angle = 0;
    float new_rad = 0;
    float new_angle = 0;
    //bool transferring = false;
    float start_time_s = 0;*/

    orbital* parent = nullptr;

    orbital_info::type type = orbital_info::NONE;

    void set_orbit(float ang, float len);
    void set_orbit(vec2f pos);

    ///on the most technical level
    void leave_battle();

    int vision_test_counter = 0;
    void do_vision_test();
    void tick(float step_s);

    void draw(sf::RenderWindow& win, empire* viewer_empire);

    bool rendered_asteroid_window = false;
    void begin_render_asteroid_window();
    void end_render_asteroid_window();

    float get_pixel_radius(sf::RenderWindow& win);

    void center_camera(system_manager& system_manage);

    bool point_within(vec2f pos);

    std::vector<std::string> get_info_str(empire* viewer_empire, bool use_info_warfare, bool full_detail);
    std::string get_empire_str(bool newline = true);

    std::string get_name_with_info_warfare(empire* viewing_empire);

    void transfer(float new_rad, float new_angle, orbital_system* in_system);
    void transfer(vec2f pos, orbital_system* in_system, bool at_back = true, bool combat_move = false);

    float get_transfer_speed();

    ///only transfers if some underlying thing says yes
    ///eg for fleets, only if they have fuel
    void request_transfer(vec2f pos, orbital_system* in_system);

    bool transferring();

    vec3f col = {1,1,1};

    ///change to produces resources so we can apply to planets
    bool is_resource_object = false;
    resource_manager produced_resources_ps;

    void make_random_resource_asteroid(float max_ps);
    void make_random_resource_planet(float max_ps);

    bool can_dispense_resources();

    empire* parent_empire = nullptr;

    void release_ownership();

    void draw_alerts(sf::RenderWindow& win, empire* viewer_empire, system_manager& system_manage);

    orbital_system* parent_system = nullptr;

    bool has_quest_alert = false;
    bool clicked = false; ///for quest alert system
    bool dialogue_open = false;

    bool can_construct_ships = true;
    bool construction_ui_open = false;

    bool is_colonised();
    bool can_colonise();

    bool is_mining = false;
    orbital* mining_target = nullptr;

    ///busy as in in combat or otherwise indisposed
    bool in_friendly_territory_and_not_busy();
    bool in_friendly_territory();

    //virtual void process_context_ui() override;

    ///if > than amount, remove parent empire
    float decolonise_timer_s = 0.f;
    bool being_decolonised = false;

    ///managed by command queue
    bool in_anchor_ui_state = false;
    vec2f last_screen_pos = {0,0};

    vec2f last_viewed_position;
    //bool ever_viewed = false;
    std::unordered_map<empire*, bool> viewed_by;
    std::unordered_map<empire*, bool> currently_viewed_by;

    static float calculate_orbital_drift_angle(float orbital_length, float step_s);
    float calculate_orbital_drift_angle(float step_s);

    int last_num_harvesting = 0;
    int current_num_harvesting = 0;

    bool force_draw_expanded_window = false;
    bool expanded_window_clicked = false;

    virtual ~orbital(){}

    void do_serialise(serialise& s, bool ser) override;

    void ensure_handled_by_client();
};

float get_orbital_update_rate(orbital_info::type type);

struct popup_info;
struct fleet_manager;

struct orbital_system : serialisable
{
    ///can never be true, is only here because of a bit of a screwup with the networking
    bool cleanup = false;

    vec2f universe_pos = {0,0};

    orbital* get_base();

    std::vector<orbital*> orbitals;
    std::vector<orbital*> asteroids; ///including useless ones

    std::set<empire*> advertised_empires;

    orbital* make_new(orbital_info::type type, float rad, int num_verts = 5);

    orbital* make_in_place(orbital* o);

    bool owns(orbital* o);

    void ensure_found_orbitals_handled();

    ///with 0 ships
    orbital* make_fleet(fleet_manager& fm, float rad, float angle, empire* e = nullptr);

    void vision_test_all();

    void tick(float step_s, orbital_system* viewed_system);

    void destroy(orbital*);
    ///non destructively reparent
    void steal(orbital*, orbital_system* s);

    ///currently viewed empire is drawn differently, see
    void draw(sf::RenderWindow& win, empire* viewer_empire);

    void cull_empty_orbital_fleets_deferred(popup_info& popup);

    orbital* get_by_element(void* element);

    void generate_asteroids_new(int n, int num_belts, int num_resource_asteroids);
    void generate_asteroids_old(int n, int num_belts, int num_resource_asteroids);
    void generate_planet_resources(float max_ps);

    void draw_alerts(sf::RenderWindow& win, empire* viewing_empire, system_manager& system_manage);

    void generate_random_system(int planets, int num_asteroids, int num_belts, int num_resource_asteroids);
    void generate_full_random_system(bool force_planet = false);

    bool highlight = false;

    ///hostile fleets
    std::vector<orbital*> get_fleets_within_engagement_range(orbital* me, bool only_hostile);

    bool can_engage(orbital* me, orbital* them);

    void make_asteroid_orbital(orbital* o);

    float accumulated_nonviewed_time = 0.f;

    bool is_owned();

    std::string get_resource_str(bool include_vision, empire* viewer_empire, bool only_owned);
    resource_manager get_potential_resources();

    ///for overall fleet window
    bool toggle_fleet_ui = true;

    static int gid;
    int unique_id = gid++;

    void do_serialise(serialise& s, bool ser) override;

    void shuffle_networked_orbitals();
};

struct popup_info;

struct system_manager : serialisable
{
    sf::Texture fleet_tex;
    sf::Sprite fleet_sprite;

    system_manager();

    std::vector<std::vector<orbital_system*>> to_draw_pathfinding;

    std::vector<orbital_system*> systems;

    ///for when we go into battle view, bit of a hack
    orbital_system* backup_system = nullptr;

    orbital_system* currently_viewed = nullptr;
    orbital_system* hovered_system = nullptr;

    orbital_system* make_new();

    ///perf issue?
    orbital_system* get_parent(orbital* o);
    orbital_system* get_by_element(void* ptr);
    orbital* get_by_element_orbital(void* ptr);

    std::vector<orbital_system*> pathfind(float max_warp_distance, orbital_system* start, orbital_system* fin);
    std::vector<orbital_system*> pathfind(orbital* o, orbital_system* fin);
    void add_draw_pathfinding(const std::vector<orbital_system*>& path);

    std::vector<orbital_system*> get_nearest_n(orbital_system* os, int n);

    std::vector<orbital*> next_frame_warp_radiuses;

    void tick(float step_s);

    void destroy(orbital_system* s);

    void repulse_fleets();

    void cull_empty_orbital_fleets_deferred(popup_info& popup);
    void destroy_cleanup(empire_manager& empire_manage);

    void add_selected_orbital(orbital* o);

    void draw_warp_radiuses(sf::RenderWindow& win, empire* viewing_empire);
    void draw_alerts(sf::RenderWindow& win, empire* viewing_empire);

    void draw_viewed_system(sf::RenderWindow& win, empire* viewer_empire);

    void set_viewed_system(orbital_system* s, bool reset_zoom = true);

    void draw_universe_map(sf::RenderWindow& win, empire* viewer_empire, popup_info& popup);
    void process_universe_map(sf::RenderWindow& win, bool lclick, empire* viewer_empire);

    ///camera. Set here because zoom will be useful
    ///Camera panning should also probably go here
    void change_zoom(float zoom, vec2f mouse_pos, sf::RenderWindow& win);
    void set_zoom(float zoom, bool auto_enter_system = false);
    void pan_camera(vec2f dir);

    bool in_system_view();
    void enter_universe_view();
    orbital_system* get_nearest_to_camera();

    float zoom_level = 1.f;
    vec2f camera;
    static float universe_scale;
    float sun_universe_rad = 200;
    float border_universe_rad = sun_universe_rad * 6.5;

    float temperature_fraction_to_star_size(float temperature_frac);

    ///also generate empires in universe, in a separate function
    void generate_universe(int num);

    void draw_ship_ui(empire* viewing_empire, popup_info& popup);

    //sf::RenderTexture temp;

    bool suppress_click_away_fleet = false;
    std::vector<orbital*> hovered_orbitals;
    std::vector<orbital*> advertised_universe_orbitals;

    void do_serialise(serialise& s, bool ser) override;

    void ensure_found_orbitals_handled();

    void erase_all();

    void shuffle_networked_orbitals();
};

#endif // SYSTEM_MANAGER_HPP_INCLUDED
