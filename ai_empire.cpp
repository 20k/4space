#include "ai_empire.hpp"
#include "empire.hpp"

struct orbital_system_descriptor
{
    orbital_system* os = nullptr;

    //float weighted_resource_importance = 0.f;

    ///how valuable are the resources in this system to us
    bool is_owned_by_me = false;
    bool is_owned = false;
    int resource_ranking = 9999999;
    int distance_ranking = 9999999;

    int empire_threat_ranking = 9999999;
    float empire_threat_value = 0.f;
};

/*std::vector<orbital_system_descriptor> process_orbitals(empire* e)
{
    //const auto& systems =
}*/

void ensure_adequate_defence(ai_empire& ai, empire* e)
{
    float ship_fraction_in_defence = 0.6f;

    ///how do we pathfind ships
    ///ship command queue? :(
    ///Hooray! All neceessary work is done to implement empire behaviours! :)


}



void ai_empire::tick(system_manager& sm, empire* e)
{
    ensure_adequate_defence(*this, e);
}
