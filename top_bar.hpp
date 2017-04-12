#ifndef TOP_BAR_HPP_INCLUDED
#define TOP_BAR_HPP_INCLUDED

#include <string>
#include <map>
#include <vector>

#define IMGUI_WINDOW_FLAGS ImGuiWindowFlags_NoFocusOnAppearing

namespace top_bar_info
{
    enum types
    {
        ECONOMY,
        RESEARCH,
        DIPLOMACY,
        FLEETS,
        BATTLES,
        SYSTEMS,
        UNIVERSE,
        NONE
    };

    static std::vector<std::string> names
    {
        "Economy",
        "Research",
        "Diplomacy",
        "Fleets",
        "Battles",
        "Systems",
        "Universe",
    };
}

struct top_bar
{
    static std::vector<std::string> headers;
    static std::map<top_bar_info::types, bool> active;

    //void set_str(top_bar_info::types type, std::string str);
    static bool get_active(top_bar_info::types type);

    static void display();
};

#endif // TOP_BAR_HPP_INCLUDED
