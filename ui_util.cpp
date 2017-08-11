#include "ui_util.hpp"
#include <imgui/imgui_internal.h>
#include <SFML/Graphics.hpp>

bool ImGui::suppress_clicks = false;
int ImGui::suppress_frames = 2;

std::vector<std::string> ImGui::to_skip_frosting;

void ImGui::DoFrosting(sf::RenderWindow& win)
{
    ImGuiContext& g = *GImGui;

    static sf::Shader shader;
    static bool has_shader;

    if(!has_shader)
    {
        shader.loadFromFile("gauss.fglsl", sf::Shader::Type::Fragment);
        shader.setUniform("texture", sf::Shader::CurrentTexture);

        has_shader = true;
    }

    static sf::Texture texture;
    static sf::Texture ntexture;

    if(texture.getSize().x != win.getSize().x || texture.getSize().y != win.getSize().y)
    {
        texture.create(win.getSize().x, win.getSize().y);
        texture.setSmooth(true);

        ntexture.create(win.getSize().x, win.getSize().y);
        ntexture.setSmooth(true);
    }

    texture.update(win);
    texture.generateMipmap();

    const sf::Texture& ref_texture = texture;

    sf::RenderStates states;
    states.shader = &shader;

    sf::BlendMode mode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero);

    states.blendMode = mode;

    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && window->HiddenFrames <= 0 && (window->Flags & (ImGuiWindowFlags_ChildWindow)) == 0)
        {
            if(strncmp(window->Name, "Debug", strlen("Debug")) == 0)
                continue;

            bool skip = false;

            for(auto& str : to_skip_frosting)
            {
                if(strncmp(window->Name, str.c_str(), strlen(str.c_str())) == 0)
                {
                    skip = true;
                    break;
                }
            }

            if(skip)
                continue;

            auto pos = window->Pos;
            ImVec2 dim = window->Size;

            sf::Sprite spr;
            spr.setTexture(ref_texture);
            spr.setPosition(pos.x, pos.y);
            spr.setTextureRect(sf::IntRect({pos.x, pos.y}, {dim.x, dim.y}));

            auto backup_view = win.getView();
            win.setView(win.getDefaultView());

            win.draw(spr, states);

            /*for(int kk=0; kk<5; kk++)
            {
                ntexture.update(win);
                spr.setTexture(ntexture);
                win.draw(spr, states);
            }*/

            win.setView(backup_view);
        }
    }

    to_skip_frosting.clear();

    ImGui::Render();

    win.display();
}

void ImGui::SkipFrosting(const std::string& name)
{
    to_skip_frosting.push_back(name);
}
