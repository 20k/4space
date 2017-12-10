#include "networking.hpp"

#include <net/shared.hpp>
#include "../4space_server/master_server/network_messages.hpp"
#include "serialise.hpp"
#include <thread>
#include <mutex>

bool network_state::connected()
{
    return (my_id != -1) && (sock.valid());
}


serialisable* network_state::get_serialisable(serialise_host_type& host_id, serialise_data_type& serialise_id)
{
    return serialise_data_helper::host_to_id_to_pointer[host_id][serialise_id];
}

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

network_state::forward_packet network_state::decode_forward(byte_fetch& fetch)
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

void network_state::make_packet_request(packet_request& request)
{
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::REQUEST);
    vec.push_back<int32_t>(sizeof(request));
    vec.push_back(request);
    vec.push_back(canary_end);

    //while(!sock_writable(sock)) {}

    udp_send_to(sock, vec.ptr, (const sockaddr*)&store);
}

void network_state::make_packet_ack(packet_ack& ack)
{
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::PACKET_ACK);
    vec.push_back<int32_t>(sizeof(ack));
    vec.push_back(ack);
    vec.push_back(canary_end);

    //while(!sock_writable(sock)) {}

    udp_send_to(sock, vec.ptr, (const sockaddr*)&store);
}

void network_state::request_incomplete_packets(int max_fragments_to_request)
{
    std::map<packet_id_type, std::vector<packet_request>> requests;

    for(auto& owners : incomplete_packets)
    {
        serialise_owner_type owner_id = owners.first;

        for(auto& packet_ids : owners.second)
        {
            packet_id_type packet_id = packet_ids.first;

            sequence_data_type& current_packet_sequence = current_packet_fragment[owner_id][packet_id];

            std::vector<packet_info>& packets = packet_ids.second;

            if(packets.size() == owner_to_packet_sequence_to_expected_size[owner_id][packet_id])
                continue;

            int total_requests = 0;

            std::sort(packets.begin(), packets.end(), [](auto& p1, auto& p2) {return p1.sequence_number < p2.sequence_number;});

            if(packets.size() > 0 && packets[0].sequence_number != 0)
            {
                for(sequence_data_type kk = 0; kk < packets[0].sequence_number; kk++)
                {
                    requests[packet_id].push_back({owner_id, kk, packet_id});
                }
            }

            sequence_data_type sequence_id = -1;

            for(int i=1; i<packets.size(); i++)
            {
                packet_info& last = packets[i-1];
                packet_info& cur = packets[i];

                for(int kk=last.sequence_number + 1; kk < cur.sequence_number; kk++)
                {
                    requests[packet_id].push_back({owner_id, kk, packet_id});
                }

                sequence_id = cur.sequence_number;
            }

            if(packets.size() == 1)
            {
                sequence_id = packets[0].sequence_number;
            }

            sequence_id++;

            for(; sequence_id < owner_to_packet_sequence_to_expected_size[owner_id][packet_id]; sequence_id++)
            {
                requests[packet_id].push_back({owner_id, sequence_id, packet_id});
            }
        }
    }

    int current_requests = 0;

    for(auto& req : requests)
    {
        for(packet_request& i : req.second)
        {
            if(current_requests >= max_fragments_to_request)
                return;

            if(!owner_to_request_timeouts[i.owner_id][i.packet_id][i.sequence_id].too_long())
                continue;

            owner_to_request_timeouts[i.owner_id][i.packet_id][i.sequence_id].request();

            i.serialise_id = owner_to_packet_id_to_serialise[i.owner_id][i.packet_id];

            make_packet_request(i);

            current_requests++;
        }
    }
}

///the only reason to defer this is in case we receive duplicates
void network_state::cleanup_available_data_and_incomplete_packets()
{
    float cleanup_time_s = 4.f;
    float hard_cleanup_time = 1.f;

    for(int i=0; i<available_data.size(); i++)
    {
        network_data& data = available_data[i];

        if(data.processed == true && data.clk.getElapsedTime().asMilliseconds() / 1000.f > cleanup_time_s)
        {
            network_object& net_obj = data.object;

            packet_id_type packet_id = data.packet_id;

            incomplete_packets[net_obj.owner_id].erase(packet_id);

            available_data[i].should_cleanup = true;
        }
    }
}

void network_state::make_available(const serialise_owner_type& owner, int id)
{
    forward_packet& packet = forward_ordered_packets[owner][id];

    serialise s;
    s.data = std::move(packet.fetch.ptr);

    available_data.push_back({packet.no, s, packet.header.packet_id});

    packet_ack ack;
    ack.owner_id = packet.no.owner_id;
    ack.packet_id = packet.header.packet_id;
    ack.sequence_id = packet.header.sequence_number;

    make_packet_ack(ack);
}

void network_state::make_packets_available()
{
    for(auto& packet_data : forward_ordered_packets)
    {
        std::deque<forward_packet>& packet_list = packet_data.second;

        std::sort(packet_list.begin(), packet_list.end(),
        [](auto& p1, auto& p2) {return p1.header.packet_id < p2.header.packet_id;});

        for(int i=0; i<(int)packet_list.size(); i++)
        {
            forward_packet& current_packet = packet_list[i];

            if(last_received_packet.find(current_packet.no.owner_id) == last_received_packet.end())
            {
                last_received_packet[current_packet.no.owner_id] = current_packet.header.packet_id - 1;
            }

            if(current_packet.header.packet_id <= last_received_packet[current_packet.no.owner_id] + 1)
            {
                if(current_packet.header.packet_id == last_received_packet[current_packet.no.owner_id] + 1)
                    last_received_packet[current_packet.no.owner_id]++;

                packet_wait_map[current_packet.no.owner_id].erase(current_packet.header.packet_id);
                make_available(current_packet.no.owner_id, 0);
                packet_list.pop_front();
                i--;
                continue;
            }
        }

        int max_requests = 50;
        int requests = 0;

        if(packet_list.size() > 0)
        {
            forward_packet& first = packet_list[0];

            packet_id_type next_packet_id = last_received_packet[first.no.owner_id] + 1;

            for(packet_id_type kk = next_packet_id; kk < first.header.packet_id && requests < max_requests; kk++)
            {
                wait_info& info = packet_wait_map[first.no.owner_id][kk];

                if(info.too_long())
                {
                    packet_request request;
                    request.owner_id = first.no.owner_id;
                    request.packet_id = kk;
                    request.sequence_id = 0;
                    request.serialise_id = owner_to_packet_id_to_serialise[request.owner_id][first.header.packet_id];

                    make_packet_request(request);
                    info.request();
                    requests++;
                }
            }
        }

        for(int i=1; i<packet_list.size() && requests < max_requests; i++)
        {
            forward_packet& last = packet_list[i-1];
            forward_packet& cur = packet_list[i];

            for(packet_id_type kk=last.header.packet_id + 1; kk < cur.header.packet_id; kk++)
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
                request.packet_id = kk;
                request.sequence_id = 0;
                ///this is invalid right?
                request.serialise_id = owner_to_packet_id_to_serialise[request.owner_id][last.header.packet_id];

                make_packet_request(request);

                requests++;
            }
        }
    }
}

bool network_state::disconnected(const serialise_owner_type& type)
{
    if(disconnection_timer[type].getElapsedTime().asSeconds() > 10)
        return true;

    return false;
}

void network_state::thread_network_receive(network_state* net_state)
{
    while(1)
    {
        int max_mutex_recv = 200;
        int cur_recv = 0;

        Sleep(1);

        std::lock_guard<std::mutex> guard(net_state->threaded_network_mutex);

        net_state->request_incomplete_packets(100);

        bool any_recv = true;

        while(any_recv && sock_readable(net_state->sock))
        {
            auto data = udp_receive_from(net_state->sock, &net_state->store);

            any_recv = data.size() > 0;

            net_state->threaded_network_receive.ptr.insert(net_state->threaded_network_receive.ptr.end(), data.begin(), data.end());

            ///very basic dos protection
            if(cur_recv >= max_mutex_recv)
                break;

            cur_recv++;
        }

        if(net_state->should_die)
            return;
    }
}


network_state::network_state() : net_thread(&network_state::thread_network_receive, this)
{

}

network_state::~network_state()
{
    should_die = 1;
    net_thread.join();
}

///need to write ACKs for forwarding packets so we know to resend them if they didn't arrive
///I think the reason the data transfer is slow is because the framerate is bad
///if we threaded it i think the data transfer would go v high
///may be worth just threading the actual socket itself
void network_state::tick()
{
    if(!sock.valid())
        return;

    std::lock_guard<std::mutex> guard(threaded_network_mutex);

    //request_incomplete_packets(50);

    cleanup_available_data_and_incomplete_packets();

    make_packets_available();

    tick_cleanup();

    byte_vector v1;
    v1.push_back(canary_start);
    v1.push_back(message::KEEP_ALIVE);
    v1.push_back(canary_end);

    udp_send_to(sock, v1.ptr, (const sockaddr*)&store);

    bool any_recv = true;

    {
        byte_fetch fetch;
        fetch.ptr.swap(threaded_network_receive.ptr);
        threaded_network_receive = byte_fetch();


        //this_frame_stats.bytes_in += data.size();

        while(!fetch.finished() && any_recv)
        {
            int32_t found_canary = fetch.get<int32_t>();

            while(found_canary != canary_start && !fetch.finished())
            {
                found_canary = fetch.get<int32_t>();
            }

            int32_t type = fetch.get<int32_t>();

            if(type == message::PACKET_ACK)
            {
                int32_t len = fetch.get<int32_t>();

                packet_ack ack = fetch.get<packet_ack>();

                disconnection_timer[ack.owner_id].restart();

                fetch.get<decltype(canary_end)>();

                if(ack.packet_id > last_unconfirmed_packet[ack.owner_id])
                {
                    last_unconfirmed_packet[ack.owner_id] = ack.packet_id;
                }
            }

            if(type == message::REQUEST)
            {
                int32_t len = fetch.get<int32_t>();

                packet_request request = fetch.get<packet_request>();

                fetch.get<int32_t>();

                network_object no;
                no.owner_id = request.owner_id;
                no.serialise_id = request.serialise_id;

                disconnection_timer[no.owner_id].restart();

                if(owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id].find(request.sequence_id) !=
                        owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id].end())
                {
                    auto& vec = owner_to_packet_id_to_sequence_number_to_data[no.owner_id][request.packet_id][request.sequence_id];

                    //while(!sock_writable(sock)){}

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

                disconnection_timer[no.owner_id].restart();

                if(packet_fragments > 1)
                {
                    owner_to_packet_sequence_to_expected_size[no.owner_id][header.packet_id] = packet_fragments;

                    std::vector<packet_info>& packets = incomplete_packets[no.owner_id][header.packet_id];

                    ///INSERT PACKET INTO PACKETS
                    packet_info next;
                    next.sequence_number = header.sequence_number;

                    //if((header.sequence_number % 100) == 0)
                    if(header.sequence_number > 400 && (header.sequence_number % 1000) == 0)
                    {
                        std::cout << header.sequence_number << " ";
                        std::cout << " " << packets.size() << std::endl;
                        std::cout << packet_fragments << std::endl;
                    }

                    next.data.data = packet.fetch.ptr;

                    if(!has_packet_fragment[no.owner_id][header.packet_id][header.sequence_number])
                    {
                        packets.push_back(next);
                        has_packet_fragment[no.owner_id][header.packet_id][header.sequence_number] = true;
                    }

                    int current_received_fragments = packets.size();

                    if(current_received_fragments == packet_fragments)
                    {
                        std::sort(packets.begin(), packets.end(), [](packet_info& p1, packet_info& p2) {return p1.sequence_number < p2.sequence_number;});

                        serialise s;

                        for(packet_info& packet : packets)
                        {
                            s.data.insert(std::end(s.data), std::begin(packet.data.data), std::end(packet.data.data));

                            packet.data.data.clear();
                        }

                        ///pipe back a response?
                        if(s.data.size() > 0)
                        {
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
                    if(received_packet[no.owner_id][header.packet_id] == false)
                    {
                        forward_packet full_forward = packet;

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

byte_vector network_state::get_fragment(int id, const network_object& no, const std::vector<char>& data, packet_id_type use_packet_id)
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


///ok so the issue is that no.owner_id is wrong
///we're treating it like the client id, but its actually who owns the object
///which means that packet ids are getting trampled
///what we really need to do is make it the client's id, and then change
///change owns() to respect this. It should be a fairly transparent change
void network_state::forward_data(const network_object& no, serialise& s)
{
    if(!connected())
        return;

    int max_to_send = 20;

    int fragments = get_packet_fragments(s.data.size());

    bool should_slowdown = false;

    for(auto& i : last_unconfirmed_packet)
    {
        if(disconnected(i.first))
            continue;

        if(i.second < (int)packet_id - 2000)
            should_slowdown = true;
    }

    if(should_slowdown)
        max_to_send = 1;

    for(int i=0; i<get_packet_fragments(s.data.size()); i++)
    {
        byte_vector frag = get_fragment(i, no, s.data, packet_id);

        ///no.owner_id is.. always me here?
        owner_to_packet_id_to_sequence_number_to_data[no.owner_id][packet_id][i] = {frag};

        if(i < max_to_send)
        {
            //while(!sock_writable(sock)) {}

            udp_send_to(sock, frag.ptr, (const sockaddr*)&store);
        }
    }

    packet_id = packet_id + 1;
}

void network_state::tick_cleanup()
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

