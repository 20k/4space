#ifndef PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
#define PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED

#include <string>

struct orbital;

struct procedural_text_generator
{
    std::string generate_planetary_text(orbital* o);
};

#endif // PROCEDURAL_TEXT_GENERATOR_HPP_INCLUDED
