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

    float get_zoom();
    void set_zoom(float zoom);
};

#endif // CAMERA_HPP_INCLUDED
