#include "system_manager.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include "ship.hpp"
#include "util.hpp"

void orbital_simple_renderable::init(int n, float min_rad, float max_rad)
{
    for(int i=0; i<n; i++)
    {
        vert_dist.push_back(randf_s(min_rad, max_rad));
    }
}

void orbital_simple_renderable::draw(sf::RenderWindow& win, float rotation, vec2f absolute_pos, vec3f col)
{
    col = col * 255.f;

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

        shape.setFillColor(sf::Color(col.x(), col.y(), col.z()));

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

void do_transfer(orbital* o)
{
    float speed_s = 10;

    vec2f end_pos = o->new_rad * (vec2f){cos(o->new_angle), sin(o->new_angle)};
    vec2f start_pos = o->old_rad * (vec2f){cos(o->old_angle), sin(o->old_angle)};

    float distance = (end_pos - start_pos).length();

    float traverse_time = distance / speed_s;

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

    o->orbital_angle = iangle;
    o->orbital_length = irad;

    if(distance < 2 || frac > 1)
    {
        o->transferring = false;
    }
}

void orbital::tick(float step_s)
{
    internal_time_s += step_s;

    rotation += rotation_velocity_ps * step_s;

    if(transferring)
        do_transfer(this);

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


    if(render_type == 0)
        simple_renderable.draw(win, rotation, absolute_pos, col);
    else if(render_type == 1)
        sprite.draw(win, rotation, absolute_pos, {1, 1, 1}, highlight);

    highlight = false;
}

void orbital::center_camera(sf::RenderWindow& win)
{
    sf::View view1 = win.getDefaultView();

    view1.setCenter(absolute_pos.x(), absolute_pos.y());

    win.setView(view1);
}

bool orbital::point_within(vec2f pos)
{
    vec2f dim = rad * 1.5f;
    vec2f apos = absolute_pos;

    if(pos.x() < apos.x() + dim.x() && pos.x() >= apos.x() - dim.x() && pos.y() < apos.y() + dim.y() && pos.y() >= apos.y() - dim.y())
    {
        return true;
    }

    return false;
}

std::vector<std::string> orbital::get_info_str()
{
    if(type != orbital_info::FLEET || data == nullptr)
    {
        std::string str = "Radius: " + to_string_with_enforced_variable_dp(rad);

        //vec2f rpos = round(absolute_pos * 10.f) / 10.f;

        vec2f rpos = absolute_pos;

        std::string pstr = "Position: " + to_string_with_enforced_variable_dp(rpos.x()) + " " + to_string_with_enforced_variable_dp(rpos.y());

        std::string rstr = "";

        if(is_resource_object)
        {
            rstr = "\n\n" + produced_resources_ps.get_formatted_str();
        }

        return {str + "\n" + pstr + rstr};
    }
    else
    {
        ship_manager* mgr = (ship_manager*)data;

        std::vector<std::string> ret = mgr->get_info_strs();

        //ret.push_back("Position: " + std::to_string(absolute_pos.x()) + " " + std::to_string(absolute_pos.y()));

        return ret;
    }
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

void orbital::make_random_resource_asteroid(float total_ps)
{
    ///[1, 4]
    int num_resources = randf_s(1.f, 5.f);

    for(int i=0; i<num_resources; i++)
    {
        int rand_res = (int)resource::get_random_processed();

        produced_resources_ps.resources[rand_res].amount += randf_s(0.25f, total_ps) * resource::global_resource_multiplier;
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

    for(auto& i : ore_mults)
    {
        produced_resources_ps.resources[i.first].amount += randf_s(0.55f, total_ps) * i.second * resource::global_resource_multiplier;
    }

    is_resource_object = true;
}

bool orbital::can_dispense_resources()
{
    if(is_resource_object || orbital_info::PLANET)
        return true;

    return false;
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
    //n->simple_renderable.init(10, n->rad * 0.85f, n->rad * 1.2f);

    if(type == orbital_info::ASTEROID)
    {
        n->col = {1, 1, 1};
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
    /*for(auto& i : orbitals)
    {
        i->draw(win);
    }*/

    for(int kk=orbitals.size()-1; kk >= 0; kk--)
    {
        orbitals[kk]->draw(win);
    }
}

void orbital_system::cull_empty_orbital_fleets()
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

void orbital_system::generate_asteroids(int n, int num_belts, int num_resource_asteroids)
{
    std::vector<float> exclusion_radiuses;

    for(auto& i : orbitals)
    {
        if(i->type == orbital_info::PLANET)
        {
            exclusion_radiuses.push_back(i->orbital_length);
        }
    }

    float min_belt = 50.f;

    float max_belt = 500.f;

    int asteroids_per_belt = n / num_belts;

    std::vector<orbital*> cur_orbitals;

    for(int i=0; i<num_belts; i++)
    {
        float rad = 0.f;

        bool bad = false;

        int max_tries = 10;
        int cur_tries = 0;

        do
        {
            bad = false;

            rad = randf_s(min_belt, max_belt);

            for(auto& i : exclusion_radiuses)
            {
                if(fabs(rad - i) < 20)
                {
                    bad = true;
                    break;
                }
            }

            cur_tries++;
        }
        while(bad && cur_tries < max_tries);

        exclusion_radiuses.push_back(rad);

        for(int kk=0; kk<asteroids_per_belt; kk++)
        {
            float angle = ((float)kk / (asteroids_per_belt + 1)) * 2 * M_PI;

            float len = rad * randf_s(0.9, 1.1);

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
    if(o1->absolute_pos == o2->absolute_pos)
    {
        o1->orbital_length -= 1;

        return;
    }

    if(o1->transferring || o2->transferring)
        return;

    vec2f o1_to_o2 = (o2->absolute_pos - o1->absolute_pos).norm();

    vec2f new_o1_pos = -o1_to_o2 + o1->absolute_pos;
    vec2f new_o2_pos = o1_to_o2 + o2->absolute_pos;

    o1->transfer(new_o1_pos);
    o2->transfer(new_o2_pos);
}

void system_manager::repulse_fleets()
{
    for(orbital_system* sys : systems)
    {
        for(orbital* o : sys->orbitals)
        {
            for(orbital* k : sys->orbitals)
            {
                if(k == o)
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

void system_manager::cull_empty_orbital_fleets()
{
    for(auto& i : systems)
    {
        i->cull_empty_orbital_fleets();
    }
}