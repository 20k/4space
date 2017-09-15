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

float zoom_handler::get_zoom()
{
    return zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    zoom_level = zoom;
}
