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

struct zoom_handler
{
    float zoom_level = 1.f;
    float destination_zoom_level = 1.f;

    float greatest_zoom_diff_target = 1.f;
    float greatest_zoom_diff_zoom = 1.f;
    float zoom_accum = 0.f;

    double zoom_time = 1.f;
    double current_zoom_time = 1.f;

    double current_time = 0.;
    double destination_time = 0.;

    bool zooming = false;

    bool is_zoom_accum = false;

    void tick(float dt_s);

    float get_zoom();
    float get_destination_zoom();
    void set_zoom(float zoom);
    void offset_zoom(float amount);
};

#endif // CAMERA_HPP_INCLUDED
