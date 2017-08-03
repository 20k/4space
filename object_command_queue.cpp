#include "object_command_queue.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include <SFML/Graphics.hpp>


float get_length_circle(vec2f p1, vec2f p2)
{
    float max_rad = std::max(p1.length(), p2.length());

    p1 = p1.norm() * max_rad;
    p2 = p2.norm() * max_rad;

    //float a = (p1 - p2).length();

    float A = acos(clamp(dot(p1.norm(), p2.norm()), -1.f, 1.f));

    //float r = sqrt(a*a / (2 * (1 - cos(A))));
    ///fuck we know r its just p1.length()

    float r = p1.length();

    return fabs(r * A);
}


///returns if finished
bool do_transfer(orbital* o, float diff_s, queue_type& type)
{
    object_command_queue_info::queue_element_data& data = type.data;

    vec2f end_pos = data.new_radius * (vec2f){cos(data.new_angle), sin(data.new_angle)};

    float straightline_distance = (end_pos - o->absolute_pos).length();

    float linear_extra = fabs(end_pos.length() - o->absolute_pos.length());
    float radius_extra = get_length_circle(o->absolute_pos, end_pos);

    float dist = sqrt(linear_extra * linear_extra + radius_extra*radius_extra);

    float a1 = o->absolute_pos.angle();
    float a2 = end_pos.angle();

    if(fabs(a2 + 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 += 2*M_PI;
    }

    if(fabs(a2 - 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 -= 2*M_PI;
    }

    float cur_angle = (o->absolute_pos - o->parent->absolute_pos).angle();
    float cur_rad = (o->absolute_pos - o->parent->absolute_pos).length();

    float iangle = cur_angle + diff_s * (a2 - a1);
    float irad = cur_rad + diff_s * (data.new_radius - cur_rad);

    vec2f calculated_next = irad * (vec2f){cos(iangle), sin(iangle)};
    vec2f calculated_cur = o->orbital_length * (vec2f){cos(o->orbital_angle), sin(o->orbital_angle)};

    float speed = 5.f;

    ///this is the real speed here
    vec2f calc_dir = (calculated_next - calculated_cur).norm() / speed;

    vec2f calc_real_next = calculated_cur + calc_dir;

    iangle = calc_real_next.angle();
    irad = calc_real_next.length();

    o->orbital_angle = iangle;
    o->orbital_length = irad;

    if(straightline_distance < 2)
    {
        //o->transferring = false;
        return true;
    }

    return false;
}

void object_command_queue::transfer(float pnew_rad, float pnew_angle, orbital* o)
{
    queue_type next;

    next.data.old_radius = o->orbital_length;
    next.data.old_angle = o->orbital_angle;
    next.data.new_radius = pnew_rad;
    next.data.new_angle = pnew_angle;

    next.data.start_time_s = o->internal_time_s;

    next.type = object_command_queue_info::IN_SYSTEM_PATH;

    add(next);
}

void object_command_queue::transfer(vec2f pos, orbital* o)
{
    vec2f base;

    if(o->parent)
        base = o->parent->absolute_pos;

    vec2f rel = pos - base;

    transfer(rel.length(), rel.angle(), o);
}

bool object_command_queue::transferring()
{
    if(command_queue.size() == 0)
        return false;

    return command_queue.front().type == object_command_queue_info::IN_SYSTEM_PATH;
}

/*void object_command_queue::add(object_command_queue_info::queue_element_type type, const object_command_queue_info::queue_element_data& data)
{
    return add({type, data})
}*/

void object_command_queue::add(const queue_type& type)
{
    sf::Keyboard key;

    if(!key.isKeyPressed(sf::Keyboard::LShift))
        cancel();

    command_queue.push(type);
}

void object_command_queue::tick(orbital* o, float diff_s)
{
    if(command_queue.size() == 0)
        return;

    auto next = command_queue.front();
    object_command_queue_info::queue_element_type cur = next.type;

    ///just do like, ship->tick_path destination etc
    if(cur == object_command_queue_info::IN_SYSTEM_PATH)
    {
        if(do_transfer(o, diff_s, next))
        {
            should_pop = true;
        }
    }

    if(cur == object_command_queue_info::WARP)
    {

    }

    if(cur == object_command_queue_info::COLONISE)
    {

    }

    if(cur == object_command_queue_info::FIGHT)
    {

    }

    if(is_front_complete() && command_queue.size() > 0)
    {
        command_queue.pop();
        should_pop = false;
    }
}

bool object_command_queue::is_front_complete()
{
    return should_pop;
}

void object_command_queue::cancel()
{
    while(!command_queue.empty())
    {
        command_queue.pop();
    }
}
