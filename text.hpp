#ifndef TEXT_HPP_INCLUDED
#define TEXT_HPP_INCLUDED

#include <SFML/Graphics/Text.hpp>
#include <vec/vec.hpp>

namespace sf
{
    struct RenderWindow;
}

struct sf::RenderWindow;

struct text_manager
{
    static bool loaded;
    static sf::Font font;

    static void load();

    static void render(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col, bool rounding = false, float char_size = 16, float scale = 1.f);

    static void render_without_zoom(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col, bool centered = true, float scale = 1.f);
};

#endif // TEXT_HPP_INCLUDED
