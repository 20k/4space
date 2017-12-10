#include "networking.hpp"

void network_state::tick_join_game(float dt_s)
{
    if(my_id != -1)
        return;

    if(!try_join)
        return;

    if(!sock.valid())
    {
        //sock = udp_connect("77.96.132.101", GAMESERVER_PORT);
        sock = udp_connect("192.168.0.54", GAMESERVER_PORT);
        sock_set_non_blocking(sock, 1);
    }

    timeout += dt_s;

    if(timeout > timeout_max)
    {
        send_join_game(sock);

        timeout = 0;
    }
}

void network_state::leave_game()
{
    sock.close();

    my_id = -1;

    try_join = false;
}

serialisable* network_state::get_serialisable(serialise_host_type& host_id, serialise_data_type& serialise_id)
{
    return serialise_data_helper::host_to_id_to_pointer[host_id][serialise_id];
}

bool network_state::connected()
{
    return (my_id != -1) && (sock.valid());
}

bool network_state::owns(serialisable* s)
{
    return s->owned_by_host;
}

void network_state::claim_for(serialisable* s, serialise_host_type new_host)
{
    if(s == nullptr)
        return;

    if(new_host == my_id)
    {
        s->owned_by_host = true;
    }
    else
    {
        s->owned_by_host = false;
    }
}

void network_state::claim_for(serialisable& s, serialise_host_type new_host)
{
    if(new_host == my_id)
    {
        s.owned_by_host = true;
    }
    else
    {
        s.owned_by_host = false;
    }
}

void network_state::forward_data(const network_object& no, serialise& s)
{
    if(!connected())
        return;

    reliable_ordered.forward_data_to_server(sock, (const sockaddr*)&store, no, s);
}

void network_state::tick(double dt_s)
{
    tick_join_game(dt_s);

    if(!connected())
        return;
}
