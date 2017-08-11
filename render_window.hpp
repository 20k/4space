#ifndef RENDER_WINDOW_HPP_INCLUDED
#define RENDER_WINDOW_HPP_INCLUDED

#include <SFML/Graphics.hpp>

struct render_window : sf::RenderWindow
{
    sf::RenderTexture tex;

    void create(sf::VideoMode mode, const sf::String& title, sf::Uint32 style, const sf::ContextSettings& settings)
    {
        sf::RenderWindow::create(mode, title, style, settings);
        tex.create(mode.width, mode.height);
        tex.setSmooth(true);
    }

    template<typename T, typename... U>
    void draw(const T& t, U... u)
    {
        tex.draw(t, u...);
    }

    template<typename T>
    void setView(const T& t)
    {
         sf::RenderWindow::setView(t);
         tex.setView(t);
    }
};

#endif // RENDER_WINDOW_HPP_INCLUDED
