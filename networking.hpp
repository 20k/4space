#ifndef NETWORKING_HPP_INCLUDED
#define NETWORKING_HPP_INCLUDED

#include <net/shared.hpp>
#include "../4space_server/master_server/network_messages.hpp"
#include "serialise.hpp"

///intent: Make this file increasingly more general so eventually we can port it between projects

inline
void send_join_game(udp_sock& sock)
{
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::CLIENTJOINREQUEST);
    vec.push_back(canary_end);

    udp_send(sock, vec.ptr);
}

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
    ///who's sending the data
    serialise_owner_type owner_id = -1;
    serialise_data_type serialise_id = -1;
};

using packet_id_type = uint32_t;
using sequence_data_type = uint32_t;

///update so that when processed, we keep around for 10 seconds or so then delete
///when we cleanup processed data, should we tie this to cleaning up incomplete packets?
struct network_data
{
    network_object object;
    serialise data;
    packet_id_type packet_id;
    bool should_cleanup = false;
    bool processed = false;

    sf::Clock clk;

    void set_complete()
    {
        processed = true;
        clk.restart();
    }
};

inline
int get_max_packet_size_clientside()
{
    return 450;
}

inline
int get_packet_fragments(int data_size)
{
    int max_data_size = get_max_packet_size_clientside();

    int fragments = ceil((float)data_size / max_data_size);

    return fragments;
}

struct packet_info
{
    sequence_data_type sequence_number = 0;
    serialise data;
};

struct network_state
{
    int my_id = -1;
    int16_t next_object_id = 0;
    udp_sock sock;
    sockaddr_storage store;
    bool have_sock = false;
    bool try_join = false;

    packet_id_type packet_id = 0;

    bool connected()
    {
        return (my_id != -1) && (sock.valid());
    }

    float timeout_max = 5.f;
    float timeout = timeout_max;

    std::vector<network_data> available_data;
    std::map<serialise_owner_type, std::map<serialise_data_type, std::map<packet_id_type, std::vector<packet_info>>>> incomplete_packets;
    std::map<serialise_owner_type, std::map<packet_id_type, int>> owner_to_packet_sequence_to_expected_size;
    std::map<serialise_owner_type, std::map<packet_id_type, serialise_data_type>> owner_to_packet_id_to_serialise;

    ///ok. If this is nullptr, either the data is intentionally nullptr (impossible because we wouldn't
    ///be receiving updates about it)
    ///or we haven't discovered this object yet... in which case maybe its a good time to make
    ///a disk request about it, or whatever. That will be the most complex part of the networking
    /*serialisable* get_serialisable(network_object& obj)
    {
        return serialise_data_helper::owner_to_id_to_pointer[obj.owner_id][obj.serialise_id];
    }*/

    serialisable* get_serialisable(serialise_host_type& host_id, serialise_data_type& serialise_id)
    {
        return serialise_data_helper::host_to_id_to_pointer[host_id][serialise_id];
    }

    void tick_join_game(float dt_s)
    {
        if(my_id != -1)
            return;

        if(!try_join)
            return;

        if(!sock.valid())
            sock = udp_connect("127.0.0.1", GAMESERVER_PORT);

        timeout += dt_s;

        if(timeout > timeout_max)
        {
            send_join_game(sock);

            timeout = 0;
        }
    }

    void leave_game()
    {
        sock.close();

        my_id = -1;

        try_join = false;
    }

    #pragma pack(1)
    struct packet_header
    {
        uint32_t current_size;
        int32_t overall_size = 0;
        packet_id_type packet_id = 0;
        sequence_data_type sequence_number = 0;

        uint32_t calculate_size()
        {
            return sizeof(overall_size) + sizeof(packet_id) + sizeof(sequence_number);
        }
    };

    struct forward_packet
    {
        int32_t canary_first;
        message::message type;
        packet_header header;
        network_object no;
        byte_fetch fetch;
        int32_t canary_second;
    };

    forward_packet decode_forward(byte_fetch& fetch)
    {
        forward_packet ret;
        ret.canary_first = canary_start;
        ret.type = message::FORWARDING;
        ret.header = fetch.get<packet_header>();
        ret.no = fetch.get<network_object>();

        for(int i=0; i<ret.header.current_size - ret.header.calculate_size() - sizeof(network_object); i++)
        {
            ret.fetch.ptr.push_back(fetch.get<uint8_t>());
        }

        ret.canary_second = fetch.get<decltype(canary_end)>();

        return ret;
    }

    #pragma pack(1)
    struct packet_request
    {
        serialise_owner_type owner_id = 0;
        sequence_data_type sequence_id = 0;
        packet_id_type packet_id = 0;
        serialise_data_type serialise_id;
    };

    struct request_timeout_info
    {
        bool ever = false;
        sf::Clock clk;
    };

    bool owns(serialisable* s)
    {
        if(s->host_id == -1 || s->host_id == my_id)
            return true;

        return false;
    }

    void make_packet_request(packet_request& request)
    {
        byte_vector vec;
        vec.push_back(canary_start);
        vec.push_back(message::REQUEST);
        vec.push_back<int32_t>(sizeof(request));
        vec.push_back(request);
        vec.push_back(canary_end);

        while(!sock_writable(sock)) {}

        udp_send_to(sock, vec.ptr, (const sockaddr*)&store);
    }

    std::map<serialise_owner_type, std::map<packet_id_type, std::map<sequence_data_type, request_timeout_info>>> owner_to_request_timeouts;

    void request_incomplete_packets(int max_fragments_to_request)
    {
        int fragments_requested = 0;

        std::vector<packet_request> requests;

        for(auto& owners : incomplete_packets)
        {
            serialise_owner_type owner_id = owners.first;

            for(auto& serialise_ids : owners.second)
            {
                serialise_data_type serialise_id = serialise_ids.first;

                for(auto& packet_ids : serialise_ids.second)
                {
                    packet_id_type packet_id = packet_ids.first;

                    std::vector<packet_info>& packets = packet_ids.second;

                    //if(packets.size() == owner_to_packet_sequence_to_expected_size[owner_id][packet_id])
                    //    continue;

                    std::sort(packets.begin(), packets.end(), [](auto& p1, auto& p2){return p1.sequence_number < p2.sequence_number;});

                    if(packets[0].sequence_number != 0)
                    {
                        requests.push_back({owner_id, 0, packet_id});
                    }

                    for(int i=1; i<packets.size(); i++)
                    {
                        packet_info& last = packets[i-1];
                        packet_info& cur = packets[i];

                        if(cur.sequence_number - 1 != last.sequence_number)
                        {
                            requests.push_back({owner_id, cur.sequence_number - 1, packet_id});
                        }
                    }

                    for(int i=packets.size(); i<owner_to_packet_sequence_to_expected_size[owner_id][packet_id]; i++)
                    {
                        requests.push_back({owner_id, i, packet_id});
                    }
                }
            }
        }

        float request_timeout_s = 0.4f;

        for(int i=0; i<requests.size(); i++)
        {
            request_timeout_info& inf = owner_to_request_timeouts[requests[i].owner_id][requests[i].packet_id][requests[i].sequence_id];

            if(inf.clk.getElapsedTime().asMilliseconds() / 1000.f < request_timeout_s)
            {
                requests.erase(requests.begin() + i);
                i--;
                continue;
            }
        }

        int num = max_fragments_to_request;

        if(num < 0)
            num = 0;

        if(requests.size() > num)
        {
            requests.resize(num);
        }

        for(packet_request& i : requests)
        {
            i.serialise_id = owner_to_packet_id_to_serialise[i.owner_id][i.packet_id];

            make_packet_request(i);

            owner_to_request_timeouts[i.owner_id][i.packet_id][i.sequence_id].clk.restart();
            owner_to_request_timeouts[i.owner_id][i.packet_id][i.sequence_id].ever = true;
        }
    }

    ///hmm
    /*bool is_data_complete(int data_id)
    {
        if(data_id < 0 || data_id >= data_complete.size())
            return false;

        return true;
    }*/

    void cleanup_available_data_and_incomplete_packets()
    {
        float cleanup_time_s = 10.f;

        for(int i=0; i<available_data.size(); i++)
        {
            network_data& data = available_data[i];

            if(data.processed == true && data.clk.getElapsedTime().asMilliseconds() / 1000.f > cleanup_time_s)
            {
                network_object& net_obj = data.object;

                packet_id_type packet_id = data.packet_id;

                incomplete_packets[net_obj.owner_id][net_obj.serialise_id].erase(packet_id);

                available_data[i].should_cleanup = true;
            }
        }
    }

    std::map<serialise_owner_type, std::deque<forward_packet>> forward_ordered_packets;
    std::map<serialise_owner_type, packet_id_type> last_popped_id;

    //std::map<serialise_owner_type, packet_id_type> already_have;

    void make_available(const serialise_owner_type& owner, int id)
    {
        forward_packet& packet = forward_ordered_packets[owner][id];

        serialise s;
        s.data = std::move(packet.fetch.ptr);

        available_data.push_back({packet.no, std::move(s), packet.header.packet_id});

        //forward_ordered_packets[owner].erase(forward_ordered_packets[owner].begin() + id);
    }

    struct wait_info
    {
        sf::Clock clk;

        bool too_long()
        {
            return clk.getElapsedTime().asMilliseconds() > 500;
        }

        void request()
        {
            clk.restart();
        }
    };

    std::map<serialise_owner_type, std::map<packet_id_type, wait_info>> packet_wait_map;
    ///there's a few structures here and there to do with packets that actually just pile up infinitely
    ///needs to be a general "after 10 seconds or so clean these up as those packets aren't comin"
    std::map<serialise_owner_type, std::map<packet_id_type, bool>> received_packet;
    std::map<serialise_owner_type, packet_id_type> last_received_packet;

    void make_packets_available()
    {
        for(auto& packet_data : forward_ordered_packets)
        {
            std::deque<forward_packet>& packet_list = packet_data.second;

            std::sort(packet_list.begin(), packet_list.end(),
                [](auto& p1, auto& p2){return p1.header.packet_id < p2.header.packet_id;});

            ///I love you bjarne but also fuck you
            for(int i=0; i<((int)packet_list.size()) - 1; i++)
            {
                if(packet_list[i].header.packet_id == packet_list[i+1].header.packet_id)
                {
                    packet_list.erase(packet_list.begin() + i);
                    i--;
                    continue;
                }
            }

            for(int i=0; i<packet_list.size(); i++)
            {
                forward_packet& current_packet = packet_list[i];

                if(last_received_packet.find(current_packet.no.owner_id) == last_received_packet.end())
                {
                    last_received_packet[current_packet.no.owner_id] = current_packet.header.packet_id - 1;
                }

                if(current_packet.header.packet_id == last_received_packet[current_packet.no.owner_id] + 1)
                {
                    last_received_packet[current_packet.no.owner_id] = current_packet.header.packet_id;

                    packet_wait_map[current_packet.no.owner_id].erase(current_packet.header.packet_id);
                    make_available(current_packet.no.owner_id, 0);
                    packet_list.pop_front();
                    i--;
                    continue;
                }
            }

            int max_requests = 10;
            int requests = 0;

            for(int i=1; i<packet_list.size() && requests < max_requests; i++)
            {
                forward_packet& last = packet_list[i-1];
                forward_packet& cur = packet_list[i];

                if(last.header.packet_id + 1 != cur.header.packet_id)
                {
                    wait_info& info = packet_wait_map[cur.no.owner_id][last.header.packet_id];

                    if(!info.too_long())
                    {
                        requests++;
                        continue;
                    }

                    info.request();

                    packet_request request;
                    request.owner_id = last.no.owner_id;
                    request.packet_id = last.header.packet_id + 1;
                    request.sequence_id = 0;
                    request.serialise_id = owner_to_packet_id_to_serialise[request.owner_id][request.packet_id];

                    make_packet_request(request);

                    requests++;
                }
            }
        }
    }

    ///need to write ACKs for forwarding packets so we know to resend them if they didn't arrive
    ///I think the reason the data transfer is slow is because the framerate is bad
    ///if we threaded it i think the data transfer would go v high
    ///may be worth just threading the actual socket itself
    void tick()
    {
        if(!sock.valid())
            return;

        request_incomplete_packets(10);

        cleanup_available_data_and_incomplete_packets();

        make_packets_available();

        tick_cleanup();

        byte_vector v1;
        v1.push_back(canary_start);
        v1.push_back(message::KEEP_ALIVE);
        v1.push_back(canary_end);

        udp_send_to(sock, v1.ptr, (const sockaddr*)&store);

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

                if(type == message::REQUEST)
                {
                    int32_t len = fetch.get<int32_t>();

                    packet_request request = fetch.get<packet_request>();

                    fetch.get<int32_t>();

                    network_object no;
                    no.owner_id = request.owner_id;
                    no.serialise_id = request.serialise_id;

                    //auto vec = get_fragment(request.sequence_id, no, packet_id_to_sequence_number_to_data[request.packet_id][request.sequence_id].vec.ptr);

                    if(owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id].find(request.sequence_id) !=
                       owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id].end())
                    {
                        auto& vec = owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id][request.sequence_id];

                        //std::cout << vec.vec.ptr.size() << std::endl;

                        while(!sock_writable(sock)){}

                        udp_send_to(sock, vec.vec.ptr, (const sockaddr*)&store);
                    }
                    else
                    {
                        std::cout << "dont have data" << std::endl;
                    }
                }

                if(type == message::FORWARDING)
                {
                    forward_packet packet = decode_forward(fetch);

                    packet_header header = packet.header;

                    network_object no = packet.no;

                    int real_overall_data_length = header.overall_size - header.calculate_size() - sizeof(no);

                    int packet_fragments = get_packet_fragments(real_overall_data_length);

                    owner_to_packet_id_to_serialise[no.owner_id][header.packet_id] = no.serialise_id;

                    if(packet_fragments > 1)
                    {
                        owner_to_packet_sequence_to_expected_size[no.owner_id][header.packet_id] = packet_fragments;

                        std::vector<packet_info>& packets = incomplete_packets[no.owner_id][no.serialise_id][header.packet_id];

                        ///INSERT PACKET INTO PACKETS
                        packet_info next;
                        next.sequence_number = header.sequence_number;

                        //if((header.sequence_number % 100) == 0)
                        if(header.sequence_number > 400 && (header.sequence_number % 100) == 0)
                            std::cout << header.sequence_number << " ";

                        next.data.data = packet.fetch.ptr;

                        bool has = false;

                        for(packet_info& inf : packets)
                        {
                            if(inf.sequence_number == header.sequence_number)
                            {
                                has = true;
                                break;
                            }
                        }

                        if(!has)
                        {
                            packets.push_back(next);
                        }

                        int current_received_fragments = packets.size();

                        //std::cout << "FHAS " << current_received_fragments << " ";

                        if(current_received_fragments == packet_fragments)
                        {
                            //std::cout << "hello there \n";
                            //std::cout << packets.size() << " " << packet_fragments << std::endl;

                            std::sort(packets.begin(), packets.end(), [](packet_info& p1, packet_info& p2){return p1.sequence_number < p2.sequence_number;});

                            serialise s;

                            for(packet_info& packet : packets)
                            {
                                s.data.insert(std::end(s.data), std::begin(packet.data.data), std::end(packet.data.data));

                                packet.data.data.clear();
                            }

                            //std::cout << s.data.size() << std::endl;

                            ///pipe back a response?
                            if(s.data.size() > 0)
                            {
                                //std::cout << "got full dataset " << s.data.size() << std::endl;
                                //available_data.push_back({no, s, header.packet_id});

                                if(received_packet[packet.no.owner_id][packet.header.packet_id] == false)
                                {
                                    packet.fetch = byte_fetch();

                                    forward_packet full_forward = packet;

                                    full_forward.fetch.ptr = std::move(s.data);

                                    forward_ordered_packets[no.owner_id].push_back(std::move(full_forward));

                                    received_packet[packet.no.owner_id][packet.header.packet_id] = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        /*serialise s;
                        s.data = packet.fetch.ptr;

                        available_data.push_back({no, s, header.packet_id});*/

                        if(received_packet[no.owner_id][header.packet_id] == false)
                        {
                            forward_packet full_forward = packet;

                            full_forward.fetch.ptr = std::move(fetch.ptr);

                            forward_ordered_packets[no.owner_id].push_back(std::move(full_forward));

                            received_packet[packet.no.owner_id][packet.header.packet_id] = true;
                        }
                    }

                    auto found_end = packet.canary_second;

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

                    std::cout << recv_id << std::endl;
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

    byte_vector get_fragment(int id, const network_object& no, const std::vector<char>& data, packet_id_type use_packet_id)
    {
        int fragments = get_packet_fragments(data.size());

        sequence_data_type sequence_number = 0;

        packet_header header;
        header.current_size = data.size() + header.calculate_size() + sizeof(no);
        header.overall_size = header.current_size;
        header.packet_id = use_packet_id;

        if(fragments == 1)
        {
            byte_vector vec;

            vec.push_back(canary_start);
            vec.push_back(message::FORWARDING);
            vec.push_back(header);
            vec.push_back<network_object>(no);

            for(auto& i : data)
            {
                vec.push_back(i);
            }

            vec.push_back(canary_end);

            return vec;
        }

        int real_data_per_packet = ceil((float)data.size() / fragments);

        int sent = id * real_data_per_packet;
        int to_send = data.size();

        if(sent >= to_send)
            return byte_vector();

        byte_vector vec;

        int to_send_size = std::min(real_data_per_packet, to_send - sent);

        header.current_size = to_send_size + header.calculate_size() + sizeof(no);
        header.sequence_number = id;

        vec.push_back(canary_start);
        vec.push_back(message::FORWARDING);

        vec.push_back(header);
        vec.push_back<network_object>(no);

        for(int kk = 0; kk < real_data_per_packet && sent < to_send; kk++)
        {
            vec.push_back(data[sent]);

            sent++;
        }

        vec.push_back(canary_end);

        return vec;
    }

    struct resend_info
    {
        byte_vector vec;
    };

    std::map<serialise_owner_type, std::map<packet_id_type, std::map<sequence_data_type, resend_info>>> owner_to_packet_id_to_sequence_number_to_data;

    ///ok so the issue is that no.owner_id is wrong
    ///we're treating it like the client id, but its actually who owns the object
    ///which means that packet ids are getting trampled
    ///what we really need to do is make it the client's id, and then change
    ///change owns() to respect this. It should be a fairly transparent change
    void forward_data(const network_object& no, serialise& s)
    {
        int max_to_send = 10;

        int fragments = get_packet_fragments(s.data.size());

        for(int i=0; i<get_packet_fragments(s.data.size()); i++)
        {
            byte_vector frag = get_fragment(i, no, s.data, packet_id);

           // while(!sock_writable(sock)) {}

            ///ah. we have a packet id conflict, if we pipe down on no.owner_id, our packet id will overlap
            ///and the data will never get delivered
            ///no.owner_id actually needs to be the id of the client, not who owns the data
            owner_to_packet_id_to_sequence_number_to_data[no.owner_id][packet_id][i] = {frag};

            if(i < max_to_send)
            {
                while(!sock_writable(sock)) {}

                udp_send_to(sock, frag.ptr, (const sockaddr*)&store);
            }
        }

        packet_id = packet_id + 1;
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
