#ifndef OBJECT_COMMAND_QUEUE_HPP_INCLUDED
#define OBJECT_COMMAND_QUEUE_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>
#include <deque>

struct orbital_system;
struct orbital;
struct ship_manager;
struct ship;

namespace object_command_queue_info
{
    enum queue_element_type
    {
        IN_SYSTEM_PATH,
        WARP,
        COLONISE,
        FIGHT,
        ANCHOR,
        ANCHOR_UI,
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
        bool combat_move = false;
        //bool cancel_immediately = false;
        bool target_drifts = false;


        vec2f pos = {0,0};
        orbital_system* fin = nullptr;
        orbital_system* transfer_within = nullptr;
        orbital* target = nullptr;

        ship_manager* sm = nullptr;

        orbital* colony_target = nullptr;
        int64_t colony_ship_id = -1;

        bool should_pop = false;

        orbital* anchor_target = nullptr;
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

    void transfer(float new_rad, float new_angle, orbital* o, orbital_system* viewing_system, bool at_back = true, bool combat_move = false, bool target_drifts = false, bool lshift = false);
    void transfer(vec2f pos, orbital* o, orbital_system* viewing_system, bool at_back = true, bool combat_move = false, bool target_drifts = false, bool lshift = false);
    bool transferring();
    bool trying_to_warp();

    bool is_currently_colonising();
    orbital* get_colonising_target();

    void try_warp(const std::vector<orbital_system*>& systems, bool queue_to_back = false);
    void try_warp(orbital_system* fin, bool queue_to_back = false, bool lshift = false);

    void colonise(orbital* target, ship* colony_ship, bool lshift = false);

    void anchor(orbital* target, bool lshift = false);
    void anchor_ui_state(bool lshift = false);

    bool is_currently(object_command_queue_info::queue_element_type type);
    bool is_ever(object_command_queue_info::queue_element_type type);

    //void add(object_command_queue_info::queue_element_data type, const object_command_queue_info::queue_element_data& data);
    void add(const queue_type& type, bool at_back = true, bool does_not_cancel_if_at_back = false, bool shift_queue = false);
    void tick(orbital* o, float step_s);

    void drift_applicable_transfer_targets(float step_s);

    bool is_front_complete();

    bool should_pop = false;

    void cancel();
    void cancel_internal(orbital* o);

    void remove_any_of(object_command_queue_info::queue_element_type type);

    std::vector<orbital_system*> get_warp_destinations();
    orbital_system* get_warp_destination();
};

#endif // OBJECT_COMMAND_QUEUE_HPP_INCLUDED
