#ifndef NETWORKING_HPP_INCLUDED
#define NETWORKING_HPP_INCLUDED

#include <net/shared.hpp>
#include "../4space_server/master_server/network_messages.hpp"
#include "serialise.hpp"
#include <thread>
#include <mutex>

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

using packet_id_type = int32_t;
using sequence_data_type = int32_t;

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

    bool connected();

    float timeout_max = 5.f;
    float timeout = timeout_max;

    std::vector<network_data> available_data;
    std::map<serialise_owner_type, std::map<packet_id_type, std::vector<packet_info>>> incomplete_packets;
    std::map<serialise_owner_type, std::map<packet_id_type, int>> owner_to_packet_sequence_to_expected_size;
    std::map<serialise_owner_type, std::map<packet_id_type, serialise_data_type>> owner_to_packet_id_to_serialise;

    serialisable* get_serialisable(serialise_host_type& host_id, serialise_data_type& serialise_id);

    void tick_join_game(float dt_s);
    void leave_game();

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

    forward_packet decode_forward(byte_fetch& fetch);

    #pragma pack(1)
    struct packet_request
    {
        serialise_owner_type owner_id = 0;
        sequence_data_type sequence_id = 0;
        packet_id_type packet_id = 0;
        serialise_data_type serialise_id;
    };

    struct packet_ack
    {
        serialise_owner_type owner_id = 0;
        sequence_data_type sequence_id = 0;
        packet_id_type packet_id = 0;
    };

    bool owns(serialisable* s);

    void claim_for(serialisable* s, serialise_host_type new_host);

    void claim_for(serialisable& s, serialise_host_type new_host);

    void make_packet_request(packet_request& request);

    void make_packet_ack(packet_ack& ack);

    struct wait_info
    {
        bool first = true;
        sf::Clock clk;

        bool too_long()
        {
            return (clk.getElapsedTime().asMilliseconds() > 100) || first;
        }

        void request()
        {
            first = false;
            clk.restart();
        }
    };

    std::map<serialise_owner_type, std::map<packet_id_type, std::map<sequence_data_type, wait_info>>> owner_to_request_timeouts;

    std::map<serialise_owner_type, std::map<packet_id_type, sequence_data_type>> current_packet_fragment;

    std::minstd_rand random_gen;

    void request_incomplete_packets(int max_fragments_to_request);

    ///the only reason to defer this is in case we receive duplicates
    void cleanup_available_data_and_incomplete_packets();

    std::map<serialise_owner_type, std::deque<forward_packet>> forward_ordered_packets;

    void make_available(const serialise_owner_type& owner, int id);

    std::map<serialise_owner_type, std::map<packet_id_type, std::map<sequence_data_type, bool>>> has_packet_fragment;

    std::map<serialise_owner_type, std::map<packet_id_type, wait_info>> packet_wait_map;
    ///there's a few structures here and there to do with packets that actually just pile up infinitely
    ///needs to be a general "after 10 seconds or so clean these up as those packets aren't comin"
    std::map<serialise_owner_type, std::map<packet_id_type, bool>> received_packet;
    std::map<serialise_owner_type, packet_id_type> last_received_packet;

    //std::map<serialise_owner_type, std::map<packet_id_type, std::map<sequence_data_type>>> last_confirmed_packets;

    std::map<serialise_owner_type, packet_id_type> last_unconfirmed_packet;

    void make_packets_available();

    std::map<serialise_owner_type, sf::Clock> disconnection_timer;

    bool disconnected(const serialise_owner_type& type);

    std::vector<byte_vector> to_send_to_game_server;
    byte_fetch threaded_network_receive;
    std::mutex threaded_network_mutex;
    volatile int should_die = 0;

    static
    void thread_network_receive(network_state* net_state);

    bool network_thread_launched = false;

    std::thread net_thread;

    network_state();

    ~network_state();

    ///need to write ACKs for forwarding packets so we know to resend them if they didn't arrive
    ///I think the reason the data transfer is slow is because the framerate is bad
    ///if we threaded it i think the data transfer would go v high
    ///may be worth just threading the actual socket itself
    void tick();

    byte_vector get_fragment(int id, const network_object& no, const std::vector<char>& data, packet_id_type use_packet_id);

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
    void forward_data(const network_object& no, serialise& s);

    void tick_cleanup();
};


#endif // NETWORKING_HPP_INCLUDED
