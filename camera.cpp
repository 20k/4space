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

    if(zooming)
    {
        double diff = (destination_time - current_time);

        destination_time = current_time + zoom_time;

        if(diff > 0)
        {
            destination_time += diff;
        }

        destination_zoom_level = greatest_zoom_diff_target;

        //std::cout << "hi\n";
    }

    //std::cout << greatest_zoom_diff << "\n";

    greatest_zoom_diff_target = destination_zoom_level;
    greatest_zoom_diff_zoom = get_zoom();

    zooming = false;
}

float zoom_handler::get_zoom()
{
    if(current_time >= destination_time)
    {
        return zoom_level;
    }

    float frac = (destination_time - current_time) / zoom_time;

    //std::cout << 1.f - frac << " ";

    //float val =  mix(zoom_level, destination_zoom_level, 1.f - frac);

    //std::cout << zoom_level << " " << destination_zoom_level << " end";


    return zoom_level * frac + destination_zoom_level * (1.f - frac);

    //return val;

    //return zoom_level;
}

void zoom_handler::set_zoom(float zoom)
{
    /*if(fabs(zoom_level - zoom) > fabs(zoom_level - get_zoom()) + 0.1f &&
       fabs(destination_zoom_level - zoom) > fabs(destination_zoom_level - greatest_zoom_diff_target) + 0.1f)
    {
        greatest_zoom_diff_target = zoom;
        zooming = true;

        std::cout << "set " << zoom << std::endl;
    }*/

    greatest_zoom_diff_target = zoom;
    zooming = true;

    /*if(fabs(zoom - destination_zoom_level) < 0.1f)
    {
        //zooming = false;
    }
    else
    {
        destination_time = current_time + zoom_time;
        destination_zoom_level = (destination_zoom_level + zoom) / 2.f;
    }*/
}
