#include "object_command_queue.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

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

/*void object_command_queue::add(object_command_queue_info::queue_element_type type, const object_command_queue_info::queue_element_data& data)
{
    return add({type, data})
}*/

void object_command_queue::add(const queue_type& type)
{
    command_queue.push(type);
}

void object_command_queue::tick(orbital* o)
{
    if(command_queue.size() == 0)
        return;

    object_command_queue_info::queue_element_type cur = command_queue.front().type;

    ///just do like, ship->tick_path destination etc
    if(cur == object_command_queue_info::IN_SYSTEM_PATH)
    {

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
