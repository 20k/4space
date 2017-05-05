#include "ship_command_queue.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

void ship_command_queue::add(queue_type type)
{
    command_queue.push(type);
}

void ship_command_queue::tick(orbital* parent)
{
    if(command_queue.size() == 0)
        return;

    queue_type cur = command_queue.front();

    ///just do like, ship->tick_path destination etc
    if(cur == ship_command_queue_info::IN_SYSTEM_PATH)
    {

    }

    if(cur == ship_command_queue_info::WARP)
    {

    }

    if(cur == ship_command_queue_info::COLONISE)
    {

    }

    if(cur == ship_command_queue_info::FIGHT)
    {

    }

    while(is_front_complete() && command_queue.size() > 0)
    {
        command_queue.pop();
    }
}

bool ship_command_queue::is_front_complete()
{
    return false;
}
