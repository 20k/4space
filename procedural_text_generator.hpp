#ifndef PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
#define PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED

#include <string>
#include <vector>

struct orbital;

namespace procedural_text
{
    std::vector<std::string> parse_planet_names();
    std::vector<std::string> parse_star_names();

    static std::vector<std::string> planet_names = parse_planet_names();

    static std::vector<std::string> star_names = parse_star_names();
}

struct procedural_text_generator
{
    std::string generate_planetary_name();

    std::string generate_planetary_text(orbital* o);

    std::string generate_star_name();

    std::string generate_star_text(orbital* o);
};

#endif // PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
