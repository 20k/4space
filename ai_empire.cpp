#include "ai_empire.hpp"
#include "empire.hpp"

void ensure_adequate_defence(ai_empire& ai, empire* e)
{
    float ship_fraction_in_defence = 0.6f;

    ///how do we pathfind ships
    ///ship command queue? :(
    ///Hooray! All neceessary work is done to implement empire behaviours! :)
}

void ai_empire::tick(empire* e)
{
    ensure_adequate_defence(*this, e);
}
