#include "network_update_strategies.hpp"

#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include "networking.hpp"

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
    }

    template<typename T>
    void enforce_cleanups_sent(const std::vector<T*>& to_manage, network_state& net_state, int send_mode, const std::vector<int>& sent)
    {
        for(int i=0; i<to_manage.size(); i++)
        {
            if((to_manage[i]->cleanup || to_manage[i]->dirty || to_manage[i]->force_send) && sent[i] == 0)
            {
                update(to_manage[i], net_state, send_mode);
            }
        }
    }

    template<typename T>
    void do_update_strategy(float dt_s, float time_between_full_updates, const std::vector<T*>& to_manage, network_state& net_state, int send_mode)
    {
        if(to_manage.size() == 0)
            return;

        std::vector<int> updated;
        updated.resize(to_manage.size());

        if(time_elapsed_s > time_between_full_updates)
        {
            for(int i=num_updated; i<to_manage.size(); i++)
            {
                T* t = to_manage[i];

                update(t, net_state, send_mode);

                updated[i] = 1;
            }

            time_elapsed_s = 0.;
            num_updated = 0;

            enforce_cleanups_sent(to_manage, net_state, send_mode, updated);

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

            updated[num_updated] = 1;
        }

        enforce_cleanups_sent(to_manage, net_state, send_mode, updated);

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

        if(sm->dirty)
            continue;

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

    ///have a super high frequency update for literally just absolute positions for orbitals
    static update_strategy orbital_strategy;
    orbital_strategy.do_update_strategy(dt_s, get_orbital_update_rate(orbital_info::FLEET), fleets, net_state, 0);

    ///doesn't matter what we use for the update rate below
    static update_strategy body_strategy;
    body_strategy.do_update_strategy(dt_s, get_orbital_update_rate(orbital_info::PLANET), bodies, net_state, 0);

    ///most important parameter here by far
    float ship_update_rate = 8;

    ///the reason why ship cleaning isn't working accross the network is beause its not being sent
    ///need to force packet sending on cleanup
    static update_strategy ship_strategy;
    ship_strategy.do_update_strategy(dt_s, ship_update_rate, ships, net_state, 0);

    static update_strategy ship_manager_strategy;
    ship_manager_strategy.do_update_strategy(dt_s, 10, ship_managers, net_state, 0);

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
    system_strategy.do_update_strategy(dt_s, 4.f, systems, net_state, 2);

    /*std::vector<projectile*> projectiles;

    for(battle_manager* bm : all_battles.battles)
    {
        for(projectile* p : bm->projectile_manage.projectiles)
        {
            projectiles.push_back(p);
        }
    }

    static update_strategy projectile_strategy;
    projectile_strategy.do_update_strategy(dt_s, 0.1f, projectiles, net_state, 0);*/

    std::vector<all_battles_manager*> all_battles_hack{&all_battles};

    std::vector<battle_manager*> sync_battles;

    for(battle_manager* bm : all_battles.battles)
    {
        //if(!net_state.owns(bm))
        //    continue;

        if(bm->dirty)
        {
            all_battles.make_dirty();
            continue;
        }

        sync_battles.push_back(bm);
    }

    /*std::vector<projectile*> projectiles;

    for(battle_manager* bm : all_battles.battles)
    {
        for(projectile* p : bm->projectile_manage.projectiles)
        {
            if(!net_state.owns(p))
                continue;

            if(p->dirty)
            {
                p->owned_by->make_dirty();
                //all_battles.make_dirty();
                continue;
            }

            projectiles.push_back(p);
        }
    }*/

    static update_strategy all_battle_strategy;
    all_battle_strategy.do_update_strategy(dt_s, 5.f, all_battles_hack, net_state, 0);

    static update_strategy battle_strategy;
    battle_strategy.do_update_strategy(dt_s, 1.f, sync_battles, net_state, 2);

    ///optional?
    //static update_strategy projectiles_strategy;
    //projectiles_strategy.do_update_strategy(dt_s, 0.2, projectiles, net_state, 0);


    std::vector<empire*> empires;

    std::vector<empire*> needs_attention;

    ///REMEMBER THIS WONT WORK IF WE SPAWN A NEW EMPIRE AT RUNTIME OK? OK
    ///change empire manager to be the same hack as all battles
    ///maybe codify this hack functionally
    for(empire* e : empire_manage.empires)
    {
        if(!net_state.owns(e) && !e->net_claim)
            continue;

        empires.push_back(e);
    }

    for(empire* e : empire_manage.empires)
    {
        if(e->force_send)
        {
            needs_attention.push_back(e);
        }
    }

    //std::cout << fleets.size() << std::endl;
    //if(empires.size() > 0 && randf_s(0.f, 1.f) < 0.1f)
    //    std::cout << empires.size() << std::endl;

    if(needs_attention.size() > 0)
    {
        //std::cout << needs_attention.size() << std::endl;
    }

    static update_strategy empire_strategy;
    empire_strategy.do_update_strategy(dt_s, 2.f, empires, net_state, 0);

    static update_strategy empire_attention_strategy;
    empire_attention_strategy.do_update_strategy(dt_s, 2.f, needs_attention, net_state, 3);

    elapsed_time_s += dt_s;
}
