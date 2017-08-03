#include "object_command_queue.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

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
