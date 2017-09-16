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
    //return (pow(10, frac) - 1) / 9;

    return frac;

    //return pow(2.f, frac) - 1;

    //return frac * frac * frac;
}

float proj(float zoom)
{
    float val = pow(sqrt(2), zoom);

    return std::max(val, min_zoom);
}

float unproj(float zoom)
{
    return log(zoom) / log(sqrt(2));
}

float min_zoom = 1/256.f;

void zoom_handler::tick(float dt_s)
{
    if(is_zoom_accum)
    {
        if(signum(zoom_accum) == signum(destination_zoom_level - zoom_level))
        {
            zoom_level = unproj(get_zoom());

            destination_zoom_level += zoom_accum;

            camera_offset = potential_camera_offset;
            last_camera_offset = {0,0};
        }
        else
        {
            zoom_level = unproj(get_zoom());

            destination_zoom_level = zoom_level + zoom_accum;

            camera_offset = potential_camera_offset;
            last_camera_offset = {0,0};
        }

        //zoom_level = unproj(get_zoom());

        //destination_zoom_level += zoom_accum;

        camera_offset = potential_camera_offset;
        last_camera_offset = {0,0};

        destination_time = current_time + zoom_time;

        zoom_accum = 0;
        //destination_zoom_level = std::max(destination_zoom_level, min_zoom);
    }


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

        //zoom_level = std::max(zoom_level, min_zoom);

        camera_offset = {0,0};
    }

    std::cout << get_zoom() << std::endl;
}

float zoom_handler::get_linear_zoom()
{
    if(current_time >= destination_time)
    {
        return zoom_level;
    }

    float frac = (destination_time - current_time) / zoom_time;

    frac = clamp(frac, 0.f, 1.f);

    frac = ease_function(frac);

    float res = zoom_level * frac + destination_zoom_level * (1.f - frac);

    return res;
}

float zoom_handler::get_zoom()
{
    if(current_time >= destination_time)
    {
        return proj(zoom_level);
    }

    float frac = (destination_time - current_time) / zoom_time;

    frac = clamp(frac, 0.f, 1.f);

    frac = ease_function(frac);

    float res = proj(zoom_level) * frac + proj(destination_zoom_level) * (1.f - frac);

    //res = proj(res);

    res = std::max(res, min_zoom);

    return res;
}

float zoom_handler::get_destination_zoom()
{
    return destination_zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    zoom_level = zoom;

    zoom_level = std::max(zoom_level, min_zoom);
    destination_zoom_level = zoom_level;

    destination_time = current_time - 1;
}

void zoom_handler::offset_zoom(float amount, sf::RenderWindow& win, vec2f mouse_pos, vec2f pcamera_offset)
{
    /*float scale_frac = (zoom - current_zoom);

    vec2f rel = mouse_pos - (vec2f){win.getSize().x, win.getSize().y}/2.f;

    camera_offset += -scale_frac * rel;*/

    float old_proj_zoom = proj(get_linear_zoom());
    float new_proj_zoom = proj(destination_zoom_level + zoom_accum + amount);

    vec2f rel = mouse_pos - (vec2f){win.getSize().x, win.getSize().y}/2.f;

    float scale_frac = (new_proj_zoom - old_proj_zoom);

    pcamera_offset = -scale_frac * rel;

    zoom_accum += amount;
    is_zoom_accum = true;

    ///restrict to zoom in
    if(amount < 0)
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
