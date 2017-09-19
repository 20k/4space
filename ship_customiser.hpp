#ifndef SHIP_CUSTOMISER_HPP_INCLUDED
#define SHIP_CUSTOMISER_HPP_INCLUDED

#include "ship.hpp"
#include "ui_util.hpp"
#include "util.hpp"
#include <map>
#include "ship_definitions.hpp"

extern std::map<int, bool> component_open;

struct ship_customiser
{
    bool text_input_going = false;

    std::vector<ship> saved;

    ship current;
    int64_t last_selected = -1;

    ship_customiser();

    void tick(float scrollwheel, bool lclick, vec2f mouse_change);

private:
    //ImVec2 last_stats_dim = ImVec2(0, 0);
    void save();
    void do_save_window();
    int64_t renaming_id = -1;
    std::string ship_name_buffer;
};

#endif // SHIP_CUSTOMISER_HPP_INCLUDED
