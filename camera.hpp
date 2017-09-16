#ifndef CAMERA_HPP_INCLUDED
#define CAMERA_HPP_INCLUDED

#include <vec/vec.hpp>
#include <SFML/Graphics/View.hpp>

namespace sf
{
    struct RenderWindow;
}

struct camera
{
    static int gid;
    int id = gid++;

    vec2f pos;
    float zoom_level = 1.f;

    static std::vector<camera> camera_stack;

    void push();
    void pop();

    static void set_current_camera(const camera& cam);
    static camera get_current_camera();
};

struct view_handler
{
    sf::View backup;
    sf::RenderWindow& saved;

    view_handler(sf::RenderWindow& win);

    void set_camera(camera& cam);

    ~view_handler();
};

extern float min_zoom;

struct zoom_handler
{
    vec2f last_camera_offset = {0,0};
    vec2f camera_offset = {0,0};
    vec2f potential_camera_offset = {0,0};
    vec2f advertised_offset = {0,0};
    float zoom_level = 1.f;
    float destination_zoom_level = 1.f;

    float zoom_accum = 0.f;

    double zoom_time = 0.55f;

    double current_time = 0.;
    double destination_time = 0.;

    bool is_zoom_accum = false;

    void tick(float dt_s);

    float get_linear_zoom();
    float get_zoom();
    float get_destination_zoom();
    void set_zoom(float zoom);
    void offset_zoom(float amount, sf::RenderWindow& win, vec2f mouse_pos, vec2f pcamera_offset = {0,0});

    vec2f get_camera_offset();
};

#endif // CAMERA_HPP_INCLUDED
