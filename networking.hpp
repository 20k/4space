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
///we can probably fix 'bad' networking with a combination of lzma, interpolation, updating state when necessary
///(ie on ship hit update its stats to ensure it doesn't take 1 second for it to update)
///very infrequently networking stuff the clients aren't looking at

struct network_object
{
    serialise_owner_type owner_id = -1;
    serialise_data_type serialise_id = -1;
};

struct network_data
{
    network_object object;
    serialise data;
    bool should_cleanup = false;
};

struct network_state
{
    int my_id = -1;
    int16_t next_object_id = 0;
    udp_sock sock;
    sockaddr_storage store;
    bool have_sock = false;

    bool connected()
    {
        return (my_id != -1) && (sock.valid());
    }

    float timeout_max = 5.f;
    float timeout = timeout_max;

    std::vector<network_data> available_data;

    void tick_join_game(float dt_s)
    {
        if(my_id != -1)
            return;

        timeout += dt_s;

        if(timeout > timeout_max)
        {
            sock = join_game("127.0.0.1", GAMESERVER_PORT);

            timeout = 0;
        }
    }

    void leave_game()
    {
        sock.close();

        my_id = -1;
    }

    void tick()
    {
        if(!sock.valid())
            return;

        bool any_recv = true;

        while(any_recv && sock_readable(sock))
        {
            auto data = udp_receive_from(sock, &store);

            any_recv = data.size() > 0;

            byte_fetch fetch;
            fetch.ptr.swap(data);

            //this_frame_stats.bytes_in += data.size();

            while(!fetch.finished() && any_recv)
            {
                int32_t found_canary = fetch.get<int32_t>();

                while(found_canary != canary_start && !fetch.finished())
                {
                    found_canary = fetch.get<int32_t>();
                }

                int32_t type = fetch.get<int32_t>();

                if(type == message::FORWARDING)
                {
                    uint32_t data_size = fetch.get<uint32_t>();

                    network_object no = fetch.get<network_object>();

                    serialise s;
                    s.data = fetch.ptr;
                    s.internal_counter = fetch.internal_counter;

                    available_data.push_back({no, s, false});

                    for(int i=0; i < (data_size - sizeof(network_object)) && i < 512; i++)
                    {
                        fetch.get<uint8_t>();
                    }

                    auto found_end = fetch.get<decltype(canary_end)>();

                    if(found_end != canary_end)
                    {
                        printf("forwarding error\n");
                    }
                }

                if(type == message::CLIENTJOINACK)
                {
                    int32_t recv_id = fetch.get<int32_t>();

                    int32_t canary_found = fetch.get<int32_t>();

                    if(canary_found == canary_end)
                        my_id = recv_id;
                    else
                    {
                        printf("err in CLIENTJOINACK\n");
                    }
                }

                if(type == message::PING_DATA)
                {
                    int num = fetch.get<int32_t>();

                    for(int i=0; i<num; i++)
                    {
                        int pid = fetch.get<int32_t>();
                        float ping = fetch.get<float>();
                    }

                    int32_t found_end = fetch.get<decltype(canary_end)>();

                    if(found_end != canary_end)
                    {
                        printf("err in PING_DATA\n");
                    }
                }

                if(type == message::PING)
                {
                    fetch.get<decltype(canary_end)>();
                }
            }
        }
    }

    /*void forward_data(int player_id, int object_id, int system_network_id, const byte_vector& vec)
    {
        network_variable nv(player_id, object_id, system_network_id);

        uint32_t data_size = sizeof(nv) + vec.ptr.size();

        byte_vector cv;
        cv.push_back(canary_start);
        cv.push_back(message::FORWARDING);
        cv.push_back(data_size);
        cv.push_back<network_variable>(nv);
        cv.push_vector(vec);
        cv.push_back(canary_end);

        udp_send_to(sock, cv.ptr, (const sockaddr*)&store);
    }

    int16_t get_next_object_id()
    {
        return next_object_id++;
    }*/

    /*template<typename manager_type, typename real_type>
    void check_create_network_entity(manager_type& generic_manager)
    {
        for(auto& i : available_data)
        {
            network_variable& var = std::get<0>(i);

            if(var.system_network_id != generic_manager.system_network_id)
                continue;

            if(var.player_id == my_id)
                continue;

            if(generic_manager.owns(var.object_id, var.player_id))
                continue;

            ///make new slave entity here!

            ///when reading this, ignore the template keyword
            ///its because this is a dependent type
            auto new_entity = generic_manager.template make_new<real_type>();

            real_type* found_entity = dynamic_cast<real_type*>(new_entity);

            //real_type* found_entity = dynamic_cast<real_type*>(generic_manager.make_new<real_type>());

            found_entity->object_id = var.object_id;
            found_entity->set_owner(var.player_id);
            found_entity->ownership_class = var.player_id;
        }
    }*/

    void tick_cleanup()
    {
        for(int i=0; i<available_data.size(); i++)
        {
            if(available_data[i].should_cleanup)
            {
                available_data.erase(available_data.begin() + i);
                i--;
                continue;
            }
        }
    }
};


#endif // NETWORKING_HPP_INCLUDED
