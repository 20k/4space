#ifndef OBJECT_COMMAND_QUEUE_HPP_INCLUDED
#define OBJECT_COMMAND_QUEUE_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>
#include <queue>

struct orbital_system;
struct orbital;

namespace object_command_queue_info
{
    enum queue_element_type
    {
        IN_SYSTEM_PATH,
        WARP,
        COLONISE,
        FIGHT,
    };

    ///given that target might become invalid, we need to check it still exists or clear it if invalid
    ///creates memory management difficulties in this method. Is there a better way? Targeting an orbital system is fine
    ///they're never going away. But orbital* ship targets might
    struct queue_element_data
    {
        queue_element_type type;

        vec2f pos = {0,0};
        orbital_system* dest = nullptr;
        orbital* target = nullptr;
    };
}

using queue_type = object_command_queue_info::queue_element_type;

struct object_command_queue
{
    std::queue<queue_type> command_queue;

    void add(queue_type type);
    void tick(orbital* parent);

    bool is_front_complete();

    bool should_pop = false;
};

#endif // OBJECT_COMMAND_QUEUE_HPP_INCLUDED
