#ifndef EMPIRE_HPP_INCLUDED
#define EMPIRE_HPP_INCLUDED

#include "resource_manager.hpp"

struct orbital;
struct orbital_system;
struct ship_manager;

struct empire
{
    std::string name;

    resource_manager resources;

    std::vector<orbital*> owned;
    std::vector<ship_manager*> owned_fleets;

    void take_ownership(orbital* o);
    void release_ownership(orbital* o);

    void take_ownership(ship_manager* s);
    void release_ownership(ship_manager* s);

    ///for debugging
    void take_ownership_of_all(orbital_system* o);

    bool owns(orbital* o);

    void generate_resource_from_owned(float step_s);
};

#endif // EMPIRE_HPP_INCLUDED
