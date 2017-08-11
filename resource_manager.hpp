#ifndef RESOURCE_MANAGER_HPP_INCLUDED
#define RESOURCE_MANAGER_HPP_INCLUDED

#include <vector>
#include <string>
#include <map>

namespace resource
{
    enum types
    {
        /*ICE,
        COPPER_ORE,
        NITRATES,
        IRON_ORE,
        TITANIUM_ORE,
        URANIUM_ORE,*/

        OXYGEN,
        COPPER,
        HYDROGEN,
        IRON,
        TITANIUM,
        URANIUM,
        RESEARCH,
        COUNT
    };

    static std::vector<std::string> names =
    {
        /*"Ice",
        "Copper Sulfide",
        "Nitrates", ///? Maybe good for coolant? Farming and civics?
        "Meteoric Iron",
        "Titanium Dioxide", ///has oxygen
        "Uraninite", ///has oxygen*/

        "Oxygen",
        "Copper",
        "Hydrogen",
        "Iron",
        "Titanium",
        "Uranium",
        "Research",
        "Error",
    };

    static std::vector<std::string> short_names =
    {
        /*"H20(s)",
        "CuS",
        "NO2",
        "FE,NI",
        "TiO2",
        "UO2",*/

        "O2",
        "Cu",
        "H2",
        "Fe",
        "Ti",
        "U",
        "Re",
        "WTF",
    };

    //types get_random_unprocessed();
    types get_random_processed();

    static types unprocessed_end = OXYGEN;

    bool is_processed(types type);

    static float global_resource_multiplier = 0.2f;
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

struct render_window;

struct resource_manager
{
    resource_manager();

    std::vector<resource_element> resources;

    void add(const std::map<resource::types, float>& val);

    resource_element& get_resource(resource::types type);

    void draw_ui(render_window& win);

    std::string get_unprocessed_str();
    std::string get_processed_str(bool can_skip);

    std::string get_formatted_str(bool can_skip = true);

    bool has_any_processed();
};

#endif // RESOURCE_MANAGER_HPP_INCLUDED
