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
    std::string text;
    bool pressed = false;
};

struct notification_window
{
    std::vector<text_element> elements;

    void tick_draw(sf::RenderWindow& win);
};

struct notification_manager
{
    std::vector<notification_window*> windows;

    notification_window* make_new();
    void destroy(notification_window*);
};

#endif // NOTIFICATION_WINDOW_HPP_INCLUDED
