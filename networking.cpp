#include "networking.hpp"

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
    //s.host_id = new_id;

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

}

void network_state::tick(double dt_s)
{

}
