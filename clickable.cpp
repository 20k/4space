#include "clickable.hpp"
#include "system_manager.hpp"

void clickable::make_from_local(orbital* po)
{
    o = po;

    pos = o->last_viewed_position;
    rad = o->rad;
}

void clickable::make_from_universe(orbital* po)
{
    o = po;

    pos = o->universe_view_pos;
    rad = o->rad;
}
