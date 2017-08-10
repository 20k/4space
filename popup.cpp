#include "popup.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

int popup_element::gid;

void popup_info::clear()
{
    for(popup_element& pe : elements)
    {
        orbital* o = (orbital*)pe.element;

        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        sm->toggle_fleet_ui = false;
        sm->can_merge = false;

        for(ship* s : sm->ships)
        {
            s->shift_clicked = false;
        }

        o->command_queue.remove_any_of(object_command_queue_info::ANCHOR_UI);
    }

    //declaring_war = false;
    elements.clear();
}
