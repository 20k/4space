#ifndef NETWORK_UPDATE_STRATEGIES_HPP_INCLUDED
#define NETWORK_UPDATE_STRATEGIES_HPP_INCLUDED

struct empire_manager;
struct system_manager;
struct fleet_manager;
struct all_battles_manager;
struct network_state;

struct network_updater
{
    double elapsed_time_s = 0;

    void tick(float dt_s, network_state& net_state, empire_manager& empire_manage, system_manager& system_manage, fleet_manager& fleet_manage, all_battles_manager& all_battles);

private:
    int last_num_orbitals = 0;
};

#endif // NETWORK_UPDATE_STRATEGIES_HPP_INCLUDED
