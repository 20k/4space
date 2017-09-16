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

float ease_function(float frac)
{
    return frac;
}

void zoom_handler::tick(float dt_s)
{
    /*if(current_time >= destination_time)
    {
        zoom_level = destination_zoom_level;
    }*/

    float min_zoom = 1.f / 1024.f;

    advertised_offset = {0,0};

    #ifdef OLD_ZOOM
    if(zooming)
    {
        /*double diff = (destination_time - current_time);

        destination_time = current_time + zoom_time;

        destination_zoom_level = greatest_zoom_diff_target;

        destination_zoom_level = std::max(destination_zoom_level, min_zoom);*/

        zoom_level = greatest_zoom_diff_target;

        zoom_level = std::max(zoom_level, min_zoom);
        destination_zoom_level = zoom_level;

        destination_time = current_time - 1;
    }
    #endif

    ///need to handle reversing mouse
    if(is_zoom_accum)
    {
        if(signum(zoom_accum) == signum(destination_zoom_level - zoom_level))
        {
            zoom_level = get_zoom();

            destination_zoom_level += zoom_accum;

            camera_offset = potential_camera_offset;
            last_camera_offset = {0,0};
        }
        else
        {
            zoom_level = get_zoom();

            destination_zoom_level = zoom_level + zoom_accum;

            camera_offset = potential_camera_offset;
            last_camera_offset = {0,0};
        }

        destination_time = current_time + zoom_time;
        zoom_accum = 0;
        destination_zoom_level = std::max(destination_zoom_level, min_zoom);
    }

    greatest_zoom_diff_target = destination_zoom_level;
    greatest_zoom_diff_zoom = get_zoom();

    zooming = false;
    is_zoom_accum = false;

    potential_camera_offset = {0,0};

    float frac = (destination_time - current_time) / zoom_time;
    frac = clamp(frac, 0.f, 1.f);
    frac = ease_function(frac);
    last_camera_offset = mix((vec2f){0.f, 0.f}, camera_offset, frac);

    current_time += dt_s;

    if(current_time >= destination_time)
    {
        zoom_level = destination_zoom_level;

        //advertised_offset += dt_s * camera_offset / zoom_time;

        camera_offset = {0,0};
    }
    else
    {
        //advertised_offset += dt_s * camera_offset / zoom_time;
    }
}

float zoom_handler::get_zoom()
{
    if(current_time >= destination_time)
    {
        return zoom_level;
    }

    float frac = (destination_time - current_time) / zoom_time;

    frac = clamp(frac, 0.f, 1.f);

    frac = ease_function(frac);

    return zoom_level * frac + destination_zoom_level * (1.f - frac);
}

float zoom_handler::get_destination_zoom()
{
    return destination_zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    /*greatest_zoom_diff_target = zoom;
    zooming = true;*/

    float min_zoom = 1.f / 1024.f;

    zoom_level = zoom;

    zoom_level = std::max(zoom_level, min_zoom);
    destination_zoom_level = zoom_level;

    destination_time = current_time - 1;
}

void zoom_handler::offset_zoom(float amount, vec2f pcamera_offset)
{
    zoom_accum += amount;
    is_zoom_accum = true;

    potential_camera_offset += pcamera_offset;
}

vec2f zoom_handler::get_camera_offset()
{
    //if(current_time >= destination_time)
    //    return {0,0};

    float frac = (destination_time - current_time) / zoom_time;

    frac = clamp(frac, 0.f, 1.f);

    frac = ease_function(frac);

    vec2f current_camera_abs = mix((vec2f){0.f, 0.f}, camera_offset, frac);

    return current_camera_abs - last_camera_offset;

    //return advertised_offset;
}
