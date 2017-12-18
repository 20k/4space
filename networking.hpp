#ifndef NETWORKING_HPP_INCLUDED
#define NETWORKING_HPP_INCLUDED

#include <net/shared.hpp>
#include "../4space_server/master_server/network_messages.hpp"
#include "../4space_server/reliability_ordered_shared.hpp"
#include "../serialise/serialise.hpp"
#include <thread>
#include <mutex>

///intent: Make this file increasingly more general so eventually we can port it between projects

///for the moment, do poor quality networking
///where its fully determined in advanced what will be networked out of systems
///per network tick
///granularity is on the level of functions that have a do_serialise method
///get full state transfer working first
///we can probably fix 'bad' networking with a combination of lzma, interpolation, updating state when necessary
///(ie on ship hit update its stats to ensure it doesn't take 1 second for it to update)
///very infrequently networking stuff the clients aren't looking at

struct network_state
{
    int my_id = -1;
    udp_sock sock;
    sockaddr_storage store;
    bool have_sock = false;
    bool try_join = false;

    float timeout = 5;
    float timeout_max = 5;

    network_reliable_ordered reliable_ordered;

    void tick_join_game(float dt_s);

public:
    //unused
    void leave_game();

///THIS IS THE WHOLE EXPOSED API
    bool connected();

    bool owns(serialisable* s);

    void claim_for(serialisable* s, serialise_host_type new_host);
    void claim_for(serialisable& s, serialise_host_type new_host);

    std::vector<network_data> available_data;

    serialisable* get_serialisable(serialise_host_type& host_id, serialise_data_type& serialise_id);

    void forward_data(const network_object& no, serialise& s);

    void tick(double dt_s);
};


#endif // NETWORKING_HPP_INCLUDED
