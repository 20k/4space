#include "text.hpp"

#include <SFML/Graphics.hpp>



bool text_manager::loaded = false;
sf::Font text_manager::font;

void text_manager::load()
{
    if(loaded)
        return;

    font.loadFromFile("VeraMono.ttf");

    loaded = true;
}

void text_manager::render(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col, bool rounding, float char_size, float scale)
{
    load();

    col = col * 255;

    col = clamp(col, 0.f, 255.f);

    if(rounding)
        pos = round(pos);

    sf::Text text(str, font);

    text.setCharacterSize(char_size);
    text.setColor(sf::Color(col.x(), col.y(), col.z()));
    text.setPosition(pos.x(), pos.y());
    text.setScale(scale, scale);

    win.draw(text);
}

void text_manager::render_without_zoom(sf::RenderWindow& win, const std::string& str, vec2f pos, vec3f col, bool centered, float scale)
{
    load();

    col = col * 255;

    col = clamp(col, 0.f, 255.f);

    sf::Text text(str.c_str(), font);

    text.setColor(sf::Color(col.x(), col.y(), col.z()));


    /*sf::View view = win.getView();
    sf::View def_view = win.getDefaultView();
    def_view.setSize(view.getSize());*/


    //float iv_scale = 1.f;
    //iv_scale = def_view.getInverseTransform().transformPoint(iv_scale, 0.f).x / 10000.f;
    /*text.setScale(iv_scale*scale, iv_scale*scale);


    if(centered)
    {
        float w = text.getGlobalBounds().width;

        pos.x() = pos.x() - w/2;
    }

    text.setPosition(pos.x(), pos.y());*/


    /*sf::View backup_view = win.getView();
    sf::View new_view = win.getDefaultView();

    auto spos = win.mapCoordsToPixel({pos.x(), pos.y()});

    vec2f screen_pos = {spos.x, spos.y};

    new_view.setCenter(backup_view.getTransform().transformPoint(backup_view.getCenter()));

    text.setScale(scale, scale);

    if(centered)
    {
        float w = text.getGlobalBounds().width;

        pos.x() = pos.x() - w/2;
    }*/

    if(str == "")
        return;

    sf::View current_view = win.getView();
    sf::View default_view = win.getDefaultView();

    auto projected = win.mapCoordsToPixel({pos.x(), pos.y()});

    vec2f spos = {projected.x, projected.y};

    text.setScale(scale, scale);

    if(centered)
    {
        spos = spos - (vec2f){text.getGlobalBounds().width/2, text.getGlobalBounds().height/2};
    }

    text.setPosition({spos.x(), spos.y()});

    win.setView(default_view);

    win.draw(text);

    win.setView(current_view);
}
