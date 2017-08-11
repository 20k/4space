#include "ui_util.hpp"
#include "render_window.hpp"
#include <imgui/imgui_internal.h>

bool ImGui::suppress_clicks = false;
int ImGui::suppress_frames = 2;

void ImGui::DoFrosting(render_window& win)
{
    ImGuiContext& g = *GImGui;

    win.tex.display();

    sf::RenderWindow& real_window = win;

    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && window->HiddenFrames <= 0 && (window->Flags & (ImGuiWindowFlags_ChildWindow)) == 0)
        {
            auto pos = window->PosFloat;
            //ImRect dim = window->ClipRect;
            ImVec2 dim = window->Size;

            //printf("%f %f\n", pos.x, pos.y);

            sf::RectangleShape shape;

            shape.setPosition(pos.x, pos.y);
            shape.setSize({dim.x, dim.y});
            //shape.setFillColor(sf::Color(255, 0, 255, 255));
            shape.setTexture(&win.tex.getTexture());

            sf::IntRect rect({pos.x, pos.y}, {dim.x, dim.y});

            shape.setTextureRect(rect);

            auto backup_view = win.getView();
            win.setView(win.getDefaultView());

            real_window.draw(shape);

            win.setView(backup_view);
        }
    }

    real_window.display();

    win.tex.clear();
}
