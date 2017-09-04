#include "network_update_strategies.hpp"

#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include "networking.hpp"

/*void orbital_update_strategy(float time_between, system_manager& system_manage)
{

}

void fleet_update_strategy(fleet_manager& fleet_manage)
{

}

void battle_update_strategy(all_battles_manager& all_battles)
{

}

void empire_update_strategy(empire_manager& empire_manage)
{

}*/

///for updating orbitals and ships
///have a separate one for updating.. basically managers and higher level info
///eg for a system, we need to pipe the vector which contains references to orbitals
/*void update_strategy(float current_time, float time_between_updates, int my_id, int max_ids, serialisable* object)
{

}*/

/*void send_data(network_object& no, serialise& ser, network_state& net_state)
{
    serialise_data_helper::send_mode = 0;
    serialise_data_helper::ref_mode = 0;

    net_state.forward_data(no, ser);
}*/

/*void send_data(serialisable* t, serialise& ser, network_state& net_state, int send_mode)
{

}*/

struct update_strategy
{
    int num_updated = 0;

    double time_elapsed_s = 0;

    template<typename T>
    void update(T* t, network_state& net_state, int send_mode)
    {
        serialise_data_helper::send_mode = send_mode;
        serialise_data_helper::ref_mode = 0;

        serialise ser;
        ///because o is a pointer, we allow the stream to force decode the pointer
        ///if o were a non ptr with a do_serialise method, this would have to be false
        //ser.allow_force = true;

        ser.default_owner = net_state.my_id;

        serialise_host_type host_id = net_state.my_id;

        if(t->host_id != -1)
        {
            host_id = t->host_id;
        }

        ser.handle_serialise(serialise_data_helper::send_mode, true);
        ser.handle_serialise(host_id, true);
        ser.force_serialise(t, true);

        network_object no;

        no.owner_id = net_state.my_id;
        no.serialise_id = t->serialise_id;

        net_state.forward_data(no, ser);


        //std::cout << t->serialise_id << std::endl;
    }

    template<typename T>
    void do_update_strategy(float dt_s, float time_between_full_updates, const std::vector<T*>& to_manage, network_state& net_state, int send_mode)
    {
        if(to_manage.size() == 0)
            return;

        if(time_elapsed_s > time_between_full_updates)
        {
            for(int i=num_updated; i<to_manage.size(); i++)
            {
                T* t = to_manage[i];

                update(t, net_state, send_mode);
            }

            time_elapsed_s = 0.;
            num_updated = 0;

            return;
        }

        float frac = time_elapsed_s / time_between_full_updates;

        frac = clamp(frac, 0.f, 1.f);

        int num_to_update_to = frac * (float)to_manage.size();

        num_to_update_to = clamp(num_to_update_to, 0, (int)to_manage.size());

        for(; num_updated < num_to_update_to; num_updated++)
        {
            T* t = to_manage[num_updated];

            update(t, net_state, send_mode);
        }

        time_elapsed_s += dt_s;
    }
};

void network_updater::tick(float dt_s, network_state& net_state, empire_manager& empire_manage, system_manager& system_manage, fleet_manager& fleet_manage, all_battles_manager& all_battles)
{
    if(!net_state.connected())
        return;

    //float time_between_empire_updates

    int num_orbitals = 0;

    std::vector<orbital*> fleets;
    std::vector<orbital*> bodies;

    std::vector<orbital_system*> systems;

    std::vector<ship_manager*> ship_managers;
    std::vector<ship*> ships;

    ///the reason why fleet merging doesn't work fully is because there's no longer an orbital
    ///which exists as it gets cleaned up on player 1
    for(ship_manager* sm : fleet_manage.fleets)
    {
        if(!net_state.owns(sm))
            continue;

        //if(sm->dirty)
        //    continue;

        ship_managers.push_back(sm);

        for(ship* s : sm->ships)
        {
            if(s->dirty)
                continue;

            ships.push_back(s);
        }
    }

    for(orbital_system* sys : system_manage.systems)
    {
        ///in mode 2 we cooperatively share memory!
        //if(net_state.owns(sys))
        {
            systems.push_back(sys);
        }

        for(orbital* o : sys->orbitals)
        {
            /*if(o->owner_id != 1 && o->owner_id != net_state.my_id)
            {
                continue;
            }*/

            //std::cout << o->owner_id << std::endl;

            if(!net_state.owns(o))
                continue;

            if(o->type == orbital_info::FLEET && o->data->dirty)
            {
                o->dirty = true;
            }

            if(o->dirty)
                continue;

            if(o->type == orbital_info::FLEET)
            {
                fleets.push_back(o);
                continue;
            }
            else
            {
                bodies.push_back(o);
                continue;
            }
        }
    }

    //if(fleets.size() > 0)
    //    std::cout << fleets.size() << std::endl;

    static update_strategy orbital_strategy;
    orbital_strategy.do_update_strategy(dt_s, get_orbital_update_rate(orbital_info::FLEET), fleets, net_state, 0);

    ///doesn't matter what we use for the update rate below
    static update_strategy body_strategy;
    body_strategy.do_update_strategy(dt_s, get_orbital_update_rate(orbital_info::PLANET), bodies, net_state, 0);

    static update_strategy ship_strategy;
    ship_strategy.do_update_strategy(dt_s, 1.f, ships, net_state, 0);

    static update_strategy ship_manager_strategy;
    ship_manager_strategy.do_update_strategy(dt_s, 1, fleets, net_state, 0);

    ///we're getting a null unformed orbital on the other client
    ///investigate
    ///the problem was that the orbital strategy would pipe across an orbital
    ///but there is some conflict with the dirty send strategy

    ///OK. So we aren't serialising it because it doesn't exist in the owner to id to pointer table
    ///where it doesn't exist because its never been encountered before. This means its just ditched
    ///but then why are we crashing on do update strategy for systems?

    ///Ok. Same problem still happening just less likely across frames, problem is packet reordering I think

    ///OH OK. What happens is that something else processes the dirty, which means that when we send system
    ///we don't send with dirty
    ///that means that when the system packet arrives, we get a bad orbital and boom it are crashing

    ///No. Ok something is actually broken, still crashing on system strategy sending and only system strategy
    ///still getting orphaned orbitals
    ///still suspect packet reordering
    ///implement packet ordering, need it anyway


    ///so. I think the problem is that we're giving the system references to orbitals that may get created
    ///but aren't fully initialised, hence the crash
    ///mode 2 is diff mode, where we send diff updates
    ///change in container size (num dirty), maybe diff hp as well
    static update_strategy system_strategy;
    system_strategy.do_update_strategy(dt_s, 0.5f, systems, net_state, 2);

    //std::cout << fleets.size() << std::endl;

    ///Hmm. This isn't the best plan
    ///the problem is, nobody owns these
    ///sort this out through dirty and make_new
    /*network_object no;
    ///CONFLICT BETWEEN PLAYERS
    no.owner_id = net_state.my_id;
    no.serialise_id = -1;

    serialise ser;

    ser.handle_serialise<decltype(serialise_data_helper::disk_mode)>(2, true);
    ser.force_serialise(empire_manage, true);
    ser.force_serialise(system_manage, true);
    ser.force_serialise(fleet_manage, true);
    ser.force_serialise(all_battles, true);

    send_data(t, ser, net_state);*/

    elapsed_time_s += dt_s;
}
