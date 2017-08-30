#ifndef NETWORKING_HPP_INCLUDED
#define NETWORKING_HPP_INCLUDED

#include <net/shared.hpp>
#include "../4space_server/master_server/network_messages.hpp"
#include "serialise.hpp"

///intent: Make this file increasingly more general so eventually we can port it between projects

inline
udp_sock join_game(const std::string& address, const std::string& port)
{
    udp_sock sock = udp_connect(address, port);

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::CLIENTJOINREQUEST);
    vec.push_back(canary_end);

    udp_send(sock, vec.ptr);

    return sock;
}

///for the moment, do poor quality networking
///where its fully determined in advanced what will be networked out of systems
///per network tick
///granularity is on the level of functions that have a do_serialise method
///get full state transfer working first

#endif // NETWORKING_HPP_INCLUDED
