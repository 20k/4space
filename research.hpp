#ifndef RESEARCH_HPP_INCLUDED
#define RESEARCH_HPP_INCLUDED

#include <vector>
#include <string>

struct empire;

///more research types? Stealth?
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

    ///base cost of research expensiveness is 50
    #define TESTING
    #ifdef TESTING
    float get_research_level_cost(int level, float cost = 50.f, bool testing = true);
    #else
    float get_research_level_cost(int level, float cost = 50.f, bool testing = false);
    #endif // TESTING

    float get_cost_scaling(float level, float cost);

    float tech_unit_to_research_currency(float tech_unit);
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

    void add_amount(const research_category& category);
};

#endif // RESEARCH_HPP_INCLUDED
