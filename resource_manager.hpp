#ifndef RESOURCE_MANAGER_HPP_INCLUDED
#define RESOURCE_MANAGER_HPP_INCLUDED

#include <vector>
#include <string>

namespace resource
{
    enum types
    {
        ICE,
        COPPER_ORE,
        NITRATES,
        IRON_ORE,
        TITANIUM_ORE,
        URANIUM_ORE,

        OXYGEN,
        COPPER,
        HYDROGEN,
        IRON,
        TITANIUM,
        URANIUM,
        COUNT
    };

    static std::vector<std::string> names =
    {
        "Ice",
        "Copper Sulfide",
        "Nitrates", ///? Maybe good for coolant? Farming and civics?
        "Meteoric Iron",
        "Titanium Dioxide", ///has oxygen
        "Uraninite", ///has oxygen

        "Oxygen",
        "Copper",
        "Hydrogen",
        "Iron",
        "Titanium",
        "Uranium",
        "Error",
    };

    static std::vector<std::string> short_names =
    {
        "Ice",
        "CuS",
        "NO2",
        "FE,NI",
        "TiO2",
        "UO2",

        "O2",
        "Cu",
        "H2",
        "Fe",
        "Ti",
        "U",
        "WTF",
    };

    static types unprocessed_end = OXYGEN;

    bool is_processed(types type);
}

struct resource_element
{
    resource::types type = resource::COUNT;
    float amount = 0.f;
};

namespace sf
{
    struct RenderWindow;
}

struct resource_manager
{
    resource_manager();

    std::vector<resource_element> resources;

    resource_element& get_resource(resource::types type);

    void draw_ui(sf::RenderWindow& win);
};

#endif // RESOURCE_MANAGER_HPP_INCLUDED
