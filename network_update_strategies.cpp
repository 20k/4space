#include "network_update_strategies.hpp"

#include "empire.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include "battle_manager.hpp"
#include "networking.hpp"

void orbital_update_strategy(system_manager& system_manage)
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

}

void network_updater::tick(float dt_s, network_state& net_state, empire_manager& empire_manage, system_manager& system_manage, fleet_manager& fleet_manage, all_battles_manager& all_battles)
{
    if(!net_state.connected())
        return;

    elapsed_time_s += dt_s;
}
