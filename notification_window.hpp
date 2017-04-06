#ifndef NOTIFICATION_WINDOW_HPP_INCLUDED
#define NOTIFICATION_WINDOW_HPP_INCLUDED

#include <string>
#include <vector>

namespace sf
{
    struct RenderWindow;
}

struct text_element
{
    int type = 0; ///useful for other systems identifying this
    std::string text;
    bool pressed = false;
    bool toggle_state = false;
};

struct notification_window
{
    std::string title;

    std::vector<text_element> elements;

    void tick_draw(sf::RenderWindow& win);
};

///I'm not sure this is necessary, each system can own its notification window
struct notification_manager
{
    std::vector<notification_window*> windows;

    notification_window* make_new();
    void destroy(notification_window*);
};

#endif // NOTIFICATION_WINDOW_HPP_INCLUDED
