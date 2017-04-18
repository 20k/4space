#include "procedural_text_generator.hpp"
#include "system_manager.hpp"
#include <fstream>
#include "util.hpp"
#include "empire.hpp"

//uint16_t procedural_text::fid;

std::vector<std::string> procedural_text::parse_default(const std::string& file)
{
    std::vector<std::string> ret;

    std::ifstream in(file);

    std::string str;

    while(std::getline(in, str))
    {
        ret.push_back(str);
    }

    return ret;
}

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

std::vector<std::string> procedural_text::parse_star_names()
{
    std::vector<std::string> ret;

    std::ifstream in("data/stars.txt");

    std::string str;

    while(std::getline(in, str))
    {
        ret.push_back(str);
    }

    return ret;
}

std::string procedural_text::select_random(const std::vector<std::string>& dataset)
{
    return dataset[(int)randf_s(0.f, dataset.size())];
}

std::string procedural_text_generator::generate_planetary_name()
{
    return procedural_text::planet_names[(int)randf_s(0.f, procedural_text::planet_names.size())];
}

std::string procedural_text_generator::generate_star_name()
{
    return procedural_text::star_names[(int)randf_s(0.f, procedural_text::star_names.size())];
}

int pick_random(int minx, int maxx)
{
    return randf_s(minx, maxx);
}

bool is_vowel(uint8_t c)
{
    c = tolower(c);

    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

std::string fix_grammar(std::string str)
{
    for(int sid = 1; sid < str.length()-3; sid++)
    {
        if(str.substr(sid, 3) == " a ")
        {
            uint8_t next = str[sid + 3];

            if(!is_vowel(next))
                continue;

            str.insert(str.begin() + 2 + sid, 'n');
        }
    }

    return str;
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
        {
            "Curious", "Teeming", "Alien", "intriguing", "Strange",
        },
    };

    std::vector<std::string> vec = majority_element_to_description[o->resource_type_for_flavour_text];

    std::random_shuffle(vec.begin(), vec.end());

    vec.resize(2);

    std::string name = o->name;

    std::string description = generate_planet_description_text(name, vec[0], vec[1]);// + " " + std::to_string(o->resource_type_for_flavour_text);

    return fix_grammar(description);
}

std::string procedural_text_generator::generate_star_text(orbital* o)
{
    float mint = 2000.f;
    float maxt = 40000.f;

    float temperature = randf_s(0.f, 1.f) * randf_s(0.f, 1.f) * (maxt - mint) + mint;

    std::vector<int> lower_bounds = {0, 3500, 5000, 6000, 7500, 10000, 30000};

    int bound = 0;

    for(int i=0; i<lower_bounds.size(); i++)
    {
        bool b1 = temperature >= lower_bounds[i];
        bool b2 = true;

        if(i < lower_bounds.size()-1)
        {
            b2 = temperature < lower_bounds[i+1];
        }

        if(b1 && b2)
        {
            bound = i;
            break;
        }
    }


    std::vector<std::string> spectral_class = {"M", "K", "G", "F", "A", "B", "O"};

    std::string ret;

    ret = "Spectral Class: " + spectral_class[bound] + "\n" +
           "Temperature: " + to_string_with_precision(round(temperature / 100)*100, 6) + "K";

    return ret;
}

std::string procedural_text_generator::generate_ship_name()
{
    ///make the ss bit also random
    return "SS " + procedural_text::select_random(procedural_text::ship_names);
}

std::string procedural_text_generator::generate_fleet_name(orbital* o)
{
    if(o->parent_empire == nullptr)
        throw;

    int fname = o->parent_empire->fleet_name_counter++;

    return std::string("Fleet ") + std::to_string(fname);
}

std::string procedural_text_generator::generate_empire_name()
{
    std::string b1 = procedural_text::select_random(procedural_text::empires_1);
    std::string b2 = procedural_text::select_random(procedural_text::empires_2);

    std::string the = randf_s(0.f, 1.f) < 0.5 ? "" : "The ";

    return the + b1 + " " + b2;
}
