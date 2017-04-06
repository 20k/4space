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

int pick_random(int minx, int maxx)
{
    return randf_s(minx, maxx);
}

std::string fix_grammar(std::string str)
{

}

std::string generate_planet_description_text(std::string n1, std::string d1, std::string d2)
{
    int pick = pick_random(0, 2);

    std::string ret;

    if(pick == 0)
    {
        ret = n1 + " is a " + d1 + ", " + d2 + " world";
    }
    if(pick == 1)
    {
        ret = "The planet " + n1 + " is " + d1 + " and " + d2;
    }

    return ret;
}

///So, we can then use this sysetm to actually generate relevant information about a planet
///EG a large battle occured here scarring the terrain, life, homeworld
std::string procedural_text_generator::generate_planetary_text(orbital* o)
{
    std::vector<std::vector<std::string>> majority_element_to_description
    {
        {
            "Green", "Luscious", "Joyful", "Bountiful", "Temperate", "Rich", "Earth-like",
        },
        {
            "Acrid", "Acidic", "Hostile", "Sulphurous", "Rocky",
        },
        {
            "Gaseous", "Inflammable", "Flammable", "Hot",
        },
        {
            "Metallic", "Rocky", "Brown",
        },
        {
            "Metallic", "Gleaming", "White",
        },
        {
            "Radioactive", "Irradiated", "Rocky", "Dangerous", "Hazardous",
        },
    };

    std::vector<std::string> vec = majority_element_to_description[o->resource_type_for_flavour_text];

    std::random_shuffle(vec.begin(), vec.end());

    vec.resize(2);

    std::string name = o->name;

    std::string description = generate_planet_description_text(name, vec[0], vec[1]) + " " + std::to_string(o->resource_type_for_flavour_text);

    return description;
}
