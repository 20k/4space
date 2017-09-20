#include "ui_util.hpp"
#include <imgui/imgui_internal.h>
#include <SFML/Graphics.hpp>

bool ImGui::suppress_keyboard = false;
bool ImGui::suppress_clicks = false;
int ImGui::suppress_frames = 2;

std::vector<std::string> ImGui::to_skip_frosting;

void has_context_menu::context_tick_menu()
{
    if(context_request_open)
    {
        context_is_open = true;

        //ImGui::OpenPopupEx("Menu", true);
    }

    /*if(context_request_close)
    {
        context_is_open = false;

        //ImGui::CloseCurrentPopup();
    }*/

    //request_context_close = false;
    //request_context_open = false;
}

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
    //static sf::Texture ntexture;

    if(texture.getSize().x != win.getSize().x || texture.getSize().y != win.getSize().y)
    {
        texture.create(win.getSize().x, win.getSize().y);
        texture.setSmooth(true);

        //ntexture.create(win.getSize().x, win.getSize().y);
        //ntexture.setSmooth(true);
    }

    texture.update(win);
    texture.generateMipmap();

    sf::RenderStates states;
    states.shader = &shader;

    sf::BlendMode mode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero);

    states.blendMode = mode;

    auto backup_view = win.getView();
    win.setView(win.getDefaultView());

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
            spr.setTexture(texture);
            spr.setPosition(pos.x, pos.y);
            spr.setTextureRect(sf::IntRect({pos.x, pos.y}, {dim.x, dim.y}));


            win.draw(spr, states);

            /*for(int kk=0; kk<5; kk++)
            {
                ntexture.update(win);
                spr.setTexture(ntexture);
                win.draw(spr, states);
            }*/
        }
    }

    win.setView(backup_view);

    to_skip_frosting.clear();
}

void ImGui::SkipFrosting(const std::string& name)
{
    to_skip_frosting.push_back(name);
}

void ImGui::SolidSmallButton(const std::string& txt, vec3f highlight_col, vec3f col)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 0.f));

    highlight_col = clamp(highlight_col, 0.f, 1.f);

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(highlight_col.x(), highlight_col.y(), highlight_col.z(), 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(highlight_col.x(), highlight_col.y(), highlight_col.z(), 1));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x(), col.y(), col.z(), 0));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(col.x(), col.y(), col.z(), 1));

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.5f));

    auto cpos = ImGui::GetCursorPos();

    ImGui::BeginGroup();

    ImGui::ButtonEx(("###" + txt + "MYBUTT").c_str(), ImVec2(ImGui::GetWindowSize().x - 12,0), ImGuiButtonFlags_AlignTextBaseLine);

    cpos.x += ImGui::GetStyle().FramePadding.x;

    cpos.x = round(cpos.x);

    ImGui::SetCursorPos(cpos);

    ImGui::Text(txt.c_str());

    ImGui::EndGroup();

    ImGui::PopStyleColor(4);

    ImGui::PopStyleVar(3);
}
