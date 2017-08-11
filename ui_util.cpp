#include "ui_util.hpp"
#include <SFML/Graphics.hpp>

std::vector<vec2f> ImGui::saved_window_positions;
std::vector<vec2f> ImGui::saved_window_sizes;


bool ImGui::suppress_clicks = false;
int ImGui::suppress_frames = 2;

void ImGui::do_frosting(sf::RenderWindow& output, sf::RenderTexture& input)
{
    static sf::RenderTexture tex;

    if(tex.getSize().x != output.getSize().x || tex.getSize().y != output.getSize().y)
        tex.create(output.getSize().x, output.getSize().y);

    input.display();

    sf::Sprite spr;
    //spr.setTexture(output.)
}
