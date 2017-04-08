#ifndef TEXT_HPP_INCLUDED
#define TEXT_HPP_INCLUDED

#include <SFML/Graphics/Text.hpp>
#include <vec/vec.hpp>

namespace sf
{
    struct RenderWindow;
}

struct text_manager
{
    static bool loaded;
    static sf::Font font;

    static void load();

    static void render(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col, bool rounding = false);
};

#endif // TEXT_HPP_INCLUDED
