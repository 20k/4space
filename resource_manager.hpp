#ifndef RESOURCE_MANAGER_HPP_INCLUDED
#define RESOURCE_MANAGER_HPP_INCLUDED

#include <vector>
#include <string>
#include <map>
#include "serialise.hpp"

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

        //OXYGEN,
        HYDROGEN,
        IRON,
        COPPER,
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

        //"Oxygen",
        "Hydrogen",
        "Iron",
        "Copper",
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

        //"O2",
        "H2",
        "Fe",
        "Cu",
        "Ti",
        "U",
        "Re",
        "WTF",
    };

    static std::vector<vec3f> colours
    {
        {0xff / 255.f, 0x6f / 255.f, 0}, ///h2, orange
        {0xc2 / 255.f, 0x19 / 255.f, 0x1c / 255.f}, ///iron, red
        {0.4f, 0.4f, 1.f}, ///copper, blue
        {0xbd / 255.f, 0x96 / 255.f, 0xa6 / 255.f}, ///titanium, silver
        {0x2d / 255.f, 0xc9 / 255.f, 0x06 / 255.f}, ///uranium, green
        {1.f, 1.f, 1.f}, ///research, white
        {1.f, 0.f, 1.f}, ///err
    };

    ///h2 -> ff6f00
    ///fe -> c2191c
    ///cu -> 0404c4
    ///ti -> bd96a6
    ///U -> 2dc906
    ///RE -> white

    //types get_random_unprocessed();
    types get_random_processed();

    //static types unprocessed_end = OXYGEN;

    bool is_processed(types type);

    static float global_resource_multiplier = 0.2f;

    inline vec3f to_col(types type)
    {
        return mix({1.f, 1.f, 1.f}, colours[type], 0.65f);
    }

    inline vec3f to_col(int type)
    {
        return to_col((types)type);
    }
}

struct resource_element
{
    resource::types type = resource::COUNT;
    double amount = 0.f;
};

namespace sf
{
    struct RenderWindow;
}

struct resource_manager : serialisable
{
    resource_manager();

    std::vector<resource_element> resources;

    void add(const std::map<resource::types, float>& val);

    resource_element& get_resource(resource::types type);

    float get_weighted_rarity();

    void draw_ui(sf::RenderWindow& win, resource_manager& produced_ps);

    void render_formatted_str(bool can_skip);
    void render_tooltip(bool can_skip);

    //std::string get_unprocessed_str();
    std::string get_processed_str(bool can_skip);

    std::string get_formatted_str(bool can_skip = true);


    bool has_any_processed();

    void do_serialise(serialise& s, bool ser) override;
};

#endif // RESOURCE_MANAGER_HPP_INCLUDED
