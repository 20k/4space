#include "camera.hpp"
#include <SFML/Graphics.hpp>

std::vector<camera> camera::camera_stack;
int camera::gid = 0;

void camera::push()
{
    camera_stack.push_back(*this);
}

void camera::pop()
{
    if(camera_stack.size() > 0)
    {
        camera_stack.pop_back();
    }
}


void camera::set_current_camera(const camera& cam)
{
    if(camera_stack.size() > 0)
    {
        camera_stack.back() = cam;
    }
}

camera camera::get_current_camera()
{
    if(camera_stack.size() > 0)
        return camera_stack.back();

    return camera();
}

view_handler::view_handler(sf::RenderWindow& win) : backup(win.getView()), saved(win)
{

}

void view_handler::set_camera(camera& cam)
{
    sf::View view = saved.getView();

    view.setCenter(cam.pos.x(), cam.pos.y());

    saved.setView(view);
}

view_handler::~view_handler()
{
    saved.setView(backup);
}

void zoom_handler::tick(float dt_s)
{
    current_time += dt_s;

    if(current_time >= destination_time)
    {
        zoom_level = destination_zoom_level;
    }

    float min_zoom = 1.f / 1024.f;

    if(zooming)
    {
        double diff = (destination_time - current_time);

        destination_time = current_time + zoom_time;

        destination_zoom_level = greatest_zoom_diff_target;

        destination_zoom_level = std::max(destination_zoom_level, min_zoom);
    }

    ///need to handle reversing mouse
    if(is_zoom_accum)
    {
        if(signum(zoom_accum) == signum(destination_zoom_level - zoom_level))
        {
            zoom_level = get_zoom();

            destination_zoom_level += zoom_accum;
        }
        else
        {
            zoom_level = get_zoom();

            destination_zoom_level = zoom_level + zoom_accum;
        }

        destination_time = current_time + zoom_time;
        zoom_accum = 0;
        destination_zoom_level = std::max(destination_zoom_level, min_zoom);
    }

    greatest_zoom_diff_target = destination_zoom_level;
    greatest_zoom_diff_zoom = get_zoom();

    zooming = false;
    is_zoom_accum = false;
}

float zoom_handler::get_zoom()
{
    if(current_time >= destination_time)
    {
        return zoom_level;
    }

    float frac = (destination_time - current_time) / zoom_time;

    return zoom_level * frac + destination_zoom_level * (1.f - frac);
}

float zoom_handler::get_destination_zoom()
{
    return destination_zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    greatest_zoom_diff_target = zoom;
    zooming = true;
}

void zoom_handler::offset_zoom(float amount)
{
    zoom_accum += amount;
    is_zoom_accum = true;
}
