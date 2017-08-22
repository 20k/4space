#include "popup.hpp"
#include "system_manager.hpp"
#include "ship.hpp"

int popup_element::gid;

void popup_info::cleanup(void* element)
{
    orbital* o = (orbital*)element;

    if(o->type != orbital_info::FLEET)
        return;

    o->command_queue.remove_any_of(object_command_queue_info::ANCHOR_UI);
    o->command_queue.remove_any_of(object_command_queue_info::COLONISE_UI);

    ship_manager* sm = (ship_manager*)o->data;

    sm->toggle_fleet_ui = false;
    sm->can_merge = false;

    for(ship* s : sm->ships)
    {
        s->shift_clicked = false;
    }
}

void popup_info::clear()
{
    for(popup_element& pe : elements)
    {
        cleanup(pe.element);
    }

    //declaring_war = false;
    elements.clear();
}
