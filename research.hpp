#ifndef RESEARCH_HPP_INCLUDED
#define RESEARCH_HPP_INCLUDED

#include <vector>
#include <string>

struct empire;

namespace research_info
{
    enum types
    {
        WEAPONS,
        MATERIALS,
        SCANNERS,
        PROPULSION,
        COUNT
    };

    static std::vector<std::string> names
    {
        "Weapons",
        "Materials",
        "Scanners",
        "Propulsion",
    };

    ///ok intuitive research level costs:
    ///first level: 50
    ///second level: 200
    ///third level: 400
    ///fourth level: 1000
    ///fifth level: 5000
    ///sixth level: 20000

    ///take two:
    ///first level: 50
    ///second level 200
    ///third level: 800
    ///fourth level: 3200
    ///fifth level: 12800
    ///so simply 50 * 4 * research_level + researches below, where 4 is scaling

    float get_research_level_cost(int level);
}

struct research_category
{
    research_info::types type = research_info::COUNT;
    float amount = 0;
};

struct research
{
    std::vector<research_category> categories;

    research();

    void draw_ui(empire* emp);
    void tick(float step_s);
};

#endif // RESEARCH_HPP_INCLUDED
