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

int orbital::gid;
int orbital_system::gid;

float system_manager::universe_scale = 100.f;

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

    if(e1.type != e2.type);
    {
        return e1.type < e2.type;
    }

    return e1.id < e2.id;
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

void orbital_simple_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, bool force_high_quality, bool draw_outline, const std::string& tag, vec3f col)
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

    if(draw_outline)
    {
        ImGui::SetNextWindowPos(ImVec2(real_coord.x + pixel_rad.x, real_coord.y));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.1));

        ImGui::Begin(tag.c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW);

        ImGui::Text(tag.c_str());

        ImGui::End();

        ImGui::PopStyleColor();
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

        float d1 = vert_dist[cur] * scale;
        float d2 = vert_dist[next] * scale;

        float a1 = ((float)cur / (vert_dist.size())) * 2 * M_PI;
        float a2 = ((float)next / (vert_dist.size())) * 2 * M_PI;

        a1 += rotation;
        a2 += rotation;

        vec2f l1 = d1 * (vec2f){cosf(a1), sinf(a1)};
        vec2f l2 = d2 * (vec2f){cosf(a2), sinf(a2)};

        l1 += absolute_pos;
        l2 += absolute_pos;

        vec2f perp = perpendicular((l2 - l1).norm());

        sf::Vertex v[4];

        v[0].position = sf::Vector2f(l1.x(), l1.y());
        v[1].position = sf::Vector2f(l2.x(), l2.y());
        v[2].position = sf::Vector2f(l2.x() + perp.x(), l2.y() + perp.y());
        v[3].position = sf::Vector2f(l1.x() + perp.x(), l1.y() + perp.y());

        sf::Color scol = sf::Color(col.x(), col.y(), col.z());

        v[0].color = scol;
        v[1].color = scol;
        v[2].color = scol;
        v[3].color = scol;

        win.draw(v, 4, sf::Quads);
    }
    #endif
}

void sprite_renderable::load(const std::string& str)
{
    img.loadFromFile(str);
    tex.loadFromImage(img);
    //tex.setSmooth(true);
    ///smooth rendering causes issues, we'll have to go high res or constructive
    ///also looks hideous
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

    win.draw(spr);
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

float get_length_circle(vec2f p1, vec2f p2)
{
    float max_rad = std::max(p1.length(), p2.length());

    p1 = p1.norm() * max_rad;
    p2 = p2.norm() * max_rad;

    //float a = (p1 - p2).length();

    float A = acos(clamp(dot(p1.norm(), p2.norm()), -1.f, 1.f));

    //float r = sqrt(a*a / (2 * (1 - cos(A))));
    ///fuck we know r its just p1.length()

    float r = p1.length();

    return fabs(r * A);
}

void do_transfer(orbital* o, float diff_s)
{
    ///doesn't do speed, ignore me as a concept
    //float speed_s_old = 30;

    vec2f end_pos = o->new_rad * (vec2f){cos(o->new_angle), sin(o->new_angle)};
    //vec2f start_pos = o->old_rad * (vec2f){cos(o->old_angle), sin(o->old_angle)};

    //float distance_lin = (end_pos - start_pos).length();

    /*float linear_extra = fabs(end_pos.length() - start_pos.length());
    float radius_extra = get_length_circle(start_pos, end_pos);

    float dist = sqrt(linear_extra * linear_extra + radius_extra*radius_extra);

    float traverse_time = dist / speed_s_old;

    ///nope. Ok need to work out angular distance on a circle between two points

    float frac = (o->internal_time_s - o->start_time_s) / traverse_time;

    float a1 = start_pos.angle();
    float a2 = end_pos.angle();

    if(fabs(a2 + 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 += 2*M_PI;
    }

    if(fabs(a2 - 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 -= 2*M_PI;
    }

    float iangle = o->old_angle + frac * (a2 - a1);
    float irad = o->new_rad * frac + o->old_rad * (1.f - frac);

    vec2f calculated_next = irad * (vec2f){cos(iangle), sin(iangle)};
    vec2f calculated_cur = o->orbital_length * (vec2f){cos(o->orbital_angle), sin(o->orbital_angle)};*/

    float straightline_distance = (end_pos - o->absolute_pos).length();

    float linear_extra = fabs(end_pos.length() - o->absolute_pos.length());
    float radius_extra = get_length_circle(o->absolute_pos, end_pos);

    float dist = sqrt(linear_extra * linear_extra + radius_extra*radius_extra);

    float a1 = o->absolute_pos.angle();
    float a2 = end_pos.angle();

    if(fabs(a2 + 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 += 2*M_PI;
    }

    if(fabs(a2 - 2 * M_PI - a1) < fabs(a2 - a1))
    {
        a2 -= 2*M_PI;
    }

    float cur_angle = (o->absolute_pos - o->parent->absolute_pos).angle();
    float cur_rad = (o->absolute_pos - o->parent->absolute_pos).length();

    float iangle = cur_angle + diff_s * (a2 - a1);
    float irad = cur_rad + diff_s * (o->new_rad - cur_rad);

    vec2f calculated_next = irad * (vec2f){cos(iangle), sin(iangle)};
    vec2f calculated_cur = o->orbital_length * (vec2f){cos(o->orbital_angle), sin(o->orbital_angle)};

    float speed = 5.f;

    ///this is the real speed here
    vec2f calc_dir = (calculated_next - calculated_cur).norm() / speed;

    vec2f calc_real_next = calculated_cur + calc_dir;

    iangle = calc_real_next.angle();
    irad = calc_real_next.length();

    o->orbital_angle = iangle;
    o->orbital_length = irad;

    if(straightline_distance < 2)
    {
        o->transferring = false;
    }
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

void orbital::tick(float step_s)
{
    internal_time_s += step_s;

    rotation += rotation_velocity_ps * step_s;

    if(transferring)
        do_transfer(this, step_s);

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

    double mass_of_sun = 2 * pow(10., 30.);
    double distance_from_earth_to_sun = 149000000000;
    double g_const = 6.674 * pow(10., -11.);

    float mu = mass_of_sun * g_const;

    //double years_per_second =

    double default_scale = (365*24*60*60.) / 50.;

    double scaled_real_dist = (orbital_length / default_scale) * distance_from_earth_to_sun;

    double vspeed_sq = mu / scaled_real_dist;

    float calculated_angular_velocity_ps = sqrtf(vspeed_sq / (scaled_real_dist * scaled_real_dist));

    if(!transferring)
        orbital_angle += calculated_angular_velocity_ps * step_s;

    //if(orbital_angle >= M_PIf)
    //    orbital_angle -= 2*M_PIf;

    absolute_pos = orbital_length * (vec2f){impl_cos(orbital_angle), impl_sin(orbital_angle)} + parent->absolute_pos;
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

    bool currently_has_vision = viewer_empire->has_vision(parent_system);

    bool not_currently_viewed_fleet = false;

    if(currently_has_vision || type != orbital_info::FLEET)
    {
        last_viewed_position = absolute_pos;
    }

    if(currently_has_vision)
    {
        viewed_by[viewer_empire] = true;
    }

    if(!currently_has_vision && type == orbital_info::FLEET)
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

    if(render_type == 0)
        simple_renderable.draw(win, rotation, last_viewed_position, force_high_quality, type == orbital_info::PLANET, name, current_simple_col);
    if(render_type == 1)
        sprite.draw(win, rotation, last_viewed_position, current_sprite_col, highlight);

    if(highlight && type == orbital_info::FLEET)
    {
        auto real_coord = mapCoordsToPixel_float(last_viewed_position.x(), last_viewed_position.y(), win.getView(), win);

        ImGui::SetNextWindowPos(ImVec2(round(real_coord.x + get_pixel_radius(win)), round(real_coord.y)));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0.1));

        ImGui::Begin((get_name_with_info_warfare(viewer_empire) + "###" + name + std::to_string(unique_id)).c_str(), nullptr, IMGUI_JUST_TEXT_WINDOW);

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

std::vector<std::string> orbital::get_info_str(empire* viewer_empire, bool use_info_warfare)
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
            ret = mgr->get_info_strs_with_info_warfare(viewer_empire, this);
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

void orbital::transfer(float pnew_rad, float pnew_angle)
{
    old_rad = orbital_length;
    old_angle = orbital_angle;
    new_rad = pnew_rad;
    new_angle = pnew_angle;

    transferring = true;

    start_time_s = internal_time_s;
}

void orbital::transfer(vec2f pos)
{
    vec2f base;

    if(parent)
        base = parent->absolute_pos;

    vec2f rel = pos - base;

    transfer(rel.length(), rel.angle());
}

void orbital::request_transfer(vec2f pos)
{
    if(type != orbital_info::FLEET)
        return transfer(pos);

    ship_manager* sm = (ship_manager*)data;

    if(sm->any_in_combat())
        return;

    if(sm->can_move_in_system())
        transfer(pos);
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

    if(rad < 2.3)
    {
        simple_renderable.init(simple_renderable.vert_dist.size(), 2.3, 2.3);
        rad = 2.3;
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

void orbital::draw_alerts(sf::RenderWindow& win, empire* viewing_empire, system_manager& system_manage)
{
    if(parent_empire != viewing_empire)
        return;

    if(parent_system != system_manage.currently_viewed)
        return;

    if(type != orbital_info::FLEET)
    {
        if(has_quest_alert)
        {
            text_manager::render(win, "?", absolute_pos + (vec2f){8, -20}, {0.3, 1.0, 0.3});
        }

        return;
    }

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

        n->description = generator.generate_star_text(n);
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

    if(n->render_type == 0)
    {
        n->simple_renderable.init(num_verts, n->rad * 0.85f, n->rad * 1.2f);
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

    return n;
}

void orbital_system::tick(float step_s, orbital_system* viewed)
{
    for(auto& i : orbitals)
    {
        i->tick(step_s);
    }

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
    for(int i=0; i<(int)orbitals.size(); i++)
    {
        if(orbitals[i] == o)
        {
            orbitals.erase(orbitals.begin() + i);
            delete o;
            i--;
            break;
        }
    }

    for(int i=0; i<(int)asteroids.size(); i++)
    {
        if(asteroids[i] == o)
        {
            asteroids.erase(asteroids.begin() + i);
            delete o;
            i--;
            break;
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

void orbital_system::cull_empty_orbital_fleets(empire_manager& empire_manage)
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

void orbital_system::generate_full_random_system()
{
    int min_planets = 0;
    int max_planets = 5;

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

system_manager::system_manager()
{
    fleet_tex.loadFromFile(orbital_info::load_strs[orbital_info::FLEET]);
    fleet_sprite.setTexture(fleet_tex);
}

orbital_system* system_manager::make_new()
{
    orbital_system* sys = new orbital_system;

    systems.push_back(sys);

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

    if(o1->transferring || o2->transferring)
        return;

    vec2f o1_to_o2 = (o2->absolute_pos - o1->absolute_pos);

    if(approx_equal(o1_to_o2, (vec2f){0.f, 0}, 0.001f))
    {
        o1_to_o2 = {1, 0};
    }

    vec2f new_o1_pos = -o1_to_o2.norm() + o1->absolute_pos;
    vec2f new_o2_pos = o1_to_o2.norm() + o2->absolute_pos;

    o1->transfer(new_o1_pos);
    o2->transfer(new_o2_pos);
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

void system_manager::cull_empty_orbital_fleets(empire_manager& empire_manage)
{
    for(auto& i : systems)
    {
        i->cull_empty_orbital_fleets(empire_manage);
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

std::pair<vec2f, vec2f> get_intersection(vec2f p1, vec2f p2, float r)
{
    vec2f avg = (p1 + p2) / 2.f;

    float r1_len = (avg - p1).length();

    float olen = sqrt(std::max(r*r - r1_len*r1_len, 0.f));

    vec2f o2dir = (p1 - avg).norm();

    o2dir = o2dir.rot(M_PI/2) * olen/2.f;

    return {avg - o2dir * 1.02f, avg + o2dir * 1.02f};
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

void system_manager::draw_universe_map(sf::RenderWindow& win, empire* viewer_empire, popup_info& popup)
{
    //printf("zoom %f\n", zoom_level);

    suppress_click_away_fleet = false;
    hovered_orbitals.clear();
    advertised_universe_orbitals.clear();

    if(in_system_view())
        return;

    sf::BlendMode blend(sf::BlendMode::DstAlpha, sf::BlendMode::DstAlpha, sf::BlendMode::Subtract);

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

    float frad = sun_universe_rad * 6.5f;

    for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        if(os->get_base()->parent_empire != nullptr)
        {
            vec3f col = os->get_base()->parent_empire->colour * 255.f;

            sf::CircleShape ncircle;

            //ncircle.setFillColor({20, 20, 20, 255});
            ncircle.setFillColor({0,0,0,255});
            ncircle.setOutlineColor(sf::Color(col.x(), col.y(), col.z()));

            ncircle.setRadius(frad);

            ncircle.setOrigin(ncircle.getLocalBounds().width/2, ncircle.getLocalBounds().height/2);

            ncircle.setOutlineThickness(sun_universe_rad / 15.f);
            ncircle.setPosition({pos.x(), pos.y()});

            ncircle.setPointCount(100.f);

            win.draw(ncircle, blend);
        }
    }

    //sf::BlendMode b2(sf::BlendMode::SrcColor, sf::BlendMode::DstColor, sf::BlendMode::Add);

    for(orbital_system* os : systems)
    {
        vec2f pos = os->universe_pos * universe_scale;

        if(os->get_base()->parent_empire != nullptr)
        {
            vec3f col = os->get_base()->parent_empire->colour * 255.f;

            sf::CircleShape ncircle;

            ncircle.setFillColor({20, 20, 20, 255});
            ncircle.setOutlineColor(sf::Color(col.x(), col.y(), col.z()));

            ncircle.setRadius(frad);

            ncircle.setOrigin(ncircle.getLocalBounds().width/2, ncircle.getLocalBounds().height/2);

            //ncircle.setOutlineThickness(sun_universe_rad / 15.f);
            ncircle.setPosition({pos.x(), pos.y()});

            ncircle.setPointCount(100.f);

            ///change to add if we want to see overlap
            win.draw(ncircle);
        }
    }

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

            sf::RectangleShape shape;

            //shape.setSize({400.f, 400.f});
            shape.setSize({(p2 - p1).length(), sun_universe_rad/15.f});
            shape.setRotation(r2d((p2 - p1).angle()));

            shape.setOrigin(shape.getLocalBounds().width/2, shape.getLocalBounds().height/2);

            shape.setPosition(p1.x(), p1.y());

            vec3f colour = (o1->get_base()->parent_empire->colour + o2->get_base()->parent_empire->colour)/2.f;

            colour = colour * 255.f;

            shape.setFillColor({colour.x(), colour.y(), colour.z()});

            win.draw(shape);

            /*shape.setPosition(p2.x(), p2.y());

            win.draw(shape);*/
        }
    }


    for(int i=0; i<systems.size(); i++)
    {
        orbital_system* os = systems[i];


        vec2f pos = os->universe_pos * universe_scale;

        sf::CircleShape circle;

        circle.setPosition({pos.x(), pos.y()});
        circle.setFillColor(sf::Color(255, 200, 50));

        if(os->highlight)
        {
            circle.setFillColor(sf::Color(255, 255, 150));
        }

        circle.setRadius(sun_universe_rad);
        circle.setOrigin(circle.getLocalBounds().width/2, circle.getLocalBounds().height/2);

        win.draw(circle);

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

        vec2f fleet_draw_pos = pos + (vec2f){sun_universe_rad, 0.f};
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

void system_manager::generate_universe(int num)
{
    float exclusion_rad = 20.f;

    for(int i=0; i<num; i++)
    {
        orbital_system* sys = make_new();
        sys->generate_full_random_system();

        float random_angle = randf_s(0.f, 2*M_PI);

        float random_radius = randf_s(0.f, 1.f) * randf_s(0.f, 1.f) * 500;

        vec2f random_position = random_radius * (vec2f){cosf(random_angle), sinf(random_angle)};

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

            //random_position = randv<2, float>(-200.f, 200.f);

            random_angle = randf_s(0.f, 2*M_PI);
            random_radius = randf_s(0.f, 1.f) * randf_s(0.f, 1.f) * 500;

            random_position = random_radius * (vec2f){cosf(random_angle), sinf(random_angle)};

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

    ImGui::Begin("Fleets", &top_bar::active[top_bar_info::FLEETS], ImGuiWindowFlags_AlwaysAutoResize | IMGUI_WINDOW_FLAGS);

    std::map<empire_popup, std::vector<orbital_system*>> empire_to_systems;

    for(orbital_system* os : systems)
    {
        if(os->get_base()->parent_empire == nullptr)
            continue;

        empire_popup pop;
        pop.e = os->get_base()->parent_empire;
        pop.id = os->unique_id;
        pop.hidden = os->get_base()->parent_empire == nullptr; ///currently always false
        pop.type = orbital_info::NONE;
        pop.is_player = os->get_base()->parent_empire == viewing_empire;

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

            if(ship_map.size() == 0)
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

            ImGui::Text("(goto)");

            if(ImGui::IsItemClicked_Registered())
            {
                set_viewed_system(sys);
            }

            int remove_len = sys_name.length();

            if(viewing)
            {
                std::string view_str = "(viewing)";

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

                    if(viewing_empire != e)
                        tooltip::add(relations_str + " (" + viewing_empire->get_relations_string(e) + ")");
                    else
                        tooltip::add("(Your Empire)");
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
