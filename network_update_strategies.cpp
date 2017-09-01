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

struct update_strategy
{
    int num_updated = 0;

    double time_elapsed_s = 0;

    template<typename T>
    void update(T* t, network_state& net_state)
    {
        serialise_data_helper::disk_mode = 0;

        serialise ser;
        ///because o is a pointer, we allow the stream to force decode the pointer
        ///if o were a non ptr with a do_serialise method, this would have to be false
        ser.allow_force = true;
        ser.default_owner = net_state.my_id;

        ser.handle_serialise(serialise_data_helper::disk_mode, true);
        ser.handle_serialise(t, true);

        network_object no;

        if(t->owner_id == -1)
            no.owner_id = net_state.my_id;
        else
            no.owner_id = t->owner_id;

        no.serialise_id = t->serialise_id;

        net_state.forward_data(no, ser);
    }

    template<typename T>
    void do_update_strategy(float dt_s, float time_between_full_updates, const std::vector<T*>& to_manage, network_state& net_state)
    {
        if(to_manage.size() == 0)
            return;

        if(time_elapsed_s > time_between_full_updates)
        {
            for(int i=num_updated; i<to_manage.size(); i++)
            {
                T* t = to_manage[i];

                update(t, net_state);
            }

            time_elapsed_s = 0.;
            num_updated = 0;

            return;
        }

        float frac = time_elapsed_s / time_between_full_updates;

        frac = clamp(frac, 0.f, 1.f);

        int num_to_update_to = frac * (float)to_manage.size();

        for(; num_updated < num_to_update_to; num_updated++)
        {
            T* t = to_manage[num_updated];

            update(t, net_state);
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

    std::vector<orbital*> orbitals;
    std::vector<orbital*> bodies;

    for(orbital_system* sys : system_manage.systems)
    {
        for(orbital* o : sys->orbitals)
        {
            if(o->owner_id != 1 && o->owner_id != net_state.my_id)
            {
                continue;
            }

            if(o->type == orbital_info::FLEET)
            {
                orbitals.push_back(o);
                continue;
            }
            else
            {
                bodies.push_back(o);
                continue;
            }
        }
    }

    static update_strategy orbital_strategy;
    orbital_strategy.do_update_strategy(dt_s, 0.5f, orbitals, net_state);

    static update_strategy body_strategy;
    body_strategy.do_update_strategy(dt_s, 5.f, bodies, net_state);

    elapsed_time_s += dt_s;
}
