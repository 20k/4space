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

    //return frac;

    //return pow(2.f, frac) - 1;

    return frac * frac * frac * frac;
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

zoom_handler::zoom_handler()
{
    zoom_level = proj(1);
    destination_zoom_level = proj(1);
}

void zoom_handler::tick(float dt_s)
{
    if(is_zoom_accum)
    {
        if(signum(zoom_accum) == signum(destination_zoom_level - zoom_level))
        {
            zoom_level = unproj(get_zoom());

            destination_zoom_level += zoom_accum;
        }
        else
        {
            zoom_level = unproj(get_zoom());

            destination_zoom_level = zoom_level + zoom_accum;
        }

        camera_offset = potential_camera_offset;
        last_camera_offset = {0,0};

        destination_time = current_time + zoom_time;

        zoom_accum = 0;
    }

    is_zoom_accum = false;

    potential_camera_offset = {0,0};

    last_camera_offset = get_camera_pos();

    current_time += dt_s;

    if(current_time >= destination_time)
    {
        zoom_level = destination_zoom_level;

        camera_offset = {0,0};
    }
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

///the problem is, we need to interpolate in linear space
///but then that breaks the camera panning mechanics as they inherently assume projected space
float zoom_handler::get_zoom()
{
    if(current_time >= destination_time)
    {
        return proj(zoom_level);
    }

    float frac = (destination_time - current_time) / zoom_time;

    frac = clamp(frac, 0.f, 1.f);

    frac = ease_function(frac);

    float res = (zoom_level) * frac + (destination_zoom_level) * (1.f - frac);

    res = proj(res);

    res = std::max(res, min_zoom);

    return res;
}

float zoom_handler::get_destination_zoom()
{
    return destination_zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    zoom_level = unproj(zoom);

    zoom_level = std::max(zoom_level, min_zoom);
    destination_zoom_level = zoom_level;

    destination_time = current_time - 1;
}

void zoom_handler::offset_zoom(float amount, sf::RenderWindow& win, vec2f mouse_pos)
{
    float old_proj_zoom = get_zoom();
    float new_proj_zoom = proj(destination_zoom_level + zoom_accum + amount);

    if(signum(amount) != signum(destination_zoom_level - zoom_level))
    {
        new_proj_zoom = proj(unproj(get_zoom()) + amount);
    }

    vec2f rel = mouse_pos - (vec2f){win.getSize().x, win.getSize().y}/2.f;

    float scale_frac = (new_proj_zoom - old_proj_zoom);

    vec2f next_camera_offset = -scale_frac * rel;

    zoom_accum += amount;
    is_zoom_accum = true;

    ///restrict to zoom in
    if(amount < 0)
        potential_camera_offset += next_camera_offset;
}

vec2f zoom_handler::get_camera_pos()
{
    float diff = (proj(destination_zoom_level) - proj(zoom_level));

    if(fabs(diff) < 0.001f)
        return {0,0};

    float frac = (get_zoom() - proj(zoom_level)) / diff;

    frac = fabs(frac);

    vec2f current_camera_abs = mix((vec2f){0.f, 0.f}, camera_offset, 1.f - frac);

    return current_camera_abs;
}

///need to somehow tie camera offset directly to zoom levels
///so it can never be desync'ds
vec2f zoom_handler::get_camera_offset()
{
    return get_camera_pos() - last_camera_offset;
}
