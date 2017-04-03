#include "system_manager.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Vertex.hpp>

void orbital_simple_renderable::init(int n, float min_rad, float max_rad)
{
    for(int i=0; i<n; i++)
    {
        vert_dist.push_back(randf_s(min_rad, max_rad));
    }
}

void orbital_simple_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos)
{
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

        vec2f l1 = d1 * (vec2f){cosf(a1), sinf(a1)};
        vec2f l2 = d2 * (vec2f){cosf(a2), sinf(a2)};

        l1 += absolute_pos;
        l2 += absolute_pos;

        sf::RectangleShape shape;

        float width = (l1 - l2).length();
        float height = 1;

        shape.setPosition(l1.x(), l1.y());

        shape.setSize({width, height});

        shape.setRotation(r2d((l2 - l1).angle()));

        #ifdef HOLLOWISH
        if((i % 2) == 0)
            continue;
        #endif

        win.draw(shape);
    }
}

void sprite_renderable::load(const std::string& str)
{
    img.loadFromFile(str);
    tex.loadFromImage(img);
}

void sprite_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos)
{
    sf::Sprite spr(tex);

    spr.setOrigin(spr.getLocalBounds().width/2, spr.getLocalBounds().height/2);
    spr.setPosition(absolute_pos.x(), absolute_pos.y());
    spr.setRotation(r2d(rotation));

    win.draw(spr);
}

void orbital::set_orbit(float ang, float len)
{
    orbital_angle = ang;
    orbital_length = len;

    //angular_velocity_ps = ang_vel_s;
}

void orbital::tick(float step_s)
{
    rotation += rotation_velocity_ps * step_s;

    if(parent == nullptr)
        return;

    double mass_of_sun = 2 * pow(10., 30.);
    double distance_from_earth_to_sun = 149000000000;
    double g_const = 6.674 * pow(10., -11.);

    float mu = mass_of_sun * g_const;

    //double years_per_second =

    double default_scale = (365*24*60*60.) / 50.;

    double scaled_real_dist = (orbital_length / default_scale) * distance_from_earth_to_sun;

    double vspeed = sqrt(mu / scaled_real_dist);

    float calculated_angular_velocity_ps = sqrt(vspeed * vspeed / (scaled_real_dist * scaled_real_dist));

    orbital_angle += calculated_angular_velocity_ps * step_s;

    absolute_pos = orbital_length * (vec2f){cosf(orbital_angle), sinf(orbital_angle)} + parent->absolute_pos;
}

void orbital::draw(sf::RenderWindow& win)
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

    if(render_type == 0)
        simple_renderable.draw(win, rotation, absolute_pos);
    else
        sprite.draw(win, rotation, absolute_pos);
}

void orbital::center_camera(sf::RenderWindow& win)
{
    sf::View view1 = win.getDefaultView();

    view1.setCenter(absolute_pos.x(), absolute_pos.y());

    win.setView(view1);
}

orbital* orbital_system::make_new(orbital_info::type type, float rad)
{
    orbital* n = new orbital;

    n->type = type;
    n->rad = rad;
    //n->simple_renderable.init(10, n->rad * 0.85f, n->rad * 1.2f);

    n->render_type = orbital_info::render_type[type];

    if(n->render_type == 0)
    {
        n->simple_renderable.init(10, n->rad * 0.85f, n->rad * 1.2f);
    }
    else if(n->render_type == 1)
    {
        n->sprite.load(orbital_info::load_strs[type]);
    }

    orbitals.push_back(n);

    return n;
}

void orbital_system::tick(float step_s)
{
    for(auto& i : orbitals)
    {
        i->tick(step_s);
    }
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
            return;
        }
    }
}

///need to figure out higher positioning, but whatever
void orbital_system::draw(sf::RenderWindow& win)
{
    for(auto& i : orbitals)
    {
        i->draw(win);
    }
}

orbital_system* system_manager::make_new()
{
    orbital_system* sys = new orbital_system;

    systems.push_back(sys);

    return sys;
}

void system_manager::tick(float step_s)
{
    for(auto& i : systems)
    {
        i->tick(step_s);
    }
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
