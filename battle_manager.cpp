#include "battle_manager.hpp"
#include <SFML/Graphics.hpp>

void projectile::load(int type)
{
    using namespace ship_component_elements;

    std::string str = "./pics/";

    if(type == RAILGUN)
    {
        str += "railgun.png";
    }

    if(type == TORPEDO)
    {
        str += "torpedo.png";
    }

    if(type == PLASMAGUN)
    {
        str += "plasmabolt.png";
    }

    if(type == COILGUN)
    {
        str += "coilgun.png";
    }

    img.loadFromFile(str);
    tex.loadFromImage(img);
}

projectile* projectile_manager::make_new()
{
    projectile* p = new projectile;

    projectiles.push_back(p);

    return p;
}

bool projectile_within_ship(projectile* p, ship* s)
{
    //vec2f tl = i->local_pos -

    if(p->pteam == s->team)
        return false;

    vec2f tl = s->local_pos - s->dim/2.f;
    vec2f br = s->local_pos + s->dim/2.f;

    float back_angle = -s->local_rot;

    //tl = (tl - s->local_pos).rot(back_angle) + s->local_pos;
    //br = (br - s->local_pos).rot(back_angle) + s->local_pos;

    vec2f ppos = (p->local_pos - s->local_pos).rot(back_angle) + s->local_pos;

    //printf("%f %f %f %f %f %f\n", ppos.x(),ppos.y(), tl.x(), tl.y(), br.x(), br.y());

    if(ppos.x() < br.x() && ppos.x() >= tl.x() && ppos.y() < br.y() && ppos.y() >= tl.y())
    {
        return true;
    }

    return false;
}

void projectile_manager::tick(battle_manager& manage, float step_s)
{
    //for(auto& i : projectiles)
    for(int kk=0; kk < projectiles.size(); kk++)
    {
        projectile* p = projectiles[kk];

        p->local_pos = p->local_pos + p->velocity * step_s;

        for(auto& i : manage.ships)
        {
            if(projectile_within_ship(p, i.second))
            {
                //printf("hi\n");

                i.second->hit(p);

                destroy(p);
                kk--;
                break;
            }
        }
    }
}

void projectile_manager::destroy(projectile* proj)
{
    for(int i=0; i<(int)projectiles.size(); i++)
    {
        if(projectiles[i] == proj)
        {
            projectiles.erase(projectiles.begin() + i);
            i--;

            delete proj;

            continue;
        }
    }
}

void projectile_manager::draw(sf::RenderWindow& win)
{
    /*sf::RectangleShape rect;

    rect.setFillColor(sf::Color(255, 128, 100));

    for(projectile* p : projectiles)
    {
        rect.setOrigin(p->dim.x()/2, p->dim.y()/2);

        rect.setSize({p->dim.x(), p->dim.y()});

        rect.setPosition({p->local_pos.x(), p->local_pos.y()});

        rect.setRotation(r2d(p->local_rot));

        win.draw(rect);
    }*/

    for(projectile* p : projectiles)
    {
        sf::Sprite spr(p->tex);

        spr.setOrigin(spr.getLocalBounds().width/2, spr.getLocalBounds().height/2);

        spr.setPosition(p->local_pos.x(), p->local_pos.y());

        spr.setRotation(r2d(p->local_rot + M_PI/2));

        win.draw(spr);
    }
}

void tick_ship_ai(battle_manager& battle_manage, ship& s, float step_s)
{
    ship* nearest = battle_manage.get_nearest_hostile(&s);

    vec2f their_pos = nearest->local_pos;
    vec2f my_pos = s.local_pos;

    float max_turn_rate_per_s = step_s * M_PI/16;

    float my_angle = s.local_rot;
    //float their_angle = nearest->local_rot;
    float their_angle_from_me = (nearest->local_pos - s.local_pos).angle();

    float to_them = their_angle_from_me - my_angle;

    float max_change = signum(to_them) * std::min((float)fabs(to_them), max_turn_rate_per_s);

    my_angle += max_change;

    s.local_rot = my_angle;

    //printf("%f angle\n", to_them);
}

void tick_ai(battle_manager& battle_manage, float step_s)
{
    for(auto& i : battle_manage.ships)
    {
        int team = i.first;

        ship& s = *i.second;

        tick_ship_ai(battle_manage, s, step_s);
    }
}

void battle_manager::tick(float step_s)
{
    tick_ai(*this, step_s);

    for(auto& i : ships)
    {
        ship& s = *i.second;

        s.tick_all_components(step_s);

        std::vector<component> fired = s.fire();

        for(auto& i : fired)
        {
            vec2f cpos = s.local_pos;

            projectile* p = projectile_manage.make_new();

            p->local_pos = cpos;
            p->local_rot = s.local_rot;

            p->dim = {3, 2};

            float roffset = randf_s(-0.025f, 0.025f);

            p->local_rot += roffset;

            p->pteam = s.team;

            p->damage = 30;

            float speed = 20.f;

            p->velocity = speed * (vec2f){cos(p->local_rot), sin(p->local_rot)};

            p->load(i.get_weapon_type());
        }
    }

    projectile_manage.tick(*this, step_s);
}

void battle_manager::draw(sf::RenderWindow& win)
{
    projectile_manage.draw(win);

    sf::RectangleShape rect;

    /*float w = 50;
    float h = 20;

    rect.setOrigin(w/2, h/2);

    rect.setSize({w, h});*/

    rect.setFillColor(sf::Color(255, 255, 255));

    for(auto& p : ships)
    {
        ship* s = p.second;

        rect.setOrigin(s->dim.x()/2, s->dim.y()/2);
        rect.setSize({s->dim.x(), s->dim.y()});

        rect.setPosition({s->local_pos.x(), s->local_pos.y()});
        rect.setRotation(r2d(s->local_rot));

        win.draw(rect);
    }
}

void battle_manager::add_ship(ship* s)
{
    ships[s->team] = s;

    vec2f team_positions[2] = {{500, 40}, {500, 400}};

    s->local_pos = team_positions[s->team];

    s->dim = {50, 20};
}

ship* battle_manager::get_nearest_hostile(ship* s)
{
    int team = s->team;

    float min_dist = FLT_MAX;
    ship* found_ship = nullptr;

    for(auto& i : ships)
    {
        if(i.second->team == team)
            continue;

        vec2f my_pos = s->local_pos;
        vec2f their_pos = i.second->local_pos;

        float dist = (my_pos - their_pos).length();

        if(dist < min_dist)
        {
            min_dist = dist;
            found_ship = i.second;
        }
    }

    return found_ship;
}
