#ifndef CLICKABLE_HPP_INCLUDED
#define CLICKABLE_HPP_INCLUDED

#include <vec/vec.hpp>

struct orbital;

struct clickable
{
    vec2f pos;
    float rad = 0.f;

    orbital* o;

    clickable(vec2f ppos, float prad, orbital* po)
    {
        pos = ppos;
        rad = prad;
        o = po;
    }

    clickable()
    {

    }

    void make_from_local(orbital* po);
    void make_from_universe(orbital* po);
};

#endif // CLICKABLE_HPP_INCLUDED
