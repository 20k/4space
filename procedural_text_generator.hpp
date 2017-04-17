#ifndef PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
#define PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED

#include <string>
#include <vector>

struct orbital;

namespace procedural_text
{
    std::vector<std::string> parse_default(const std::string& file);

    std::vector<std::string> parse_planet_names();
    std::vector<std::string> parse_star_names();

    static std::vector<std::string> planet_names = parse_planet_names();
    static std::vector<std::string> star_names = parse_star_names();

    static std::vector<std::string> ship_names = parse_default("./data/ships.txt");
    static std::vector<std::string> empires_1 = parse_default("./data/empires_1.txt");
    static std::vector<std::string> empires_2 = parse_default("./data/empires_2.txt");
}

struct procedural_text_generator
{
    std::string generate_planetary_name();
    std::string generate_planetary_text(orbital* o);

    std::string generate_star_name();
    std::string generate_star_text(orbital* o);

    ///ship names, and empire names
    std::string generate_ship_name();
    std::string generate_fleet_name(orbital* o);

    std::string generate_empire_name();
};

#endif // PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
