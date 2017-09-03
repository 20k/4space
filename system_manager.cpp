#include "system_manager.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include "ship.hpp"
#include "util.hpp"
#include "empire.hpp"
#include "procedural_text_generator.hpp"
#include "../../render_projects/imgui/imgui.h"
#include "text.hpp"
#include "tooltip_handler.hpp"
#include "top_bar.hpp"
#include <deque>
#include "popup.hpp"
#include <unordered_map>
#include "ui_util.hpp"
#include <set>


int orbital::gid;
int orbital_system::gid;

float system_manager::universe_scale = 100.f;

double position_history_element::max_history_s = 20;

inline
bool operator<(const empire_popup& e1, const empire_popup& e2)
{
    if(e1.is_player != e2.is_player)
    {
        if(e1.is_player && !e2.is_player)
            return true;

        return false;
    }

    if(e1.is_allied != e2.is_allied)
    {
        if(e1.is_allied && !e2.is_allied)
            return true;

        return false;
    }

    if(e1.hidden != e2.hidden)
    {
        if(e1.hidden && !e2.hidden)
            return false;

        return true;
    }

    if(e1.e != e2.e)
    {
        if(e1.e == nullptr || e2.e == nullptr)
        {
            return std::less<empire*>()(e1.e, e2.e);
        }

        return e1.e->team_id < e2.e->team_id;
    }

    //if(e1.type != e2.type);
    {
        return e1.type < e2.type;
    }

    //creturn e1.id < e2.id;
}


void orbital_simple_renderable::init(int n, float min_rad, float max_rad)
{
    vert_dist.clear();

    for(int i=0; i<n; i++)
    {
        vert_dist.push_back(randf_s(min_rad, max_rad));
    }
}

sf::Vector2f mapCoordsToPixel_float(float x, float y, const sf::View& view, const sf::RenderTarget& target)
{
    sf::Vector2f normalized = view.getTransform().transformPoint(x, y);

    // Then convert to viewport coordinates
    sf::Vector2f pixel;
    sf::IntRect viewport = target.getViewport(view);
    pixel.x = (( normalized.x + 1.f) / 2.f * viewport.width  + viewport.left);
    pixel.y = ((-normalized.y + 1.f) / 2.f * viewport.height + viewport.top);

    return pixel;
}

vec2f map_coords_to_pixel(vec2f pos, const sf::RenderTarget& target)
{
    sf::Vector2f normalized = target.getView().getTransform().transformPoint(pos.x(), pos.y());

    // Then convert to viewport coordinates
    sf::Vector2f pixel;
    sf::IntRect viewport = target.getViewport(target.getView());
    pixel.x = (( normalized.x + 1.f) / 2.f * viewport.width  + viewport.left);
    pixel.y = ((-normalized.y + 1.f) / 2.f * viewport.height + viewport.top);

    return {pixel.x, pixel.y};
}

vec2f map_pixel_to_coords(vec2f pos, const sf::RenderTarget& target)
{
    // First, convert from viewport coordinates to homogeneous coordinates
    sf::Vector2f normalized;
    sf::IntRect viewport = target.getViewport(target.getView());
    normalized.x = -1.f + 2.f * (pos.x() - viewport.left) / viewport.width;
    normalized.y =  1.f - 2.f * (pos.y() - viewport.top)  / viewport.height;

    // Then transform by the inverse of the view matrix
    auto ret =  target.getView().getInverseTransform().transformPoint(normalized);

    return {ret.x, ret.y};
}

void orbital_simple_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, bool force_high_quality, bool draw_outline, const std::string& tag, vec3f col, bool show_detail, orbital* o)
{
    col = col * 255.f;

    auto real_coord = win.mapCoordsToPixel({absolute_pos.x(), absolute_pos.y()});

    if(real_coord.x < 0 || real_coord.x > win.getSize().x || real_coord.y < 0 || real_coord.y >= win.getSize().y)
        return;

    float rad_to_check = 0;

    for(auto& rad : vert_dist)
    {
        rad_to_check += rad;
    }

    rad_to_check /= vert_dist.size();

    auto pixel_rad = mapCoordsToPixel_float(rad_to_check + win.getView().getCenter().x, win.getView().getCenter().y, win.getView(), win);

    pixel_rad.x -= win.getSize().x/2;

    if((draw_outline && !o->rendered_asteroid_window) || o->force_draw_expanded_window)
    {
        if(!show_detail)
            ImGui::SkipFrosting(tag);

        ImGui::SetNextWindowPos(ImVec2(real_coord.x + pixel_rad.x, real_coord.y));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.1));

        ImGuiWindowFlags_ extra = (ImGuiWindowFlags_)0;

        if(!o->force_draw_expanded_window)
        {
            extra = ImGuiWindowFlags_NoInputs;
        }

        ImGui::BeginOverride(tag.c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW_INPUTS | extra);

        ImGui::Text(tag.c_str());

        if((show_detail && o && o->is_resource_object) || o->force_draw_expanded_window)
        {
            auto info = o->produced_resources_ps.get_formatted_str();

            ImGui::Text(info.c_str());
        }

        o->expanded_window_clicked = false;

        if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
            o->expanded_window_clicked = true;
        }

        ImGui::End();


        ImGui::PopStyleColor();

        o->force_draw_expanded_window = false;
    }

    main_rendering(win, rotation, absolute_pos, 1.f, col);
}

void orbital_simple_renderable::main_rendering(sf::RenderWindow& win, float rotation, vec2f absolute_pos, float scale, vec3f col)
{
    #if 1
    for(int i=0; i<vert_dist.size(); i++)
    {
        int cur = i;
        int next = (i + 1) % vert_dist.size();

        float d1 = vert_dist[cur];
        float d2 = vert_dist[next];

        float a1 = ((float)cur / (vert_dist.size())) * 2 * M_PI;
        float a2 = ((float)next / (vert_dist.size())) * 2 * M_PI;

        a1 += rotation;
        a2 += rotation;

        vec2f l1 = d1 * (vec2f){cosf(a1), sinf(a1)} * scale;
        vec2f l2 = d2 * (vec2f){cosf(a2), sinf(a2)} * scale;

        l1 += absolute_pos;
        l2 += absolute_pos;

        vec2f perp = perpendicular((l2 - l1).norm());

        sf::Vertex v[4];

        v[0].position = sf::Vector2f(l1.x(), l1.y());
        v[1].position = sf::Vector2f(l2.x(), l2.y());
        v[2].position = sf::Vector2f(l2.x() + perp.x() * scale, l2.y() + perp.y() * scale);
        v[3].position = sf::Vector2f(l1.x() + perp.x() * scale, l1.y() + perp.y() * scale);

        sf::Color scol = sf::Color(col.x(), col.y(), col.z());

        v[0].color = scol;
        v[1].color = scol;
        v[2].color = scol;
        v[3].color = scol;

        win.draw(v, 4, sf::Quads);
    }
    #endif
}

void orbital_simple_renderable::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(vert_dist, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {

    }

    handled_by_client = true;
}

void premultiply(sf::Image& image);

void sprite_renderable::load(const std::string& str)
{
    if(loaded)
        return;

    img.loadFromFile(str);
    premultiply(img);
    tex.loadFromImage(img);
    tex.setSmooth(true);
    ///smooth rendering causes issues, we'll have to go high res or constructive
    ///also looks hideous

    loaded = true;
}

void sprite_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, vec3f col, bool highlight)
{
    col = col * 255.f;

    col = clamp(col, 0.f, 255.f);

    sf::Sprite spr;
    spr.setTexture(tex);

    spr.setOrigin(spr.getLocalBounds().width/2, spr.getLocalBounds().height/2);
    spr.setRotation(r2d(rotation));

    /*if(highlight)
    {
        float ux = spr.getLocalBounds().width;
        float uy = spr.getLocalBounds().height;

        spr.setScale(2.f, 2.f);
        //spr.setScale((ux + 15) / ux, (uy + 15) / uy);
        spr.setColor(sf::Color(0, 128, 255));

        win.draw(spr);

        spr.setScale(2.f, 2.f);
    }*/

    spr.setColor(sf::Color(col.x(), col.y(), col.z()));

    if(highlight)
    {
        spr.setColor(sf::Color(0, 128, 255));
    }

    spr.setPosition(absolute_pos.x(), absolute_pos.y());

    sf::BlendMode mode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add);

    vec2f spos = map_coords_to_pixel(absolute_pos, win);
    vec2f wspos = map_coords_to_pixel(absolute_pos + (vec2f){tex.getSize().x, 0.f}, win);

    float scale_approx = (wspos.x() - spos.x()) / (tex.getSize().x);

    if(scale_approx >= 1 - 0.1f)
    {
        tex.setSmooth(false);

        spos = round(spos);
    }
    else
    {
        tex.setSmooth(true);
    }

    vec2f new_pos = map_pixel_to_coords(spos, win);

    spr.setPosition({new_pos.x(), new_pos.y()});

    win.draw(spr, mode);
}

void orbital::set_orbit(float ang, float len)
{
    orbital_angle = ang;
    orbital_length = len;

    //printf("SETTING %f %f\n", orbital_angle, orbital_length);

    //angular_velocity_ps = ang_vel_s;
}

void orbital::set_orbit(vec2f pos)
{
    vec2f base;

    if(parent)
        base = parent->absolute_pos;

    vec2f rel = pos - base;

    set_orbit(rel.angle(), rel.length());
}

/*inline
float impl_cos(float x)
{
    x += 1.57079632f;

    if (x >  3.14159265f)
        x -= 6.28318531f;

    if (x < 0)
        return 1.27323954f * x + 0.405284735f * x * x;
    else
        return 1.27323954f * x - 0.405284735f * x * x;
}

inline
float impl_sin(float x)
{
    if (x < 0)
        return 1.27323954f * x + .405284735f * x * x;
    else
        return 1.27323954f * x - 0.405284735f * x * x;
}*/

/*inline
float sin_fast(float x)
{
    float sin;

    if (x < 0)
    {
        sin = 1.27323954f * x + .405284735f * x * x;

        if (sin < 0)
            sin = .225 * (sin *-sin - sin) + sin;
        else
            sin = .225 * (sin * sin - sin) + sin;
    }
    else
    {
        sin = 1.27323954f * x - 0.405284735f * x * x;

        if (sin < 0)
            sin = .225 * (sin *-sin - sin) + sin;
        else
            sin = .225 * (sin * sin - sin) + sin;
    }

    return sin;
}

inline
float cos_fast(float x)
{
    x += 1.57079632f;

    if (x >  3.14159265f)
        x -= 6.28318531f;

    return sin_fast(x);
}*/

inline float impl_sin(float x)
{
    return sinf(x);
}

inline float impl_cos(float x)
{
    return cosf(x);
}

void orbital::do_vision_test()
{
    vision_test_counter++;

    if(((vision_test_counter + unique_id) % 4) != 0)
        return;

    for(empire* e : parent_system->advertised_empires)
    {
        bool currently_has_vision = e->has_vision(parent_system);

        if(currently_has_vision)
            e->update_vision_of_allies(this);

        currently_viewed_by[e] = currently_has_vision;

        if(currently_has_vision)
        {
            viewed_by[e] = true;
        }
    }
}

void check_for_resources(orbital* me)
{
    if(me->type != orbital_info::FLEET)
        return;

    ship_manager* sm = (ship_manager*)me->data;

    if(!sm->any_with_element(ship_component_elements::ORE_HARVESTER))
        return;

    float harvest_dist = 40.f;

    for(orbital* o : me->parent_system->orbitals)
    {
        if(o->type != orbital_info::ASTEROID && !o->is_resource_object)
            continue;

        vec2f my_pos = me->absolute_pos;
        vec2f their_pos = o->absolute_pos;

        float dist = (their_pos - my_pos).length();

        if(dist > harvest_dist)
            continue;

        for(ship* s : sm->ships)
        {
            component* primary = s->get_component_with_primary(ship_component_elements::ORE_HARVESTER);

            if(primary == nullptr)
                continue;

            o->current_num_harvesting++;

            int num_harvest = o->last_num_harvesting;

            if(num_harvest == 0)
                return;

            for(int type = 0; type < primary->components.size(); type++)
            {
                component_attribute& attr = primary->components[type];

                if(!attr.present)
                    continue;

                auto res_type = ship_component_elements::element_infos[(int)type].resource_type;

                if(res_type == resource::COUNT)
                    continue;

                float ore_mult = primary->components[ship_component_elements::ORE_HARVESTER].produced_per_s;

                if(ore_mult > FLOAT_BOUND)
                    ore_mult = (1 + ore_mult)/2.f;

                attr.produced_per_s = ore_mult * o->produced_resources_ps.get_resource(res_type).amount / num_harvest;

                me->is_mining = true;
                me->mining_target = o;
            }

            return;
        }
    }
}

void orbital::tick(float step_s)
{
    if(render_type == 1 && !sprite.loaded)
    {
        sprite.load(orbital_info::load_strs[type]);
    }

    internal_time_s += step_s;

    rotation += rotation_velocity_ps * step_s;

    //if(transferring)
    //    do_transfer(this, step_s);

    if(!owned_by_host)
    {
        command_queue.cancel();
    }

    command_queue.tick(this, step_s);

    is_mining = false;

    if(type == orbital_info::FLEET && ((ship_manager*)data)->any_with_element(ship_component_elements::ORE_HARVESTER))
    {
        check_for_resources(this);
    }

    if(type == orbital_info::ASTEROID && is_resource_object)
    {
        last_num_harvesting = current_num_harvesting;
        current_num_harvesting = 0;
    }

    if(type == orbital_info::FLEET)
    {
        ship_manager* sm = (ship_manager*)data;

        sm->in_friendly_territory = in_friendly_territory();
    }

    if(parent == nullptr)
        return;

    float colo_rate = 1.f * step_s;

    if(being_decolonised && parent_empire != nullptr)
    {
        if(type == orbital_info::FLEET)
        {
            printf("MASSIVE ERROR IN TICK, WRONG TYPE FOR DELOCO\n");
            return;
        }

        decolonise_timer_s += colo_rate;

        if(decolonise_timer_s >= orbital_info::decolonise_time_s)
        {
            parent_empire = nullptr;

            decolonise_timer_s = 0;
        }

        being_decolonised = false;
    }
    else
    {
        decolonise_timer_s -= colo_rate;

        decolonise_timer_s = std::max(decolonise_timer_s, 0.f);

        being_decolonised = false;
    }


    if(!transferring())
        orbital_angle += calculate_orbital_drift_angle(step_s);

    //if(orbital_angle >= M_PIf)
    //    orbital_angle -= 2*M_PIf;

    absolute_pos = orbital_length * (vec2f){impl_cos(orbital_angle), impl_sin(orbital_angle)} + parent->absolute_pos;

    ///the jittering is caused by the internal clock we receive
    ///potentially being out compared to our own estimation of the clock
    ///what would be really useful is to compare the differences between the two clocks
    ///and distribute the error gradually rather than immediately
    if(!owned_by_host)
    {
        if(multiplayer_position_history.size() == 1)
        {
            absolute_pos = multiplayer_position_history[0].pos;
        }

        if(type != orbital_info::FLEET && multiplayer_position_history.size() > 0)
        {
            absolute_pos = multiplayer_position_history.back().pos;
            multiplayer_position_history.clear();
            return;
        }

        for(int i=0; i<((int)multiplayer_position_history.size()) - 1; i++)
        {
            position_history_element& cur = multiplayer_position_history[i];
            position_history_element& next = multiplayer_position_history[i + 1];

            double minternal = internal_time_s - get_orbital_update_rate(type) * 2.f;

            if(minternal >= cur.time_s && minternal < next.time_s)
            {
                double frac = (minternal - cur.time_s) / (next.time_s - cur.time_s);

                absolute_pos = mix(cur.pos, next.pos, frac);

                for(int kk=0; kk < i; kk++)
                {
                    multiplayer_position_history.pop_front();
                }

                break;
            }
        }
    }
}

float orbital::calculate_orbital_drift_angle(float orbital_length, float step_s)
{
    constexpr double mass_of_sun = 2 * pow(10., 30.);
    constexpr double distance_from_earth_to_sun = 149000000000;
    constexpr double g_const = 6.674 * pow(10., -11.);

    constexpr double mu = mass_of_sun * g_const;

    constexpr double default_scale = (365*24*60*60.) / 50.;

    constexpr double dist_div_default_scale = distance_from_earth_to_sun / default_scale;

    constexpr double mu_div = mu / (dist_div_default_scale * dist_div_default_scale * dist_div_default_scale);

    float res = mu_div / (orbital_length * orbital_length * orbital_length);

    float calculated_angular_velocity_ps = sqrtf(res);

    return calculated_angular_velocity_ps * step_s;
}

float orbital::calculate_orbital_drift_angle(float step_s)
{
    return calculate_orbital_drift_angle(orbital_length, step_s);
}

void orbital::begin_render_asteroid_window()
{
    ImGui::SetNextWindowPos(ImVec2(last_screen_pos.x(), last_screen_pos.y()));

    ImGui::BeginOverride((name + "##" + std::to_string(unique_id)).c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW_INPUTS);

    if(rendered_asteroid_window == false)
    {
        ImGui::Text(name.c_str());

        auto info = produced_resources_ps.get_formatted_str();

        ImGui::Text(info.c_str());
    }

    rendered_asteroid_window = true;
}

void orbital::end_render_asteroid_window()
{
    ImGui::End();
}

void orbital::draw(sf::RenderWindow& win, empire* viewer_empire)
{
    /*std::vector<sf::Vertex> lines;

    for(int i=0; i<vert_dist.size(); i++)
    {
        int cur = i;
        int next = (i + 1) % vert_dist.size();

        float d1 = vert_dist[cur];
        float d2 = vert_dist[next];

        float a1 = ((float)cur / (vert_dist.size() + 1)) * 2 * M_PI;
        float a2 = ((float)next / (vert_dist.size() + 1)) * 2 * M_PI;

        vec2f l1 = d1 * (vec2f){cosf(a1), sinf(a1)};
        vec2f l2 = d2 * (vec2f){cosf(a2), sinf(a2)};

        l1 += absolute_pos;
        l2 += absolute_pos;


        lines.push_back(sf::Vertex(sf::Vector2f(l1.x(), l1.y())));
    }

    win.draw(&lines[0], lines.size(), sf::Lines);*/

    /*int wh = 2;

    if(highlight)
    {
        for(int y=-wh; y<=wh; y++)
        {
            for(int x=-wh; x<=wh; x++)
            {
                if(render_type == 0)
                    simple_renderable.draw(win, rotation, absolute_pos + (vec2f){x, y}, {0, 0.5f, 1});
                else
                    sprite.draw(win, rotation, absolute_pos + (vec2f){x, y}, {1, 0.5f, 1});
            }
        }
    }*/

    bool currently_viewed = currently_viewed_by[viewer_empire];

    bool not_currently_viewed_fleet = false;

    if(currently_viewed || type != orbital_info::FLEET)
    {
        last_viewed_position = absolute_pos;
    }

    if(!currently_viewed && type == orbital_info::FLEET)
    {
        not_currently_viewed_fleet = true;
    }

    if(!viewed_by[viewer_empire] && type == orbital_info::FLEET)
        return;

    vec3f current_simple_col = col;
    vec3f current_sprite_col = viewer_empire->get_relations_colour(parent_empire);

    if(not_currently_viewed_fleet)
    {
        current_sprite_col = current_sprite_col/2.f;
    }

    bool force_high_quality = type != orbital_info::ASTEROID;

    bool draw_name_window = type == orbital_info::PLANET;

    sf::Keyboard key;

    //if(is_resource_object && key.isKeyPressed(sf::Keyboard::LAlt))
    //    draw_name_window = false;

    bool show_detail = key.isKeyPressed(sf::Keyboard::LAlt);

    if(type == orbital_info::STAR)
    {
        if(show_detail)
        {
            draw_name_window = true;
            show_detail = false;
        }
    }

    if(render_type == 0)
        simple_renderable.draw(win, rotation, last_viewed_position, force_high_quality, draw_name_window, name, current_simple_col, show_detail, this);
    if(render_type == 1)
        sprite.draw(win, rotation, last_viewed_position, current_sprite_col, highlight);

    auto real_coord = mapCoordsToPixel_float(last_viewed_position.x(), last_viewed_position.y(), win.getView(), win);

    last_screen_pos = {real_coord.x, real_coord.y};

    rendered_asteroid_window = false;

    if(is_resource_object && show_detail && !draw_name_window)
    {
        begin_render_asteroid_window();
        end_render_asteroid_window();
    }

    if(highlight && type == orbital_info::FLEET)
    {
        std::string nname = get_name_with_info_warfare(viewer_empire) + "###" + name + std::to_string(unique_id);

        ImGui::SkipFrosting(nname);

        ImGui::SetNextWindowPos(ImVec2(round(real_coord.x + get_pixel_radius(win)), round(real_coord.y)));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.1));

        ImGui::BeginOverride(nname.c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW);

        ImGui::Text(get_name_with_info_warfare(viewer_empire).c_str());

        ImGui::End();

        ImGui::PopStyleColor();
    }

    was_highlight = highlight;

    highlight = false;
}

float orbital::get_pixel_radius(sf::RenderWindow& win)
{
    float rad_to_check = 0;

    if(render_type == 0)
    {
        for(auto& rad : simple_renderable.vert_dist)
        {
            rad_to_check += rad;
        }

        rad_to_check /= simple_renderable.vert_dist.size();
    }
    else if(render_type == 1)
    {
        rad_to_check = sprite.tex.getSize().x/2.f;
    }

    auto pixel_rad = mapCoordsToPixel_float(rad_to_check + win.getView().getCenter().x, win.getView().getCenter().y, win.getView(), win);

    pixel_rad.x -= win.getSize().x/2;

    return pixel_rad.x;
}

void orbital::center_camera(system_manager& system_manage)
{
    system_manage.camera = absolute_pos;
}

bool orbital::point_within(vec2f pos)
{
    vec2f dim = rad * 1.5f;
    vec2f apos = last_viewed_position;

    if(pos.x() < apos.x() + dim.x() && pos.x() >= apos.x() - dim.x() && pos.y() < apos.y() + dim.y() && pos.y() >= apos.y() - dim.y())
    {
        return true;
    }

    return false;
}

std::vector<std::string> orbital::get_info_str(empire* viewer_empire, bool use_info_warfare, bool full_detail)
{
    if(type != orbital_info::FLEET || data == nullptr)
    {
        std::string str = "Radius: " + to_string_with_enforced_variable_dp(rad);

        //vec2f rpos = round(absolute_pos * 10.f) / 10.f;

        vec2f rpos = absolute_pos;

        std::string pstr = "Position: " + to_string_with_enforced_variable_dp(rpos.x()) + " " + to_string_with_enforced_variable_dp(rpos.y());

        if(type == orbital_info::STAR)
            pstr = "";

        std::string rstr = "";

        if(is_resource_object && viewed_by[viewer_empire])
        {
            rstr = "\n\n" + produced_resources_ps.get_formatted_str();
        }

        return {str + "\n" + pstr + rstr};
    }
    else
    {
        ship_manager* mgr = (ship_manager*)data;

        std::vector<std::string> ret;

        if(use_info_warfare)
            ret = mgr->get_info_strs_with_info_warfare(viewer_empire, this, full_detail);
        else
            ret = mgr->get_info_strs();

        //ret.push_back("Position: " + std::to_string(absolute_pos.x()) + " " + std::to_string(absolute_pos.y()));

        return ret;
    }
}

std::string orbital::get_empire_str(bool newline)
{
    if(parent_empire == nullptr)
        return "";

    std::string str = "Empire: " + parent_empire->name;

    if(newline)
        str += "\n";

    return str;
}

std::string orbital::get_name_with_info_warfare(empire* viewing_empire)
{
    float available_scanning_power = viewing_empire->available_scanning_power_on(this);

    if(viewing_empire == parent_empire)
    {
        available_scanning_power = 1.f;
    }

    if(viewing_empire->is_allied(parent_empire))
    {
        available_scanning_power = 1.f;
    }

    if(available_scanning_power < ship_info::ship_obfuscation_level)
    {
        return "Unknown Fleet";
    }

    return name;
}

void orbital::transfer(float pnew_rad, float pnew_angle, orbital_system* in_system)
{
    /*old_rad = orbital_length;
    old_angle = orbital_angle;
    new_rad = pnew_rad;
    new_angle = pnew_angle;

    transferring = true;

    start_time_s = internal_time_s;*/

    command_queue.transfer(pnew_rad, pnew_angle, this, in_system);
}

void orbital::transfer(vec2f pos, orbital_system* in_system, bool at_back, bool combat_move)
{
    /*vec2f base;

    if(parent)
        base = parent->absolute_pos;

    vec2f rel = pos - base;

    transfer(rel.length(), rel.angle());*/

    command_queue.transfer(pos, this, in_system, at_back, combat_move);
}

float orbital::get_transfer_speed()
{
    float base_speed = 200.f / 5.f;

    if(type != orbital_info::FLEET)
        return base_speed;

    ship_manager* sm = (ship_manager*)data;

    return sm->get_move_system_speed() * base_speed;
}

void orbital::request_transfer(vec2f pos, orbital_system* in_system)
{
    if(type != orbital_info::FLEET)
        return transfer(pos, in_system);

    /*ship_manager* sm = (ship_manager*)data;

    if(sm->any_in_combat())
        return;

    if(sm->can_move_in_system())
        transfer(pos, in_system);*/

    transfer(pos, in_system);
}

bool orbital::transferring()
{
    return command_queue.transferring();
}

void orbital::make_random_resource_asteroid(float total_ps)
{
    ///[1, 4]
    int num_resources = randf_s(1.f, 5.f);

    for(int i=0; i<num_resources; i++)
    {
        int rand_res = (int)resource::get_random_processed();

        produced_resources_ps.resources[rand_res].amount += randf_s(0.25f, total_ps) * resource::global_resource_multiplier;
    }

    float mrad = 3.f;

    if(rad < mrad)
    {
        simple_renderable.init(simple_renderable.vert_dist.size(), mrad, mrad);
        rad = mrad;
    }

    col = {1, 0.8, 0};

    is_resource_object = true;
}

void orbital::make_random_resource_planet(float total_ps)
{
    int num_resources = 4;

    ///ok. Planets should be lacking an asteroid resource (in great quantity)
    ///Planets also generate a small amount of processed resources?

    std::map<resource::types, float> ore_mults;

    /*ore_mults[resource::ICE] = 1.f;
    ore_mults[resource::COPPER_ORE] = 0.2;
    ore_mults[resource::NITRATES] = 1.f;
    ore_mults[resource::IRON_ORE] = 0.3;
    ore_mults[resource::TITANIUM_ORE] = 0.1;
    ore_mults[resource::URANIUM_ORE] = 0.1;*/

    ore_mults[resource::OXYGEN] = 2.f;
    ore_mults[resource::COPPER] = 0.05f;
    ore_mults[resource::HYDROGEN] = 0.7f;
    ore_mults[resource::IRON] = 0.1f;
    ore_mults[resource::TITANIUM] = 0.025;
    ore_mults[resource::URANIUM] = 0.01f;
    ore_mults[resource::RESEARCH] = 1.f;


    float max_val = 0.f;
    int max_elem = 0;

    for(auto& i : ore_mults)
    {
        float val = randf_s(0.55f, total_ps);

        if(val > max_val)//? && i.first != resource::RESEARCH)
        {
            max_val = val;
            max_elem = i.first;
        }

        produced_resources_ps.resources[i.first].amount += val * i.second * resource::global_resource_multiplier;
    }

    resource_type_for_flavour_text = max_elem;

    //printf("%i\n", resource_type_for_flavour_text);

    is_resource_object = true;
}

bool orbital::can_dispense_resources()
{
    if(is_resource_object || orbital_info::PLANET)
        return true;

    return false;
}

void orbital::release_ownership()
{
    if(parent_empire == nullptr)
        return;

    parent_empire->release_ownership(this);
}

void orbital::draw_alerts(sf::RenderWindow& win, empire* viewing_empire, system_manager& system_manage)
{
    if(parent_system != system_manage.currently_viewed)
        return;

    //if(!viewing_empire->has_vision(system_manage.currently_viewed))
    if(!viewed_by[viewing_empire])
        return;

    if(type != orbital_info::FLEET)
    {
        if(has_quest_alert)
        {
            text_manager::render(win, "?", absolute_pos + (vec2f){8, -20}, {0.3, 1.0, 0.3});
        }

        return;
    }

    if(parent_empire != viewing_empire)
        return;

    ship_manager* sm = (ship_manager*)data;

    sm->draw_alerts(win, absolute_pos);
}

bool orbital::is_colonised()
{
    return type == orbital_info::PLANET && parent_empire != nullptr;
}

bool orbital::can_colonise()
{
    return type == orbital_info::PLANET && parent_empire == nullptr;
}

bool orbital::in_friendly_territory_and_not_busy()
{
    if(parent_empire == nullptr)
        return false;

    if(parent_system == nullptr)
        return false;

    bool fleet_is_in_good_state = true;

    if(type == orbital_info::FLEET)
    {
        ship_manager* sm = (ship_manager*)data;

        //if(sm->any_derelict() || sm->any_in_combat())
        if(sm->any_in_combat())
        {
            fleet_is_in_good_state = false;
        }
    }

    for(orbital* o : parent_system->orbitals)
    {
        if(o->parent_empire == nullptr)
            continue;

        if(o->parent_empire == parent_empire)
            continue;

        if(o->parent_empire->is_hostile(parent_empire))
        {
            fleet_is_in_good_state = false;
            break;
        }
    }

    empire* owning_system_empire = parent_system->get_base()->parent_empire;

    if(owning_system_empire == nullptr)
        return false;

    return (owning_system_empire == parent_empire || owning_system_empire->is_allied(parent_empire)) && fleet_is_in_good_state;
}

bool orbital::in_friendly_territory()
{
    if(parent_empire == nullptr)
        return false;

    if(parent_system == nullptr)
        return false;

    bool in_good_state = true;

    for(orbital* o : parent_system->orbitals)
    {
        if(o->parent_empire == nullptr)
            continue;

        if(o->parent_empire == parent_empire)
            continue;

        if(o->parent_empire->is_hostile(parent_empire))
        {
            in_good_state = false;
            break;
        }
    }

    empire* owning_system_empire = parent_system->get_base()->parent_empire;

    if(owning_system_empire == nullptr)
        return false;

    return (owning_system_empire == parent_empire || owning_system_empire->is_allied(parent_empire)) && in_good_state;
}

void orbital::do_serialise(serialise& s, bool ser)
{
    position_history_element saved_history = {absolute_pos, internal_time_s};

    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(expanded_window_clicked, ser);
        s.handle_serialise(force_draw_expanded_window, ser);
        s.handle_serialise(current_num_harvesting, ser);
        s.handle_serialise(last_num_harvesting, ser);
        s.handle_serialise(viewed_by, ser);
        ///CURRENTLY VIEWED BY
        s.handle_serialise(last_viewed_position, ser);
        s.handle_serialise(last_screen_pos, ser);
        s.handle_serialise(in_anchor_ui_state, ser);
        s.handle_serialise(being_decolonised, ser);
        s.handle_serialise(decolonise_timer_s, ser);
        s.handle_serialise(mining_target, ser);
        s.handle_serialise(is_mining, ser);
        s.handle_serialise(construction_ui_open, ser);
        s.handle_serialise(can_construct_ships, ser);
        s.handle_serialise(dialogue_open, ser);
        s.handle_serialise(clicked, ser);
        s.handle_serialise(has_quest_alert, ser);
        s.handle_serialise(parent_system, ser);
        s.handle_serialise(parent_empire, ser);
        s.handle_serialise(produced_resources_ps, ser);
        s.handle_serialise(is_resource_object, ser);
        s.handle_serialise(col, ser);
        s.handle_serialise(vision_test_counter, ser);
        s.handle_serialise(type, ser);
        s.handle_serialise(parent, ser);
        s.handle_serialise(rad, ser);
        s.handle_serialise(orbital_length, ser);
        s.handle_serialise(orbital_angle, ser);
        s.handle_serialise(rotation, ser);
        s.handle_serialise(rotation_velocity_ps, ser);
        s.handle_serialise(universe_view_pos, ser);
        s.handle_serialise(absolute_pos, ser);
        s.handle_serialise(num_moons, ser);

        s.handle_serialise(data, ser);
        s.handle_serialise(render_type, ser);
        s.handle_serialise(simple_renderable, ser);
        s.handle_serialise(was_hovered, ser);
        s.handle_serialise(was_highlight, ser);
        s.handle_serialise(highlight, ser);
        s.handle_serialise(internal_time_s, ser);
        s.handle_serialise(resource_type_for_flavour_text, ser);
        s.handle_serialise(star_temperature_fraction, ser);
        s.handle_serialise(description, ser);
        s.handle_serialise(name, ser);
        s.handle_serialise(num_verts, ser);

        s.handle_serialise(command_queue, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        //s.handle_serialise(expanded_window_clicked, ser);
        //s.handle_serialise(force_draw_expanded_window, ser);
        //s.handle_serialise(current_num_harvesting, ser);
        //s.handle_serialise(last_num_harvesting, ser);
        s.handle_serialise(viewed_by, ser);
        ///CURRENTLY VIEWED BY
        //s.handle_serialise(last_viewed_position, ser);
        //s.handle_serialise(last_screen_pos, ser);
        //s.handle_serialise(in_anchor_ui_state, ser);
        s.handle_serialise(being_decolonised, ser);
        s.handle_serialise(decolonise_timer_s, ser);
        s.handle_serialise(mining_target, ser);
        s.handle_serialise(is_mining, ser);
        //s.handle_serialise(construction_ui_open, ser);
        s.handle_serialise(can_construct_ships, ser);
        //s.handle_serialise(dialogue_open, ser);
        //s.handle_serialise(clicked, ser);
        s.handle_serialise(has_quest_alert, ser);
        s.handle_serialise(parent_system, ser);
        s.handle_serialise(parent_empire, ser);
        s.handle_serialise(produced_resources_ps, ser);
        //s.handle_serialise(is_resource_object, ser);
        //s.handle_serialise(col, ser);
        //s.handle_serialise(vision_test_counter, ser);
        //s.handle_serialise(type, ser);
        s.handle_serialise(parent, ser);
        s.handle_serialise(rad, ser);
        s.handle_serialise(orbital_length, ser);
        s.handle_serialise(orbital_angle, ser);
        s.handle_serialise(rotation, ser);
        s.handle_serialise(rotation_velocity_ps, ser);
        s.handle_serialise(universe_view_pos, ser);
        s.handle_serialise(absolute_pos, ser);
        //s.handle_serialise(num_moons, ser);

        s.handle_serialise(data, ser);
        //s.handle_serialise(render_type, ser);
        //s.handle_serialise(simple_renderable, ser);
        //s.handle_serialise(was_hovered, ser);
        //s.handle_serialise(was_highlight, ser);
        //s.handle_serialise(highlight, ser);
        s.handle_serialise(internal_time_s, ser);
        //s.handle_serialise(resource_type_for_flavour_text, ser);
        //s.handle_serialise(star_temperature_fraction, ser);
        //s.handle_serialise(description, ser);
        //s.handle_serialise(name, ser);
        //s.handle_serialise(num_verts, ser);

        //s.handle_serialise(command_queue, ser);

        /*if(!handled_by_client)
        {
            if(parent_system != nullptr)
            {
                if(type != orbital_info::ASTEROID)
                    parent_system->orbitals.push_back(this);
                else
                    parent_system->asteroids.push_back(this);
            }

            //printf("hi there\n");

            handled_by_client = true;
        }*/

        //printf("well then\n");
    }

    ///we've received a packet
    if(ser == false)
    {
        multiplayer_position_history.push_back({absolute_pos, internal_time_s});

        if(multiplayer_position_history.size() >= position_history_element::max_history_s)
        {
            double time_span = multiplayer_position_history.back().time_s
                             - multiplayer_position_history.front().time_s;

            while(multiplayer_position_history.size() > 2 && time_span > position_history_element::max_history_s)
            {
                time_span = multiplayer_position_history.back().time_s
                          - multiplayer_position_history.front().time_s;

                multiplayer_position_history.pop_front();
            }
        }
    }

    absolute_pos = saved_history.pos;
    //internal_time_s = saved_history.time_s;

    //std::cout << serialise_data_helper::send_mode << std::endl;

    handled_by_client = true;

    //printf("what?\n");
}

float get_orbital_update_rate(orbital_info::type type)
{
    if(type == orbital_info::FLEET)
        return 0.5f;

    return 5.f;
}

/*void orbital::ensure_handled_by_client()
{
    if(handled_by_client == false)
    {
        handled_by_client = true;

        if(render_type == 1)
        {
            sprite.load(orbital_info::load_strs[type]);
        }
    }
}*/

/*void orbital::process_context_ui()
{
    if(type == orbital_info::FLEET)
    {
        ship_manager* sm = (ship_manager*)data;
    }

    if(type == orbital_info::PLANET)
    {

    }
}*/

orbital* orbital_system::get_base()
{
    for(auto& i : orbitals)
    {
        if(i->type == orbital_info::STAR)
            return i;
    }

    return nullptr;
}

orbital* orbital_system::make_new(orbital_info::type type, float rad, int num_verts)
{
    orbital* n = new orbital;

    n->type = type;
    n->rad = rad;
    n->parent_system = this;
    //n->simple_renderable.init(10, n->rad * 0.85f, n->rad * 1.2f);

    procedural_text_generator generator;

    n->name = orbital_info::names[type];

    if(type == orbital_info::PLANET)
    {
        n->make_random_resource_planet(2.f);

        n->name = generator.generate_planetary_name();

        n->description = generator.generate_planetary_text(n);
    }

    else if(type == orbital_info::STAR)
    {
        n->name = generator.generate_star_name();

        float temperature_fraction = generator.generate_star_temperature_fraction(n);
        n->description = generator.generate_star_text(n, temperature_fraction);

        n->star_temperature_fraction = temperature_fraction;
    }

    else if(type == orbital_info::ASTEROID)
    {
        n->col = {1, 1, 1};
    }
    else if(type == orbital_info::FLEET)
    {
        //n->name = generator.generate_fleet_name(n);

        n->name = "";
    }

    n->render_type = orbital_info::render_type[type];
    n->num_verts = num_verts;

    if(n->render_type == 0)
    {
        n->simple_renderable.init(n->num_verts, n->rad * 0.85f, n->rad * 1.2f);
    }
    else if(n->render_type == 1)
    {
        n->sprite.load(orbital_info::load_strs[type]);
    }

    if(n->type == orbital_info::ASTEROID)
    {
        asteroids.push_back(n);
    }
    else
    {
        orbitals.push_back(n);
    }

    n->make_dirty();

    return n;
}

orbital* orbital_system::make_in_place(orbital* n)
{
    n->parent_system = this;

    if(n->render_type == 0)
    {
        n->simple_renderable.init(n->num_verts, n->rad * 0.85f, n->rad * 1.2f);
    }
    else if(n->render_type == 1)
    {
        n->sprite.load(orbital_info::load_strs[n->type]);
    }

    if(n->type == orbital_info::ASTEROID)
    {
        asteroids.push_back(n);
    }
    else
    {
        orbitals.push_back(n);
    }

    return n;
}

/*void orbital_system::ensure_found_orbitals_handled()
{
    for(orbital* o : orbitals)
    {
        if(o->handled_by_client == false)
        {
            o->handled_by_client = true;

            if(o->render_type == 1)
            {
                o->sprite.load(orbital_info::load_strs[o->type]);
            }
        }
    }

    for(orbital* o : asteroids)
    {
        if(o->handled_by_client == false)
        {
            o->handled_by_client = true;

            if(o->render_type == 1)
            {
                o->sprite.load(orbital_info::load_strs[o->type]);
            }
        }
    }
}*/

orbital* orbital_system::make_fleet(fleet_manager& fleet_manage, float rad, float angle, empire* e)
{
    ship_manager* sm = fleet_manage.make_new();

    orbital* o = make_new(orbital_info::FLEET, 5);
    o->orbital_angle = angle;
    o->orbital_length = rad;

    o->parent = get_base();
    o->data = sm;

    if(e != nullptr)
    {
        e->take_ownership(o);
        e->take_ownership(sm);
    }

    o->tick(0.f);

    return o;
}

/*void orbital_system::vision_test_all()
{
    for(orbital* o : orbitals)
    {
        o->do_vision_test();
    }
}*/

void orbital_system::tick(float step_s, orbital_system* viewed)
{
    std::set<empire*> next_empires;

    for(orbital* i : orbitals)
    {
        i->tick(step_s);

        //if(i->parent_empire == viewer_empire)
        {
            i->do_vision_test();
        }

        if(i->parent_empire == nullptr)
            continue;

        next_empires.insert(i->parent_empire);
    }

    advertised_empires = next_empires;

    accumulated_nonviewed_time += step_s;

    if(this != viewed)
    {
        return;
    }

    for(auto& i : asteroids)
    {
        i->tick(accumulated_nonviewed_time);
    }

    accumulated_nonviewed_time = 0.f;
}

void orbital_system::destroy(orbital* o)
{
    if(context_menu::current == (contextable*)o)
    {
        context_menu::set_item(nullptr);
    }

    for(int i=0; i<(int)orbitals.size(); i++)
    {
        if(orbitals[i] == o)
        {
            orbitals.erase(orbitals.begin() + i);
            delete o;
            i--;
            return;
        }
    }

    for(int i=0; i<(int)asteroids.size(); i++)
    {
        if(asteroids[i] == o)
        {
            asteroids.erase(asteroids.begin() + i);
            delete o;
            i--;
            return;
        }
    }
}

void orbital_system::steal(orbital* o, orbital_system* s)
{
    for(int i=0; i<s->orbitals.size(); i++)
    {
        if(s->orbitals[i] == o)
        {
            s->orbitals.erase(s->orbitals.begin() + i);
            break;
        }
    }

    /*for(int i=0; i<s->total_orbitals.size(); i++)
    {
        if(s->total_orbitals[i] == o)
        {
            s->total_orbitals.erase(s->total_orbitals.begin() + i);
            break;
        }
    }*/

    orbitals.push_back(o);
    //total_orbitals.push_back(o);

    o->parent = get_base();
    o->parent_system = this;
}

///need to figure out higher positioning, but whatever
void orbital_system::draw(sf::RenderWindow& win, empire* viewer_empire)
{
    //sf::Clock clk;

    //if(!viewer_empire->has_vision(this))
    //    return;

    for(auto& i : asteroids)
    {
        i->draw(win, viewer_empire);
    }

    for(int kk=orbitals.size()-1; kk >= 0; kk--)
    {
        orbitals[kk]->draw(win, viewer_empire);
    }

    //printf("elapsed %f\n", clk.getElapsedTime().asMicroseconds() / 1000.f);
}

void orbital_system::cull_empty_orbital_fleets(empire_manager& empire_manage, popup_info& popup)
{
    //for(orbital* o : orbitals)
    for(int i=0; i<orbitals.size(); i++)
    {
        orbital* o = orbitals[i];

        if(o->type == orbital_info::FLEET)
        {
            ship_manager* smanage = (ship_manager*)o->data;

            if(smanage->ships.size() == 0)
            {
                popup.rem(o);

                //printf("culled");

                empire_manage.notify_removal(o);

                destroy(o);
                i--;
                continue;
            }
        }
    }
}

orbital* orbital_system::get_by_element(void* element)
{
    for(auto& i : orbitals)
    {
        if(i->data == element)
            return i;
    }

    return nullptr;
}

void orbital_system::generate_asteroids_new(int n, int num_belts, int num_resource_asteroids)
{
    std::vector<float> exclusion_radiuses;

    for(auto& i : orbitals)
    {
        if(i->type == orbital_info::PLANET)
        {
            exclusion_radiuses.push_back(i->orbital_length);
        }
    }

    //float min_belt = 50.f;

    //float max_belt = 500.f;

    //float exclusion_radius = 60.f;

    //float max_randomness = 0.05f;

    float max_random_radius = 25.f;

    int asteroids_per_belt = n / (exclusion_radiuses.size() + 1);

    std::vector<orbital*> cur_orbitals;

    int vnum = 0;

    for(int i=0; i<exclusion_radiuses.size(); i++)
    {
        if(randf_s(0.f, 1.f) < 0.3f && vnum > 0)
            continue;

        float min_rad = exclusion_radiuses[i] + 30;

        float max_rad = min_rad + 80;

        if(i < exclusion_radiuses.size() - 1)
            max_rad = exclusion_radiuses[i+1] - 30;

        float rad = randf_s(min_rad, max_rad);

        rad = (min_rad + max_rad)/2;

        for(int kk=0; kk<asteroids_per_belt; kk++)
        {
            float angle = ((float)kk / (asteroids_per_belt + 1)) * 2 * M_PI;

            float len = rad + randf_s(-max_random_radius, max_random_radius);//rad * randf_s(1 - max_randomness, 1 + max_randomness);

            orbital* o = make_new(orbital_info::ASTEROID, 2.f * randf_s(0.5f, 1.5f), randf_s(4, 6));
            o->orbital_angle = angle + randf_s(0.f, M_PI*2/16.f);
            o->orbital_length = len;

            o->parent = get_base();
            o->rotation_velocity_ps = 2 * M_PI / randf_s(10.f, 1000.f);

            cur_orbitals.push_back(o);
        }

        vnum++;
    }

    if(cur_orbitals.size() == 0)
        return;

    int c = 0;
    int max_c = 100;

    for(int i=0; i<num_resource_asteroids && c < max_c; i++, c++)
    {
        int n = randf_s(0.f, cur_orbitals.size());

        if(cur_orbitals[n]->is_resource_object)
        {
            i--;
            continue;
        }

        cur_orbitals[n]->make_random_resource_asteroid(1.f);
        make_asteroid_orbital(cur_orbitals[n]);
    }
}

void orbital_system::generate_asteroids_old(int n, int num_belts, int num_resource_asteroids)
{
    std::vector<std::pair<float, float>> exclusion_radiuses;

    for(auto& i : orbitals)
    {
        if(i->type == orbital_info::PLANET)
        {
            float excl_rad = 20;

            if(i->num_moons >= 1)
            {
                excl_rad = 50;
            }

            exclusion_radiuses.push_back({i->orbital_length, excl_rad});
        }
    }

    float min_belt = 50.f;

    float max_belt = 500.f;

    float exclusion_radius = 60.f;

    float max_randomness = 0.05f;

    float max_random_radius = 20.f;

    int asteroids_per_belt = n / num_belts;

    std::vector<orbital*> cur_orbitals;

    for(int i=0; i<num_belts; i++)
    {
        float rad = 0.f;

        bool bad = false;

        int max_tries = 40;
        int cur_tries = 0;

        do
        {
            bad = false;

            rad = randf_s(min_belt, max_belt);

            for(auto& i : exclusion_radiuses)
            {
                if(fabs(rad - i.first) < i.second)
                {
                    bad = true;
                    break;
                }
            }

            cur_tries++;
        }
        while(bad && cur_tries < max_tries);

        exclusion_radiuses.push_back({rad, 10});

        for(int kk=0; kk<asteroids_per_belt; kk++)
        {
            float angle = ((float)kk / (asteroids_per_belt + 1)) * 2 * M_PI;

            float len = rad + randf_s(-max_random_radius, max_random_radius);//rad * randf_s(1 - max_randomness, 1 + max_randomness);

            orbital* o = make_new(orbital_info::ASTEROID, 2.f * randf_s(0.5f, 1.5f), 5);
            o->orbital_angle = angle + randf_s(0.f, M_PI*2/16.f);
            o->orbital_length = len;

            o->parent = get_base();
            o->rotation_velocity_ps = 2 * M_PI / randf_s(10.f, 1000.f);

            cur_orbitals.push_back(o);
        }
    }

    int c = 0;
    int max_c = 100;

    for(int i=0; i<num_resource_asteroids && c < max_c; i++, c++)
    {
        int n = randf_s(0.f, cur_orbitals.size());

        if(cur_orbitals[n]->is_resource_object)
        {
            i--;
            continue;
        }

        cur_orbitals[n]->make_random_resource_asteroid(1.f);
        make_asteroid_orbital(cur_orbitals[n]);
    }
}

void orbital_system::generate_planet_resources(float max_ps)
{
    for(orbital* o : orbitals)
    {
        if(o->type == orbital_info::PLANET)
        {
            o->make_random_resource_planet(max_ps);
        }
    }
}

void orbital_system::draw_alerts(sf::RenderWindow& win, empire* viewing_empire, system_manager& system_manage)
{
    for(orbital* o : orbitals)
    {
        o->draw_alerts(win, viewing_empire, system_manage);
    }
}

std::string scifi_name(int num)
{
    if(num == 0)
        return "I";
    if(num == 1)
        return "II";
    if(num == 2)
        return "III";
    if(num == 4)
        return "IV";
    if(num == 5)
        return "V";

    return "";
}

void orbital_system::generate_random_system(int planets, int num_asteroids, int num_belts, int num_resource_asteroids)
{
    float min_srad = 8.f;
    float max_srad = 15.f;

    orbital* sun = make_new(orbital_info::STAR, randf_s(min_srad, max_srad), 10);
    sun->rotation_velocity_ps = randf_s(2*M_PI/120.f, 2*M_PI/8.f);
    sun->absolute_pos = {0,0};

    bool has_close_partner = randf_s(0.f, 1.f) < 0.3f;

    float min_planet_distance = 60.f;

    if(!has_close_partner)
    {
        min_planet_distance = 150.f;
    }

    float max_planet_distance = 700.f;

    float randomness = 10.f;

    int min_verts = 5;
    int max_verts = 25;

    float min_rad = 3.f;
    float max_rad = 8.f;

    for(int i=0; i<planets; i++)
    {
        orbital* planet = make_new(orbital_info::PLANET, randf_s(min_rad, max_rad), randf_s(min_verts, max_verts));

        float nfrac = (float)i / planets;

        float ndist = nfrac * (max_planet_distance - min_planet_distance) + min_planet_distance;

        float rdist = ndist + randf_s(-randomness, randomness);

        if(rdist < 60)
        {
            rdist = 60;
        }

        planet->orbital_length = rdist;
        planet->orbital_angle = randf_s(0.f, 2*M_PI);
        planet->rotation_velocity_ps = randf_s(0.f, 2*M_PI/10.f);

        planet->parent = sun;

        int moons = randf_s(0, 3);

        for(int moon_num = 0; moon_num < moons; moon_num++)
        {
            orbital* moon = make_new(orbital_info::MOON, randf_s(min_rad/5, max_rad/5), randf_s(min_verts, max_verts));

            float mfrac = (float)moon_num / moons;

            float min_dist = 15.f;
            float max_dist = 30.f;

            //float moon_rad = randf_s(0.f, 1.f) * (max_dist - min_dist) + min_dist;

            float moon_rad = mfrac * (max_dist - min_dist) + min_dist + randf_s(0.f, min_dist);

            moon->orbital_length = moon_rad;
            moon->orbital_angle = randf_s(0.f, 2 * M_PI);

            moon->rotation_velocity_ps = randf_s(0.f, 2*M_PI/10.f);

            moon->parent = planet;

            planet->num_moons++;

            moon->name = planet->name + " (" + scifi_name(moon_num) + ")";
        }
    }

    generate_asteroids_old(num_asteroids, num_belts, num_resource_asteroids);
    //generate_planet_resources(2.f);
}

void orbital_system::generate_full_random_system(bool force_planet)
{
    int min_planets = 0;
    int max_planets = 5;

    if(force_planet)
        min_planets = 1;

    int min_asteroids = 100;
    int max_asteroids = 200;

    int min_belts = 1;
    int max_belts = 6;

    int min_resource_asteroids = 0;
    int max_resource_asteroids = 6;

    int rplanets = randf_s(min_planets, max_planets + 1);
    int rasteroids = randf_s(min_asteroids, max_asteroids + 1);
    int rbelts = randf_s(min_belts, max_belts + 1);
    int rresource = randf_s(min_resource_asteroids, max_resource_asteroids + 1);

    rasteroids += randf_s(20.f, 50.f) * rbelts;

    return generate_random_system(rplanets, rasteroids, rbelts, rresource);
}

std::vector<orbital*> orbital_system::get_fleets_within_engagement_range(orbital* me, bool only_hostile)
{
    std::vector<orbital*> ret;

    if(me->type != orbital_info::FLEET)
        return ret;

    vec2f ref_pos = me->absolute_pos;

    float engagement_rad = 40.f;

    for(orbital* o : orbitals)
    {
        if(me->parent_empire == o->parent_empire)
            continue;

        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        if(!sm->can_engage())
            continue;

        if(only_hostile)
        {
            if(!me->parent_empire->is_hostile(o->parent_empire))
                continue;
        }

        vec2f their_pos = o->absolute_pos;

        float dist = (their_pos - ref_pos).length();

        if(dist < engagement_rad)
        {
            ret.push_back(o);
        }
    }

    return ret;
}

bool orbital_system::can_engage(orbital* me, orbital* them)
{
    float dist = (me->absolute_pos - them->absolute_pos).length();

    float engagement_rad = 40.f;

    if(me->type != orbital_info::FLEET || them->type != orbital_info::FLEET)
        return false;

    ship_manager* sm_me = (ship_manager*)me->data;
    ship_manager* sm_them = (ship_manager*)them->data;

    if(dist < engagement_rad && sm_me->can_engage() && sm_them->can_engage())
    {
        return true;
    }

    return false;
}

void orbital_system::make_asteroid_orbital(orbital* o)
{
    orbitals.push_back(o);

    for(int i=0; i<asteroids.size(); i++)
    {
        if(asteroids[i] == o)
        {
            asteroids.erase(asteroids.begin() + i);
            i--;
            continue;
        }
    }
}

bool orbital_system::is_owned()
{
    assert(get_base() != nullptr);

    return get_base()->parent_empire != nullptr;
}

std::string orbital_system::get_resource_str(bool include_vision, empire* viewer_empire, bool only_owned)
{
    resource_manager resources;

    for(orbital* o : orbitals)
    {
        if(!o->is_resource_object)
            continue;

        if(!o->viewed_by[viewer_empire] && include_vision)
            continue;

        if(only_owned && o->parent_empire != viewer_empire)
            continue;

        for(int res = 0; res < o->produced_resources_ps.resources.size(); res++)
        {
            resources.resources[res].amount += o->produced_resources_ps.resources[res].amount;
        }
    }

    return resources.get_processed_str(true);
}

resource_manager orbital_system::get_potential_resources()
{
    resource_manager resources;

    for(orbital* o : orbitals)
    {
        if(!o->is_resource_object)
            continue;

        for(int res = 0; res < o->produced_resources_ps.resources.size(); res++)
        {
            resources.resources[res].amount += o->produced_resources_ps.resources[res].amount;
        }
    }

    return resources;
}

void orbital_system::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(toggle_fleet_ui, ser);
        s.handle_serialise(accumulated_nonviewed_time, ser);
        s.handle_serialise(highlight, ser);
        s.handle_serialise(asteroids, ser);
        s.handle_serialise(orbitals, ser);
        s.handle_serialise(universe_pos, ser);
        s.handle_serialise(advertised_empires, ser);
        ///ADVERTISED EMPIRES?
    }

    if(serialise_data_helper::send_mode == 0)
    {
        //s.handle_serialise(toggle_fleet_ui, ser);
        //s.handle_serialise(accumulated_nonviewed_time, ser);
        //s.handle_serialise(highlight, ser);
        //s.handle_serialise(asteroids, ser);
        s.handle_serialise(orbitals, ser);
        //s.handle_serialise(universe_pos, ser);
        //s.handle_serialise(advertised_empires, ser);
    }

    if(serialise_data_helper::send_mode == 2)
    {
        if(ser)
        {
            int32_t extra = 0;

            for(orbital* o : orbitals)
            {
                if(o->dirty > 0)
                {
                    extra++;
                }
            }

            s.handle_serialise(extra, ser);

            for(orbital* o : orbitals)
            {
                if(o->dirty > 0)
                {
                    s.handle_serialise(o, ser);
                }
            }
        }
        else
        {
            int32_t extra = 0;

            s.handle_serialise(extra, ser);

            for(int i=0; i<extra; i++)
            {
                orbital* o = nullptr;

                s.handle_serialise(o, ser);

                orbitals.push_back(o);
            }
        }
    }

    handled_by_client = true;
}

system_manager::system_manager()
{
    fleet_tex.loadFromFile(orbital_info::load_strs[orbital_info::FLEET]);
    fleet_sprite.setTexture(fleet_tex);
}

orbital_system* system_manager::make_new()
{
    orbital_system* sys = new orbital_system;

    systems.push_back(sys);

    sys->make_dirty();

    return sys;
}

orbital_system* system_manager::get_parent(orbital* o)
{
    for(auto& i : systems)
    {
        for(auto& m : i->orbitals)
        {
            if(m == o)
                return i;
        }
    }

    return nullptr;
}

orbital_system* system_manager::get_by_element(void* ptr)
{
    for(auto& i : systems)
    {
        for(auto& m : i->orbitals)
        {
            if(m->data == ptr)
                return i;
        }
    }

    return nullptr;
}

orbital* system_manager::get_by_element_orbital(void* ptr)
{
    for(auto& i : systems)
    {
        for(auto& m : i->orbitals)
        {
            if(m->data == ptr)
                return m;
        }
    }

    return nullptr;
}

float heuristic(orbital_system* start, orbital_system* fin)
{
    return (start->universe_pos - fin->universe_pos).length();
}

std::vector<orbital_system*> reconstruct_path(std::map<orbital_system*, orbital_system*>& came_from, orbital_system* current)
{
    std::vector<orbital_system*> total_path{current};

    while(came_from[current] != nullptr)
    {
        current = came_from[current];

        total_path.push_back(current);
    }

    std::reverse(total_path.begin(), total_path.end());

    return total_path;
}

std::vector<orbital_system*> system_manager::pathfind(float max_warp_distance, orbital_system* start, orbital_system* fin)
{
    std::vector<orbital_system*> ret;

    if(fin == nullptr)
        return ret;

    float max_distance = max_warp_distance;

    std::vector<orbital_system*> closed_set;
    std::vector<orbital_system*> open_set{start};

    std::map<orbital_system*, orbital_system*> came_from;

    std::map<orbital_system*, float> g_score;
    std::map<orbital_system*, float> f_score;

    g_score[start] = 0.f;
    f_score[start] = heuristic(start, fin);

    while(open_set.size() > 0)
    {
        std::vector<std::pair<orbital_system*, float>> sorted;

        for(auto& i : open_set)
        {
            sorted.push_back({i, f_score[i]});
        }

        std::sort(sorted.begin(), sorted.begin(),
                  [](const std::pair<orbital_system*, float>& p1, const std::pair<orbital_system*, float>& p2){return p1.second < p2.second;});


        orbital_system* current_sys = sorted[0].first;

        if(current_sys == fin)
        {
            return reconstruct_path(came_from, current_sys);
        }

        auto it = std::find(open_set.begin(), open_set.end(), current_sys);

        open_set.erase(it);

        closed_set.push_back(current_sys);

        for(orbital_system* next_sys : systems)
        {
            if(next_sys == current_sys)
                continue;

            vec2f dist = next_sys->universe_pos - current_sys->universe_pos;

            if(dist.length() >= max_distance)
                continue;

            if(std::find(closed_set.begin(), closed_set.end(), next_sys) != closed_set.end())
                continue;

            if(std::find(open_set.begin(), open_set.end(), next_sys) == open_set.end())
            {
                open_set.push_back(next_sys);
            }

            float found_gscore = g_score[current_sys] + heuristic(next_sys, current_sys);

            auto looking_gscore = g_score.find(next_sys);

            float testing_gscore = FLT_MAX;

            if(looking_gscore != g_score.end())
            {
                testing_gscore = looking_gscore->second;
            }

            if(found_gscore >= testing_gscore)
                continue;

            came_from[next_sys] = current_sys;

            g_score[next_sys] = found_gscore;
            f_score[next_sys] = found_gscore + heuristic(next_sys, fin);
        }
    }

    return ret;
}

std::vector<orbital_system*> system_manager::pathfind(orbital* o, orbital_system* fin)
{
    if(o->type != orbital_info::FLEET)
        return std::vector<orbital_system*>();

    ship_manager* sm = (ship_manager*)o->data;

    return pathfind(sm->get_min_warp_distance(), o->parent_system, fin);
}

void system_manager::add_draw_pathfinding(const std::vector<orbital_system*>& path)
{
    to_draw_pathfinding.push_back(path);
}

std::vector<orbital_system*> system_manager::get_nearest_n(orbital_system* os, int n)
{
    std::vector<orbital_system*> syst = systems;

    ///thanks c++. This time not even 100% sarcastically
    ///don't need to multiply by universe scale because space is relative dude
    std::sort(syst.begin(), syst.end(),

    [&](const orbital_system* a, const orbital_system* b) -> bool
    {
        return (a->universe_pos - os->universe_pos).length() < (b->universe_pos - os->universe_pos).length();
    });

    std::vector<orbital_system*> ret;

    for(orbital_system* sys : syst)
    {
        if(sys == os)
            continue;

        ret.push_back(sys);

        if(ret.size() >= n)
            break;
    }

    return ret;
}

void system_manager::tick(float step_s)
{
    for(auto& i : systems)
    {
        i->tick(step_s, currently_viewed);
    }

    repulse_fleets();
}

void system_manager::destroy(orbital_system* s)
{
    for(int i=0; i<(int)systems.size(); i++)
    {
        if(systems[i] == s)
        {
            systems.erase(systems.begin() + i);
            delete s;
            i--;
            return;
        }
    }
}

void repulse(orbital* o1, orbital* o2)
{
    /*if(o1->absolute_pos == o2->absolute_pos)
    {
        o1->orbital_length -= 1;

        return;
    }*/

    if(o1->transferring() || o2->transferring())
        return;

    vec2f o1_to_o2 = (o2->absolute_pos - o1->absolute_pos);

    float len_sq = o1_to_o2.squared_length();

    if(len_sq < 0.0001f)
    {
        o1_to_o2 = {1, 0};
    }

    vec2f new_o1_pos = -o1_to_o2.norm() + o1->absolute_pos;
    vec2f new_o2_pos = o1_to_o2.norm() + o2->absolute_pos;

    if(o1->owned_by_host)
        o1->transfer(new_o1_pos, o1->parent_system, false);

    if(o2->owned_by_host)
        o2->transfer(new_o2_pos, o2->parent_system, false);
}

void system_manager::repulse_fleets()
{
    for(orbital_system* sys : systems)
    {
        for(orbital* o : sys->orbitals)
        {
            ///this is pretty funny though
            if(o->type != orbital_info::FLEET)
            {
                continue;
            }

            if(!o->owned_by_host)
                continue;

            for(orbital* k : sys->orbitals)
            {
                if(k == o)
                    continue;

                if(k->type != orbital_info::FLEET)
                    continue;

                vec2f a1 = o->absolute_pos;
                vec2f a2 = k->absolute_pos;

                float dist = (a2 - a1).length();

                if(dist < 10.f)
                    repulse(o, k);
            }
        }
    }
}

void system_manager::cull_empty_orbital_fleets(empire_manager& empire_manage, popup_info& popup)
{
    for(auto& i : systems)
    {
        i->cull_empty_orbital_fleets(empire_manage, popup);
    }
}

void system_manager::add_selected_orbital(orbital* o)
{
    if(o->type != orbital_info::FLEET)
        return;

    next_frame_warp_radiuses.push_back(o);
}

void system_manager::draw_warp_radiuses(sf::RenderWindow& win, empire* viewing_empire)
{
    if(in_system_view())
    {
        next_frame_warp_radiuses.clear();
        return;
    }

    for(orbital* o : next_frame_warp_radiuses)
    {
        if(!viewing_empire->is_allied(o->parent_empire) && viewing_empire != o->parent_empire)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        float rad = sm->get_min_warp_distance() * universe_scale;

        sf::CircleShape circle;

        circle.setPointCount(200);
        circle.setRadius(rad);

        circle.setFillColor(sf::Color(20, 60, 20, 50));

        circle.setOutlineColor(sf::Color(200, 200, 200));
        circle.setOutlineThickness(10.f);

        circle.setOrigin(circle.getLocalBounds().width/2, circle.getLocalBounds().height/2);

        vec2f pos = o->parent_system->universe_pos * universe_scale;

        circle.setPosition({pos.x(), pos.y()});

        win.draw(circle);
    }

    next_frame_warp_radiuses.clear();
}

void system_manager::draw_alerts(sf::RenderWindow& win, empire* viewing_empire)
{
    ///change this so that later we see alerts from the overall map
    ///would be like, totally useful like!
    if(!in_system_view())
        return;

    for(auto& i : systems)
    {
        i->draw_alerts(win, viewing_empire, *this);
    }
}

void system_manager::draw_viewed_system(sf::RenderWindow& win, empire* viewer_empire)
{
    if(currently_viewed == nullptr)
        return;

    if(!in_system_view())
        return;

    currently_viewed->draw(win, viewer_empire);
}

void system_manager::set_viewed_system(orbital_system* s, bool reset_zoom)
{
    currently_viewed = s;

    if(s != nullptr && s->get_base())
    {
        if(reset_zoom)
            set_zoom(1.f);

        s->get_base()->center_camera(*this);
    }
    else
    {
        //printf("Warning no base object!\n");
    }

    top_bar::active[top_bar_info::UNIVERSE] = false;
}

bool intersect(vec2f p1, vec2f p2, float r)
{
    return (p1 - p2).length() < r * 2;
}

std::pair<vec2f, vec2f> get_intersection(vec2f p1, vec2f p2, float rad)
{
    vec2f sub_1 = p1 - p2;

    float R = sub_1.length();

    float r1 = rad;
    float r2 = rad;

    float x1 = p1.x();
    float y1 = p1.y();
    float x2 = p2.x();
    float y2 = p2.y();

    float R2 = R*R;
    float R4 = R2*R2;

    if(fabs(r1 - r2) > R || R > r1 + r2)
        return {0.f, 0.f};

    float a = (r1 * r1 - r2 * r2) / (2 * R2);
    float r2r2 = (r1 * r1 - r2 * r2);
    float c = sqrt(2 * (r1 * r1 + r2 * r2) / R2 - (r2r2 * r2r2) / R4 - 1);

    vec2f fv = (p1 + p2) / 2.f + a * (p2 - p1);
    vec2f iv_gv = (c * (p1 - p2) / 2.f);
    vec2f gv = {-iv_gv.y(), iv_gv.x()};

    vec2f i1 = fv + gv;
    vec2f i2 = fv - gv;

    return {i1, i2};
}

///do clicking next, bump up to higher level?
bool universe_fleet_ui_tick(sf::RenderWindow& win, sf::Sprite& fleet_sprite, vec2f pos, vec2f screen_offset, vec3f col)
{
    bool no_suppress_mouse = !ImGui::IsAnyItemHovered() && !ImGui::IsMouseHoveringAnyWindow();

    sf::Mouse mouse;

    vec2f mpos = {mouse.getPosition(win).x, mouse.getPosition(win).y};

    auto scr_pos = win.mapCoordsToPixel({pos.x(), pos.y()});

    vec2f real_pos = {scr_pos.x + screen_offset.x(), scr_pos.y + screen_offset.y() - fleet_sprite.getGlobalBounds().height/2.f};

    vec2f dim = {fleet_sprite.getGlobalBounds().width, fleet_sprite.getGlobalBounds().height};

    vec2f tl = real_pos;// - dim/2.f;
    vec2f br = real_pos + dim;

    bool is_hovered = false;

    if(no_suppress_mouse && mpos.x() > tl.x() && mpos.y() > tl.y() && mpos.x() < br.x() && mpos.y() < br.y())
    {
        col = {0, 0.5, 1};

        /*if(ONCE_MACRO(sf::Mouse::Left))
        {
            was_clicked = true;
        }*/

        is_hovered = true;
    }

    real_pos = round(real_pos);

    fleet_sprite.setPosition(real_pos.x(), real_pos.y());
    fleet_sprite.setColor(sf::Color(col.x() * 255.f, col.y() * 255.f, col.z() * 255.f, 255));

    auto backup_view = win.getView();
    win.setView(win.getDefaultView());
    win.draw(fleet_sprite);
    win.setView(backup_view);

    return is_hovered;
}

template<typename T>
void piecewise_linear(T& accumulator, T mstart, T mend, float mstart_frac, float mend_frac, float val)
{
    if(val <= mstart_frac || val > mend_frac)
        return;

    float modified = (val - mstart_frac) / (mend_frac - mstart_frac);

    accumulator = mix(mstart, mend, modified);
}

vec3f temperature_fraction_to_colour(float temperature_fraction)
{
    vec3f rcol = {1, 0, 1};

    float vmax = 230;

    piecewise_linear<vec3f>(rcol, {vmax, 40, 40}, {vmax, vmax, 0}, 0.f, 0.4f, temperature_fraction);
    piecewise_linear<vec3f>(rcol, {vmax, vmax, 0}, {vmax, vmax, vmax}, 0.4f, 0.8f, temperature_fraction);
    piecewise_linear<vec3f>(rcol, {vmax, vmax, vmax}, {10, 10, vmax}, 0.8f, 1.f, temperature_fraction);

    return rcol;
}

float temperature_fraction_to_size_fraction(float temperature_fraction)
{
    float size_fraction = 1.f;

    piecewise_linear(size_fraction, 0.7f, 0.3f, 0.f, 0.05f, temperature_fraction);

    piecewise_linear(size_fraction, 0.3f, 0.6f, 0.05f, 0.8, temperature_fraction);
    piecewise_linear(size_fraction, 0.6f, 1.f, 0.8f, 1.0f, temperature_fraction);

    return size_fraction;
}

vec3f get_gray_colour(vec3f in)
{
    in = in / 2.f;

    float avg = (in.x() + in.y() + in.z()) / 3.f;

    vec3f avg_vec = {avg, avg, avg};

    return mix(avg_vec, in, 0.2f);
}

bool within_circle(vec2f point, vec2f pos, float rad)
{
    return (pos - point).length() < rad;
}

bool lies_on_boundary(vec2f point, std::vector<vec2f>& points, std::vector<float>& radiuses)
{
    for(int i=0; i<points.size(); i++)
    {
        vec2f pos = points[i];
        float rad = radiuses[i];

        if(within_circle(point, pos, rad - 2))
            return false;
    }

    return true;
}

float system_manager::temperature_fraction_to_star_size(float temperature_frac)
{
    float my_radius_frac = temperature_fraction_to_size_fraction(temperature_frac);

    float max_deviation = 0.5f;

    float rad = sun_universe_rad - max_deviation * sun_universe_rad + sun_universe_rad * my_radius_frac;

    return rad;
}

void system_manager::draw_universe_map(sf::RenderWindow& win, empire* viewer_empire, popup_info& popup)
{
    //printf("zoom %f\n", zoom_level);

    suppress_click_away_fleet = false;
    hovered_orbitals.clear();
    advertised_universe_orbitals.clear();

    if(in_system_view())
        return;

    /*for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        if(os->get_base()->parent_empire != nullptr)
        {
            vec3f col = os->get_base()->parent_empire->colour * 255.f;

            sf::CircleShape ncircle;

            ncircle.setFillColor({20, 20, 20, 255});
            ncircle.setOutlineColor(sf::Color(col.x(), col.y(), col.z()));

            ncircle.setRadius(sun_universe_rad * 5.5f);

            ncircle.setOrigin(ncircle.getLocalBounds().width/2, ncircle.getLocalBounds().height/2);

            //ncircle.setOutlineThickness(sun_universe_rad / 15.f);
            ncircle.setPosition({pos.x(), pos.y()});

            ncircle.setPointCount(100.f);

            win.draw(ncircle);
        }
    }*/

    sf::BlendMode blend(sf::BlendMode::One, sf::BlendMode::One, sf::BlendMode::Add);

    float frad = border_universe_rad;

    sf::CircleShape nc2;
    nc2.setFillColor({0,0,0,255});
    nc2.setRadius(frad);
    nc2.setOrigin(nc2.getLocalBounds().width/2, nc2.getLocalBounds().height/2);
    nc2.setPointCount(80.f);
    nc2.setOutlineThickness(sun_universe_rad / 15.f);

    for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        auto projected = mapCoordsToPixel_float(pos.x(), pos.y(), win.getView(), win);

        if(projected.x + frad < 0 || projected.y + frad < 0 || projected.x - frad >= win.getSize().x || projected.y - frad >= win.getSize().y)
            continue;

        if(os->get_base()->parent_empire != nullptr)
        {
            vec3f col = os->get_base()->parent_empire->colour * 255.f;

            nc2.setOutlineColor(sf::Color(col.x(), col.y(), col.z()));
            nc2.setPosition({pos.x(), pos.y()});

            win.draw(nc2);
            //win.draw(nc2, blend);
        }
    }

    //sf::BlendMode b2(sf::BlendMode::SrcColor, sf::BlendMode::DstColor, sf::BlendMode::Add);
    //sf::CircleShape ncircle;
    //ncircle.setFillColor({0, 0, 0, 255});
    //ncircle.setRadius(frad);
    //ncircle.setOutlineThickness(sun_universe_rad / 15.f);
    //ncircle.setOrigin(ncircle.getLocalBounds().width/2, ncircle.getLocalBounds().height/2);
    nc2.setFillColor(sf::Color(20,20,20,255));
    //ncircle.setPointCount(80.f);
    nc2.setOutlineColor(sf::Color(0,0,0,0));

    for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        auto projected = mapCoordsToPixel_float(pos.x(), pos.y(), win.getView(), win);

        if(projected.x + frad < 0 || projected.y + frad < 0 || projected.x - frad >= win.getSize().x || projected.y - frad >= win.getSize().y)
            continue;

        if(os->get_base()->parent_empire != nullptr)
        {
            nc2.setPosition({pos.x(), pos.y()});

            ///change to add if we want to see overlap
            win.draw(nc2);
        }
    }

    /*nc2.setFillColor({20, 20, 20, 255});
    //ncircle.setRadius(frad);
    //ncircle.setOrigin(ncircle.getLocalBounds().width/2, ncircle.getLocalBounds().height/2);
    //ncircle.setPointCount(80.f);


    for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        auto projected = mapCoordsToPixel_float(pos.x(), pos.y(), win.getView(), win);

        if(projected.x + frad < 0 || projected.y + frad < 0 || projected.x - frad >= win.getSize().x || projected.y - frad >= win.getSize().y)
            continue;

        if(os->get_base()->parent_empire != nullptr)
        {
            vec3f col = os->get_base()->parent_empire->colour * 255.f;

            col = col / 10.f;

            //nc2.setFillColor(sf::Color(col.x(), col.y(), col.z()));
            nc2.setPosition({pos.x(), pos.y()});

            ///change to add if we want to see overlap
            win.draw(nc2);
        }
    }*/

    std::vector<vec2f> intersection_points;
    std::vector<vec2f> centres;
    std::vector<float> radiuses;

    sf::RectangleShape shape;

    for(orbital_system* o1 : systems)
    {
        for(orbital_system* o2 : systems)
        {
            if(o1 == o2)
                continue;

            if(!intersect(o1->universe_pos * universe_scale, o2->universe_pos * universe_scale, frad))
                continue;

            if(o1->get_base()->parent_empire == nullptr || o2->get_base()->parent_empire == nullptr)
                continue;

            //if(o1->get_base()->parent_empire == viewer_empire && o2->get_base()->parent_empire == viewer_empire)
            //    continue;

            if(o1->get_base()->parent_empire == o2->get_base()->parent_empire)
                continue;

            auto intersection = get_intersection(o1->universe_pos * universe_scale, o2->universe_pos * universe_scale, frad);

            vec2f p1 = intersection.first;
            vec2f p2 = intersection.second;

            intersection_points.push_back(p1);
            intersection_points.push_back(p2);

            centres.push_back(o1->universe_pos * universe_scale);
            radiuses.push_back(frad);

            //shape.setSize({400.f, 400.f});
            shape.setSize({(p2 - p1).length(), sun_universe_rad/15.f});
            shape.setRotation(r2d((p2 - p1).angle()));

            //shape.setOrigin(shape.getLocalBounds().width/2, shape.getLocalBounds().height/2);
            shape.setOrigin(0.f, shape.getLocalBounds().height/2.f);

            shape.setPosition(p1.x(), p1.y());

            vec3f colour = (o1->get_base()->parent_empire->colour + o2->get_base()->parent_empire->colour)/2.f;

            colour = colour * 255.f;

            shape.setFillColor({colour.x(), colour.y(), colour.z()});

            win.draw(shape);
        }
    }

    /*for(vec2f& point : intersection_points)
    {
        if(lies_on_boundary(point, centres, radiuses))
        {
            sf::CircleShape shape;

            shape.setRadius(5.f * universe_scale);
            shape.setPosition(point.x(), point.y());
            shape.setFillColor(sf::Color(255, 255, 255, 255));
            win.draw(shape);
        }
    }*/

    ///need to ensure that empire side stays consistent wrt line as well when tracing
    std::vector<vec2f> boundary_points;

    for(vec2f& point : intersection_points)
    {
        if(lies_on_boundary(point, centres, radiuses))
        {
            if(std::find(boundary_points.begin(), boundary_points.end(), point) == boundary_points.end())
            {
                boundary_points.push_back(point);
            }
        }
    }


    sf::CircleShape circle;
    circle.setRadius(sun_universe_rad);
    circle.setOrigin(circle.getLocalBounds().width/2, circle.getLocalBounds().height/2);

    sf::Keyboard key;

    float blip_size = sun_universe_rad / 5.5f;

    sf::CircleShape planet_blip;
    planet_blip.setRadius(blip_size);
    planet_blip.setOrigin(planet_blip.getLocalBounds().width/2, planet_blip.getLocalBounds().height/2);

    for(int i=0; i<systems.size(); i++)
    {
        orbital_system* os = systems[i];


        vec2f pos = os->universe_pos * universe_scale;

        auto projected = mapCoordsToPixel_float(pos.x(), pos.y(), win.getView(), win);

        if(projected.x + frad < 0 || projected.y + frad < 0 || projected.x - frad >= win.getSize().x || projected.y - frad >= win.getSize().y)
            continue;

        circle.setPosition({pos.x(), pos.y()});

        vec3f col = temperature_fraction_to_colour(os->get_base()->star_temperature_fraction);

        if(os->highlight)
        {
            col = col * 1.5f;
        }

        if(!os->get_base()->viewed_by[viewer_empire])
        {
            col = get_gray_colour(col);
        }

        col = clamp(col, 0.f, 255.f);

        circle.setFillColor(sf::Color(col.x(), col.y(), col.z()));

        float star_rad = temperature_fraction_to_star_size(os->get_base()->star_temperature_fraction);

        circle.setRadius(star_rad);
        circle.setOrigin(circle.getLocalBounds().width/2, circle.getLocalBounds().height/2);

        win.draw(circle);

        vec2f bottom = {pos.x(), pos.y() + star_rad * 1.8f};

        std::vector<orbital*> planets;
        std::vector<orbital*> resource_asteroids;

        for(orbital* o : os->orbitals)
        {
            if(o->type == orbital_info::PLANET)
                planets.push_back(o);

            if(o->type == orbital_info::ASTEROID && o->is_resource_object)
                resource_asteroids.push_back(o);
        }

        if(os->get_base()->viewed_by[viewer_empire])
        {
            for(int kk=0; kk<planets.size(); kk++)
            {
                float xwidth = planets.size() * sun_universe_rad / 4.f;//sun_universe_rad;

                float xoffset = 0.f;

                if(planets.size() > 1)
                    xoffset = (((float)kk / (planets.size() - 1.f)) - 0.5f) * xwidth * 2;

                planet_blip.setPosition(bottom.x() + xoffset, bottom.y());

                orbital* o = planets[kk];

                vec3f col = viewer_empire->get_relations_colour(o->parent_empire);

                if(o->parent_empire == viewer_empire)
                {
                    col = relations_info::alt_owned_col;
                }

                col = col * 255.f;

                planet_blip.setFillColor(sf::Color(col.x(), col.y(), col.z()));

                win.draw(planet_blip);
            }

            if(planets.size() > 0)
            {
                bottom.y() += sun_universe_rad * 0.6f + blip_size/2.f;
                //bottom.y() += star_rad * 0.8f + blip_size; ///for perfect separation
            }

            for(int kk=0; kk<resource_asteroids.size(); kk++)
            {
                float xwidth = resource_asteroids.size() * sun_universe_rad / 4.f;//sun_universe_rad;

                float xoffset = 0.f;

                if(resource_asteroids.size() > 1)
                    xoffset = (((float)kk / (resource_asteroids.size() - 1.f)) - 0.5f) * xwidth * 2;

                orbital* o = resource_asteroids[kk];

                vec3f col = o->col * 255.f;

                orbital_simple_renderable& renderable = o->simple_renderable;

                renderable.main_rendering(win, 0.f, {bottom.x() + xoffset, bottom.y()}, blip_size/2.f, col);
            }
        }


        /*if(key.isKeyPressed(sf::Keyboard::LAlt))
        {
            std::string use_name = os->get_base()->name + "##" + std::to_string(i);

            ImGui::SkipFrosting(use_name);

            ImGui::SetNextWindowPos(ImVec2(projected.x, projected.y));

            ImGui::BeginOverride(use_name.c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW);

            ImGui::Text(os->get_base()->name.c_str());

            ImGui::End();
        }*/

        os->highlight = false;

        ///BEGIN SHIP DISPLAY

        if(os->get_base() == nullptr)
            continue;

        if(!viewer_empire->has_vision(os))
            continue;

        ///ok this all works for the moment
        ///next up clicking the fleet icon should select all ships of that class
        std::unordered_map<empire*, std::vector<orbital*>> sorted_orbitals;

        std::map<int, std::vector<orbital*>> classed_orbitals;

        ///the index for classed_orbital represents the draw order, must be synced with colours
        for(int i=0; i<os->orbitals.size(); i++)
        {
            orbital* o = os->orbitals[i];

            if(o->type != orbital_info::FLEET)
                continue;

            advertised_universe_orbitals.push_back(o);

            sorted_orbitals[o->parent_empire].push_back(o);

            ship_manager* sm = (ship_manager*)o->data;

            ///we could probably retrieve colour here?
            if(viewer_empire == sm->parent_empire)
            {
                classed_orbitals[0].push_back(o); ///owned
            }
            else if(viewer_empire->is_allied(sm->parent_empire))
            {
                classed_orbitals[1].push_back(o); ///allied
            }
            else if(viewer_empire->is_hostile(sm->parent_empire))
            {
                classed_orbitals[3].push_back(o); ///hostile
            }
            else
            {
                classed_orbitals[2].push_back(o); ///neutral
            }
        }

        if(sorted_orbitals.size() == 0)
            continue;

        float height = fleet_sprite.getGlobalBounds().height;
        float draw_offset = height + 1.f;

        vec2f fleet_draw_pos = pos + (vec2f){star_rad, 0.f};
        vec2f screen_offset = {2, 0};

        vec3f colours[4] =
                           {relations_info::base_col,
                            relations_info::friendly_col,
                            relations_info::neutral_col,
                            relations_info::hostile_col};

        int num_offsets = classed_orbitals.size();

        if(num_offsets > 0)
        {
            screen_offset.y() = -(num_offsets - 1) * draw_offset / 2.f;
        }

        ///ok. Instead, this should advertise which orbitals are hovered
        for(auto& i : classed_orbitals)
        {
            int num = i.first;

            vec3f col = colours[num];

            bool any_highlighted = false;

            for(orbital* o : i.second)
            {
                if(o->highlight)
                {
                    any_highlighted = true;
                    o->highlight = false;
                }

                //o->universe_view_pos = fleet_draw_pos;

                auto mapped_spos = win.mapCoordsToPixel({fleet_draw_pos.x(), fleet_draw_pos.y()});

                mapped_spos = mapped_spos + sf::Vector2i(screen_offset.x(), screen_offset.y());

                auto mapped_wpos = win.mapPixelToCoords(mapped_spos);

                o->universe_view_pos = {mapped_wpos.x, mapped_wpos.y};
            }

            if(any_highlighted)
            {
                col = {0, 0.5, 1};
            }

            bool hovered = universe_fleet_ui_tick(win, fleet_sprite, fleet_draw_pos, screen_offset, col);

            if(hovered)
            {
                for(auto& o : i.second)
                {
                    hovered_orbitals.push_back(o);
                }
            }

            screen_offset.y() += draw_offset;
        }
    }

    for(auto& path : to_draw_pathfinding)
    {
        std::vector<orbital_system*> next_path = path;

        for(int i=0; i<((int)path.size())-1; i++)
        {
            orbital_system* p1 = next_path[i];
            orbital_system* p2 = next_path[i+1];

            vec2f pos_1 = p1->universe_pos * universe_scale;
            vec2f pos_2 = p2->universe_pos * universe_scale;

            /*float max_displacement = std::max(temperature_fraction_to_star_size(p1->get_base()->star_temperature_fraction),
                                              temperature_fraction_to_star_size(p2->get_base()->star_temperature_fraction));*/

            float max_displacement = sun_universe_rad * 1.2f;

            vec2f perp = perpendicular(pos_2 - pos_1);

            int width = 32;

            float offset_rad = max_displacement * 1.5f;

            float len = (pos_2 - pos_1).length() - offset_rad*2;

            sf::RectangleShape rect;
            rect.setSize({len, width});
            rect.setRotation(r2d((pos_2 - pos_1).angle()));
            rect.setOrigin({0, width/2});

            //vec2f next_pos = pos_1 + perp.norm() * 50.f;
            vec2f next_pos = pos_1;
            next_pos = next_pos + (pos_2 - pos_1).norm() * offset_rad;

            rect.setPosition(next_pos.x(), next_pos.y());

            win.draw(rect);
        }
    }

    to_draw_pathfinding.clear();
}

void system_manager::process_universe_map(sf::RenderWindow& win, bool lclick, empire* viewer_empire)
{
    hovered_system = currently_viewed;

    ///ok, i've done this in a stupid way which means that I can't set the pad on the top bar
    ///:(
    /*if(top_bar::active[top_bar_info::UNIVERSE])
    {
        if(in_system_view())
            enter_universe_view();
        else
            set_viewed_system(get_nearest_to_camera(), true);

        top_bar::active[top_bar_info::UNIVERSE] = false;
    }*/

    if(top_bar::active[top_bar_info::UNIVERSE])
    {
        if(in_system_view())
        {
            enter_universe_view();
        }
    }
    else
    {
        if(!in_system_view())
        {
            set_viewed_system(get_nearest_to_camera(), true);
        }
    }

    if(in_system_view())
        return;

    hovered_system = nullptr;

    sf::Mouse mouse;

    int x = mouse.getPosition(win).x;
    int y = mouse.getPosition(win).y;

    auto transformed = win.mapPixelToCoords({x, y});

    vec2f tpos = {transformed.x, transformed.y};

    for(orbital_system* s : systems)
    {
        vec2f apos = s->universe_pos * universe_scale;

        if((tpos - apos).length() < sun_universe_rad)
        {
            s->highlight = true;

            hovered_system = s;

            std::string str;

            if(s->get_base() != nullptr)
            {
                str = s->get_base()->name;

                if(s->get_base()->parent_empire != nullptr)
                {
                    str += "\n" + s->get_base()->get_empire_str(false);
                }

                str += "\n" + s->get_resource_str(true, viewer_empire, false);

                //ImGui::SetTooltip(str.c_str());
                tooltip::add(str.c_str());
            }

            if(lclick)
            {
                set_viewed_system(s);

                lclick = false;
            }
        }
    }
}

void system_manager::change_zoom(float amount, vec2f mouse_pos, sf::RenderWindow& win)
{
    //auto game_pos = win.mapPixelToCoords({mouse_pos.x(), mouse_pos.y()});

    float zoom = zoom_level;

    float root_2 = sqrt(2.f);

    if(amount > 0)
    {
        zoom *= pow(root_2, amount + 1);
    }
    else if(amount < 0)
    {
        #ifdef ZOOM_TOWARDS
        camera = (vec2f){game_pos.x, game_pos.y};
        #endif

        zoom /= pow(root_2, fabs(amount) + 1);

        ///zoom into position
    }

    set_zoom(zoom, true);
}

void system_manager::set_zoom(float zoom, bool auto_enter_system)
{
    bool was_in_system_view = in_system_view();

    float min_zoom = 1.f / 1000.f;
    //float max_zoom = 5.f;

    zoom_level = zoom;

    if(zoom_level < min_zoom)
        zoom_level = min_zoom;

    bool is_in_system_view = in_system_view();

    if(currently_viewed == nullptr)
        return;

    if(is_in_system_view && !was_in_system_view)
    {
        if(auto_enter_system)
            set_viewed_system(get_nearest_to_camera(), false);

        camera = camera - currently_viewed->universe_pos * universe_scale;

        if(auto_enter_system)
            currently_viewed->get_base()->center_camera(*this);
    }
    if(was_in_system_view && !is_in_system_view)
    {
        camera = currently_viewed->universe_pos * universe_scale;
        //camera = camera + currently_viewed->universe_pos * universe_scale;
    }

    top_bar::active[top_bar_info::UNIVERSE] = !is_in_system_view;
}

void system_manager::pan_camera(vec2f dir)
{
    camera = camera - dir * zoom_level;
}

bool system_manager::in_system_view()
{
    return zoom_level < 8;
}

void system_manager::enter_universe_view()
{
    //zoom_level = 10;
    set_zoom(10.f, true);

    top_bar::active[top_bar_info::UNIVERSE] = true;
}

orbital_system* system_manager::get_nearest_to_camera()
{
    float dist = FLT_MAX;
    orbital_system* f = nullptr;

    for(orbital_system* s : systems)
    {
        float found_dist = (camera - s->universe_pos * universe_scale).length();

        if(found_dist < dist)
        {
            f = s;
            dist = found_dist;
        }
    }

    return f;
}

float get_next_radius(float angle_in)
{
    float A = 1.f;
    float B = 0.5f;

    float N = M_PI * 1.1;

    float tan_val = tan(angle_in / (2 * N));

    if(fabs(tan_val) > 100000000.f)
    {
        return 0.f;
    }

    float ltan_val = log(B * tan_val);

    return A / ltan_val;
}

vec2f get_random_pos(int neg_status = 0)
{
    float random_angle = randf_s(0.f, 2*M_PI);

    float rad_scale = 300.f;

    float radius = get_next_radius(random_angle) * rad_scale;

    float random_scale = rad_scale / 5.f;

    //random_scale = mix(rad_scale / 4.f, rad_scale / 10.f, fabs(radius / rad_scale));

    float random_radius = radius + randf_s(-1.f, 1.f) * random_scale;

    if(randf_s(0.f, 1.f) < 0.1f)
    {
        random_radius = randf_s(0.f, 1.f) * randf_s(0.f, 1.f) * rad_scale * 0.5;
    }

    vec2f random_position = random_radius * (vec2f){cosf(random_angle), sinf(random_angle)};

    random_position = random_position.rot((neg_status / 2.f) * 2 * M_PI);

    return random_position;
}

void system_manager::generate_universe(int num)
{
    float exclusion_rad = 20.f;

    for(int i=0; i<num; i++)
    {
        int neg = (i % 2);

        orbital_system* sys = make_new();
        sys->generate_full_random_system();

        vec2f random_position = get_random_pos(neg);

        int c = 0;

        do
        {
            bool all_good = true;

            for(orbital_system* other : systems)
            {
                if((other->universe_pos - random_position).length() < exclusion_rad)
                {
                    all_good = false;

                    break;
                }
            }

            if(all_good)
                break;

            random_position = get_random_pos(neg);

            c++;
        }
        while(c < 100);

        sys->universe_pos = random_position;
    }
}

///group systems by empire owner
void system_manager::draw_ship_ui(empire* viewing_empire, popup_info& popup)
{
    if(!top_bar::get_active(top_bar_info::FLEETS))
        return;

    sf::Keyboard key;

    bool lshift = key.isKeyPressed(sf::Keyboard::LShift);
    bool lctrl = key.isKeyPressed(sf::Keyboard::LControl);

    ImGui::BeginOverride("Fleets", &top_bar::active[top_bar_info::FLEETS], ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    std::map<empire_popup, std::vector<orbital_system*>> empire_to_systems;

    empire* unknown_empire = viewing_empire->parent->unknown_empire;

    for(orbital_system* os : systems)
    {
        //if(os->get_base()->parent_empire == nullptr)
        //    continue;

        empire* found_empire = os->get_base()->parent_empire;

        if(found_empire == nullptr)
        {
            found_empire = unknown_empire;
        }

        ///need to skip systems if we have no scanning power on it
        ///also skip ships with no scanning power on them
        empire_popup pop;
        pop.e = found_empire;
        pop.id = os->unique_id;
        pop.hidden = found_empire == unknown_empire;
        pop.type = orbital_info::NONE;
        pop.is_player = found_empire == viewing_empire;

        empire_to_systems[pop].push_back(os);

        //empire_to_systems[os->get_base()->parent_empire].push_back(os);
    }

    int snum = 0;

    ///really want to force the first empire to be player, somehow
    ///bundle all up below into do_empire_system_stuff?
    for(auto& sys_emp : empire_to_systems)
    {
        int sys_c = 0;

        bool first_of_empire = true;
        bool first_system_seen = true;

        std::string empire_name_str;

        const empire_popup& pop = sys_emp.first;

        empire* current_empire = pop.e;

        if(current_empire != nullptr)
            empire_name_str = "Systems owned by: " + current_empire->name + " (" + std::to_string(sys_emp.second.size()) + ")";
        if(current_empire == unknown_empire)
            empire_name_str = "Systems Unowned";

        ///or... maybe draw for all fleets we know of?
        ///sort these all into a std::map -> vector of empires
        for(int sys_c = 0; sys_c < sys_emp.second.size(); sys_c++)
        {
            orbital_system* sys = sys_emp.second[sys_c];

            std::string sys_name = sys->get_base()->name;

            std::map<empire_popup, std::vector<orbital*>> ship_map;

            bool any_orbitals = false;

            for(orbital* o : sys->orbitals)
            {
                if(o->type != orbital_info::FLEET)
                    continue;

                if(!o->viewed_by[viewing_empire])
                    continue;

                ship_manager* sm = (ship_manager*)o->data;

                bool do_obfuscate = false;

                if(viewing_empire->available_scanning_power_on((ship_manager*)sm, *this) <= ship_info::accessory_information_obfuscation_level && !viewing_empire->is_allied(sm->parent_empire))
                {
                    do_obfuscate = true;
                }

                if(!viewing_empire->has_direct_vision(sys))
                {
                    continue;
                }

                empire_popup pop;
                pop.e = o->parent_empire;
                pop.id = o->unique_id;
                pop.hidden = do_obfuscate;
                pop.type = o->type;
                pop.is_player = o->parent_empire == viewing_empire;
                pop.is_allied = viewing_empire->is_allied(o->parent_empire);

                /*if(!do_obfuscate)
                    ship_map[o->parent_empire].push_back(o);
                else
                    ship_map[nullptr].push_back(o); ///feels pretty icky doing this*/

                ship_map[pop].push_back(o);
            }

            if(ship_map.size() == 0 && sys->get_base()->parent_empire != viewing_empire)
                continue;

            if(snum != 0 && first_of_empire)
                ImGui::Text("\n");

            snum = 1;

            if(first_of_empire)
            {
                std::string pad = "-";

                if(current_empire->toggle_systems_ui)
                    pad = "+";

                vec3f col = viewing_empire->get_relations_colour(current_empire);

                ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1), (pad + empire_name_str).c_str());

                if(ImGui::IsItemClicked_Registered())
                {
                    current_empire->toggle_systems_ui = !current_empire->toggle_systems_ui;
                }

                first_of_empire = false;
            }

            if(current_empire->toggle_systems_ui == false)
                continue;

            ImGui::Indent();

            ///spacing between systems
            if(!first_system_seen)
                ImGui::Text("\n");

            first_system_seen = false;

            std::string sys_pad = "-";

            if(sys->toggle_fleet_ui)
                sys_pad = "+";

            ImVec4 system_col(1,1,1,1);

            bool viewing = (currently_viewed == sys && in_system_view());

            if(viewing)
            {
                system_col = ImVec4(0.35, 0.55, 1.f, 1.f);
            }

            if(hovered_system == sys && !in_system_view())
            {
                system_col = ImVec4(1, 1, 0.05, 1);
            }

            ImGui::TextColored(system_col, (sys_pad + sys_name).c_str());

            if(ImGui::IsItemClicked_Registered())
            {
                sys->toggle_fleet_ui = !sys->toggle_fleet_ui;
            }

            ImGui::SameLine();

            ImGui::NeutralText("(Jump To System)");

            if(ImGui::IsItemClicked_Registered())
            {
                set_viewed_system(sys);
            }

            int remove_len = sys_name.length();

            if(viewing)
            {
                std::string view_str = "| Viewing";

                ImGui::SameLine();

                ImGui::Text(view_str.c_str());

                remove_len += view_str.length() + 1;
            }

            #define DASH_LINES
            #ifdef DASH_LINES
            ImGui::SameLine();

            std::string str;

            for(int i=0; i<40 - remove_len; i++)
            {
                str = str + "-";
            }

            ImGui::Text(str.c_str());

            if(ImGui::IsItemClicked_Registered())
            {
                sys->toggle_fleet_ui = !sys->toggle_fleet_ui;
            }
            #endif

            if(!sys->toggle_fleet_ui)
            {
                ImGui::Unindent();
                continue;
            }

            ImGui::Indent();

            for(auto& kk : ship_map)
            {
                const empire_popup& pop = kk.first;
                empire* e = pop.e;

                std::string empire_name;

                if(!pop.hidden)
                {
                    empire_name = "Empire: " + e->name;
                }
                else
                {
                    empire_name = "Empire: Unknown";
                }

                vec3f col = viewing_empire->get_relations_colour(pop.hidden ? nullptr : e);

                ImGui::TextColored(ImVec4(col.x(), col.y(), col.z(), 1.f), empire_name.c_str());

                if(ImGui::IsItemHovered() && e != nullptr)
                {
                    std::string relations_str = to_string_with_enforced_variable_dp(viewing_empire->get_culture_modified_friendliness(e));

                    if(!pop.hidden)
                    {
                        if(viewing_empire != e)
                            tooltip::add(relations_str + " (" + viewing_empire->get_relations_string(e) + ")");
                        else
                            tooltip::add("(Your Empire)");
                    }
                }

                ImGui::Indent();

                for(orbital* o : kk.second)
                {
                    ship_manager* sm = (ship_manager*)o->data;

                    bool do_obfuscate = false;

                    if(viewing_empire->available_scanning_power_on((ship_manager*)sm, *this) <= ship_info::ship_obfuscation_level && !viewing_empire->is_allied(sm->parent_empire))
                    {
                        do_obfuscate = true;
                    }

                    std::string fleet_name = o->name;

                    if(fleet_name == "")
                        fleet_name = orbital_info::names[o->type];

                    std::vector<std::string> display_txt = sm->get_info_strs();

                    if(do_obfuscate)
                        display_txt = {"Unknown Ship"};

                    if(do_obfuscate)
                        fleet_name = "Unknown Fleet";

                    std::string combat_pad = "";

                    if(sm->any_in_combat())
                        combat_pad = "!";

                    ///need system location... ruh roh
                    ///shunt this into orbital manager?

                    std::string pad = "-";

                    if(sm->toggle_fleet_ui)
                        pad = "+";

                    ///when clicking on fleet, also select it
                    ///could solve a few of our ui problems!
                    ImGui::Text((pad + combat_pad + fleet_name + combat_pad + pad).c_str());

                    bool shipname_clicked = ImGui::IsItemClicked_Registered();

                    if(sm->all_derelict())
                    {
                        ImGui::SameLine();
                        ImGui::Text("(Derelict)");
                    }

                    if(shipname_clicked)
                    {
                        sm->toggle_fleet_ui = !sm->toggle_fleet_ui;

                        if(sm->toggle_fleet_ui && !lctrl)
                        {
                            if(!lshift)
                            {
                                for(popup_element& pe : popup.elements)
                                {
                                    orbital* o2 = (orbital*)pe.element;

                                    if(o2->type != orbital_info::FLEET)
                                        continue;

                                    if(o2 == o)
                                        continue;

                                    ship_manager* sm = (ship_manager*)o2->data;

                                    sm->to_close_ui = true;
                                }

                                popup.elements.clear();
                            }

                            bool already_in = false;

                            for(auto& i : popup.elements)
                            {
                                if(i.element == o)
                                {
                                    already_in = true;
                                    break;
                                }
                            }


                            popup.going = true;

                            popup_element nelem;
                            nelem.element = o;

                            if(!already_in)
                                popup.elements.push_back(nelem);
                        }
                        else
                        {
                            for(int i=0; i<popup.elements.size() && !lctrl; i++)
                            {
                                if(popup.elements[i].element == o)
                                {
                                    popup.elements.erase(popup.elements.begin() + i);
                                    i--;
                                    break;
                                }
                            }
                        }

                        if(sm->toggle_fleet_ui)
                            continue;
                    }

                    if(sm->toggle_fleet_ui)
                    {
                        ImGui::Indent();

                        int num = 0;

                        for(auto& i : display_txt)
                        {
                            ImGui::Text((combat_pad + i + combat_pad).c_str());

                            #ifdef NO_OPEN_STEALTH_SHIPS_SYSTEM
                            if(ImGui::IsItemClicked_Registered() && !do_obfuscate)
                            #else
                            if(ImGui::IsItemClicked_Registered())
                            #endif
                            {
                                sm->ships[num]->display_ui = !sm->ships[num]->display_ui;
                            }

                            num++;
                        }

                        ImGui::Unindent();
                    }

                }

                ImGui::Unindent();
            }

            ImGui::Unindent();
            ImGui::Unindent();
        }
    }

    ImGui::End();
}

void system_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(camera, ser);
        s.handle_serialise(zoom_level, ser);
        ///deliberately out of order. Not important tremendously but it means that
        ///all the orbital systems will be grouped in the binary file
        s.handle_serialise(systems, ser);
        s.handle_serialise(backup_system, ser);
        s.handle_serialise(currently_viewed, ser);
        s.handle_serialise(hovered_system, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        //s.handle_serialise(camera, ser);
        //s.handle_serialise(zoom_level, ser);
        s.handle_serialise(systems, ser);
        //s.handle_serialise(backup_system, ser);
        //s.handle_serialise(currently_viewed, ser);
        //s.handle_serialise(hovered_system, ser);
    }

    handled_by_client = true;
}

/*void system_manager::ensure_found_orbitals_handled()
{
    for(orbital_system* sys : systems)
    {
        sys->ensure_found_orbitals_handled();
    }
}*/

void system_manager::erase_all()
{
    for(orbital_system* sys : systems)
    {
        for(orbital* o : sys->orbitals)
        {
            delete o;
        }

        delete sys;
    }

    systems.clear();
}
