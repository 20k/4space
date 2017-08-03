#ifndef OBJECT_COMMAND_QUEUE_HPP_INCLUDED
#define OBJECT_COMMAND_QUEUE_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>
#include <deque>

struct orbital_system;
struct orbital;
struct ship_manager;

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

        float old_radius = 0.f;
        float old_angle = 0.f;
        float new_radius = 0.f;
        float new_angle = 0.f;
        float start_time_s = 0.f;

        vec2f pos = {0,0};
        orbital_system* fin = nullptr;
        orbital_system* transfer_within = nullptr;
        orbital* target = nullptr;

        ship_manager* sm = nullptr;
    };

    struct queue_data
    {
        queue_element_type type;
        queue_element_data data;
    };
}

using queue_type = object_command_queue_info::queue_data;
//using queue_type = object_command_queue_info::queue_element_type;

///so, could deal with queueing vs interrupting by having state set
///don't want to push the full burden on the user
struct object_command_queue
{
    bool should_interrupt = false;

    std::deque<queue_type> command_queue;

    void transfer(float new_rad, float new_angle, orbital* o, orbital_system* viewing_system);
    void transfer(vec2f pos, orbital* o, orbital_system* viewing_system);
    bool transferring();
    bool trying_to_warp();

    void try_warp(orbital_system* fin);

    //void add(object_command_queue_info::queue_element_data type, const object_command_queue_info::queue_element_data& data);
    void add(const queue_type& type);
    void tick(orbital* o, float diff_s);

    bool is_front_complete();

    bool should_pop = false;

    void cancel();

    std::vector<orbital_system*> get_warp_destinations();
};

#endif // OBJECT_COMMAND_QUEUE_HPP_INCLUDED
