#include "text.hpp"

#include <SFML/Graphics.hpp>

bool text_manager::loaded = false;
sf::Font text_manager::font;

void text_manager::load()
{
    if(loaded)
        return;

    font.loadFromFile("VeraMono.ttf");
}

void text_manager::render(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col)
{
    load();

    col = col * 255;

    col = clamp(col, 0.f, 255.f);

    pos = round(pos);

    sf::Text text(str.c_str(), font);

    text.setCharacterSize(16);
    text.setColor(sf::Color(col.x(), col.y(), col.z()));
    text.setPosition(pos.x(), pos.y());

    win.draw(text);
}
