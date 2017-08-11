#include "ui_util.hpp"
#include "render_window.hpp"
#include <imgui/imgui_internal.h>

bool ImGui::suppress_clicks = false;
int ImGui::suppress_frames = 2;

void ImGui::DoFrosting(render_window& win)
{
    ImGuiContext& g = *GImGui;

    /*win.tex.display();

    sf::RenderWindow& real_window = win;

    auto backup_view = real_window.getView();
    real_window.setView(real_window.getDefaultView());

    sf::Sprite spr(win.tex.getTexture());
    real_window.draw(spr);
    win.setView(backup_view);*/

    static bool has_shader;
    static sf::Shader shader;

    if(!has_shader)
    {
        shader.loadFromFile("gauss.fglsl", sf::Shader::Type::Fragment);
        shader.setUniform("texture", sf::Shader::CurrentTexture);

        has_shader = true;
    }

    static sf::RenderTexture backup;

    if(backup.getSize().x != win.getSize().x || backup.getSize().y != win.getSize().y)
    {
        backup.create(win.getSize().x, win.getSize().y, false);
        backup.setSmooth(true);
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

    /*sf::Sprite halving(texture);
    halving.setScale(0.5f, 0.5f);

    backup.draw(halving);

    backup.display();*/

    const sf::Texture& ref_texture = texture;
    //const sf::Texture& ref_rexture = backup.getTexture();

    sf::RenderStates states;
    states.shader = &shader;

    sf::BlendMode mode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero);

    states.blendMode = mode;

    sf::Mouse mouse;

    auto mpos = mouse.getPosition(win);

    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && window->HiddenFrames <= 0 && (window->Flags & (ImGuiWindowFlags_ChildWindow)) == 0)
        {
            //if(window->BeginCount == 0)
            //    continue;
            std::cout << window->Name << std::endl;

            if(strncmp(window->Name, "Debug", strlen("Debug")) == 0)
                continue;

            auto pos = window->Pos;
            //ImRect dim = window->ClipRect;
            ImVec2 dim = window->Size;

            //if(mpos.x > pos.x && mpos.x < pos.x + dim.x && mpos.y > pos.y && mpos.y < pos.y + dim.y)
            //    std::cout << window->Name << "  ss" << std::endl;

            //printf("%f %f\n", pos.x, pos.y);

            /*sf::RectangleShape shape;

            shape.setPosition(pos.x, pos.y);
            shape.setSize({dim.x, dim.y});
            shape.setTexture(&ref_rexture);

            sf::IntRect rect({pos.x/2, pos.y/2}, {dim.x/2, dim.y/2});

            shape.setTextureRect(rect);*/

            sf::Sprite spr;
            spr.setTexture(ref_texture);
            spr.setPosition(pos.x, pos.y);
            spr.setTextureRect(sf::IntRect({pos.x, pos.y}, {dim.x, dim.y}));
            //spr.setScale(0.5f, 0.5f);
            //spr.setColor(sf::Color(255, 0, 255, 255));

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

    ImGui::Render();

    win.display();

    //win.tex.clear();
}
