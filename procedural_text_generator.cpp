#include "procedural_text_generator.hpp"
#include "system_manager.hpp"
#include <fstream>

std::vector<std::string> procedural_text::parse_planet_names()
{
    std::vector<std::string> ret;

    std::ifstream in("data/minor_planets.txt");

    std::string str;

    while(std::getline(in, str))
    {
        while(str.size() > 0 && str[0] == ' ')
        {
            str.erase(str.begin());
        }

        auto pos = str.find(' ');

        str = str.erase(0, pos+1);

        ret.push_back(str);
    }

    return ret;
}

std::string procedural_text_generator::generate_planetary_name()
{
    return procedural_text::planet_names[(int)randf_s(0.f, procedural_text::planet_names.size())];
}

std::string procedural_text_generator::generate_planetary_text(orbital* o)
{

}
