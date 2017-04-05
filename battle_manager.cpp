#include "battle_manager.hpp"
#include <SFML/Graphics.hpp>
#include "system_manager.hpp"
#include "empire.hpp"

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

    tex.setSmooth(true);
}

projectile* projectile_manager::make_new()
{
    projectile* p = new projectile;

    projectiles.push_back(p);

    return p;
}

bool point_within_ship(vec2f pos, ship* s)
{
    vec2f tl = s->local_pos - s->dim/2.f;
    vec2f br = s->local_pos + s->dim/2.f;

    float back_angle = -s->local_rot;

    //tl = (tl - s->local_pos).rot(back_angle) + s->local_pos;
    //br = (br - s->local_pos).rot(back_angle) + s->local_pos;

    vec2f ppos = (pos - s->local_pos).rot(back_angle) + s->local_pos;

    //printf("%f %f %f %f %f %f\n", ppos.x(),ppos.y(), tl.x(), tl.y(), br.x(), br.y());

    if(ppos.x() < br.x() && ppos.x() >= tl.x() && ppos.y() < br.y() && ppos.y() >= tl.y())
    {
        return true;
    }

    return false;
}

bool projectile_within_ship(projectile* p, ship* s)
{
    //vec2f tl = i->local_pos -

    if(p->pteam == s->team)
        return false;

    return point_within_ship(p->local_pos, s);
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
            bool term = false;

            std::vector<ship*>& slist = i.second;

            for(ship* found_ship : slist)
            {
                if(projectile_within_ship(p, found_ship))
                {
                    //printf("hi\n");

                    found_ship->hit(p);

                    destroy(p);
                    kk--;
                    term = true;
                    break;
                }
            }

            if(term)
                break;
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

void projectile_manager::destroy_all()
{
    for(auto& i : projectiles)
    {
        delete i;
    }

    projectiles.clear();
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

        float scale = 1.f;

        if(p->base.has_tag(component_tag::SCALE))
            scale = p->base.get_tag(component_tag::SCALE);

        spr.setScale(scale, scale);

        win.draw(spr);
    }
}

void tick_ship_ai(battle_manager& battle_manage, ship& s, float step_s)
{
    ship* nearest = battle_manage.get_nearest_hostile(&s);

    //vec2f their_pos = nearest->local_pos;
    //vec2f my_pos = s.local_pos;

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

        std::vector<ship*> slist = i.second;

        for(ship* s : slist)
            tick_ship_ai(battle_manage, *s, step_s);
    }
}

void battle_manager::tick(float step_s)
{
    tick_ai(*this, step_s);

    for(auto& i : ships)
    {
        for(auto& s : i.second)
        {
            std::vector<component> fired = s->fire();

            for(component& kk : fired)
            {
                vec2f cpos = s->local_pos;

                projectile* p = projectile_manage.make_new();

                p->local_pos = cpos;
                p->local_rot = s->local_rot;

                p->dim = {3, 2};

                float roffset = randf_s(-0.025f, 0.025f);

                p->local_rot += roffset;

                p->pteam = s->team;

                p->base = kk;

                float speed = kk.get_tag(component_tag::SPEED);

                p->velocity = speed * (vec2f){cos(p->local_rot), sin(p->local_rot)};

                p->load(kk.get_weapon_type());
            }
        }
        //ship& s = *i.second;

        //s.tick_all_components(step_s);

    }

    projectile_manage.tick(*this, step_s);
}

void battle_manager::draw(sf::RenderWindow& win)
{
    projectile_manage.draw(win);

    //sf::RectangleShape rect;

    /*float w = 50;
    float h = 20;

    rect.setOrigin(w/2, h/2);

    rect.setSize({w, h});*/

    /*rect.setFillColor(sf::Color(255, 255, 255));

    for(auto& p : ships)
    {
        ship* s = p.second;

        rect.setOrigin(s->dim.x()/2, s->dim.y()/2);
        rect.setSize({s->dim.x(), s->dim.y()});

        rect.setPosition({s->local_pos.x(), s->local_pos.y()});
        rect.setRotation(r2d(s->local_rot));

        win.draw(rect);
    }*/

    std::vector<ship*> linear_vec;

    for(auto& p : ships)
    {
        for(ship* s : p.second)
        {
            linear_vec.push_back(s);
        }
    }

    for(int i = linear_vec.size()-1; i >= 0; i--)
    {
        ship* s = linear_vec[i];

        sf::Texture& tex = s->tex;

        sf::Sprite spr(tex);

        spr.setOrigin(spr.getLocalBounds().width/2, spr.getLocalBounds().height/2);
        spr.setPosition({s->local_pos.x(), s->local_pos.y()});
        spr.setRotation(r2d(s->local_rot));

        if(s->highlight)
        {
            spr.setColor(sf::Color(0, 128, 255));

            int hw = 2;

            for(int y=-hw; y<=hw; y++)
            {
                for(int x=-hw; x<=hw; x++)
                {
                    spr.setPosition(s->local_pos.x() + x, s->local_pos.y() + y);

                    win.draw(spr);
                }
            }

            spr.setColor(sf::Color(255, 255, 255));

            spr.setPosition(s->local_pos.x(), s->local_pos.y());
        }

        win.draw(spr);

        s->highlight = false;
    }
}

void battle_manager::add_ship(ship* s)
{
    for(auto& i : ships)
    {
        for(auto& sh : i.second)
        {
            if(s == sh)
                return;
        }
    }

    s->enter_combat();

    int prev_num = ships[s->team].size();

    ships[s->team].push_back(s);

    vec2f team_positions[2] = {{500, 40}, {500, 400}};

    s->local_pos = team_positions[s->team];

    s->local_pos.x() += prev_num * 50;

    s->dim = {100, 40};

    s->check_load({s->dim.x(), s->dim.y()});
}

ship* battle_manager::get_nearest_hostile(ship* s)
{
    int team = s->team;

    float min_dist = FLT_MAX;
    ship* found_ship = nullptr;

    for(auto& i : ships)
    {
        for(ship* os : i.second)
        {
            if(os->team == team)
                continue;

            vec2f my_pos = s->local_pos;
            vec2f their_pos = os->local_pos;

            float dist = (my_pos - their_pos).length();

            if(dist < min_dist)
            {
                min_dist = dist;
                found_ship = os;
            }
        }
    }

    return found_ship;
}

ship* battle_manager::get_ship_under(vec2f pos)
{
    for(auto& i : ships)
    {
        for(auto& kk : i.second)
        {
            if(point_within_ship(pos, kk))
                return kk;
        }
    }

    return nullptr;
}

void battle_manager::set_view(system_manager& system_manage)
{
    vec2f avg = {0,0};
    int num = 0;

    for(auto& i : ships)
    {
        for(auto& kk : i.second)
        {
            avg += kk->local_pos;

            num++;
        }
    }

    if(num > 0)
        avg = avg / num;

    /*sf::View v1 = win.getDefaultView();
    v1.setCenter(avg.x(), avg.y());

    win.setView(v1);*/

    system_manage.camera = avg;
}

bool battle_manager::can_disengage(empire* disengaging_empire)
{
    return true;
}

void battle_manager::do_disengage(empire* disengaging_empire)
{
    for(auto& i : ships)
    {
        for(ship* s : i.second)
        {
            s->leave_combat();

            ///nobody initiated this, ie we're leaving combat in an acceptable way
            if(disengaging_empire == nullptr)
                continue;

            if(s->team != disengaging_empire->team_id)
                continue;

            s->apply_disengage_penalty();
        }
    }
}

bool battle_manager::any_in_fleet_involved(ship_manager* sm)
{
    for(auto& i : ships)
    {
        for(ship* s : i.second)
        {
            for(ship* s2 : sm->ships)
            {
                if(s == s2)
                    return true;
            }
        }
    }

    return false;
}

void battle_manager::destructive_merge_into_me(battle_manager* bm)
{
    for(auto& i : bm->projectile_manage.projectiles)
    {
        projectile_manage.projectiles.push_back(i);
    }

    for(auto& i : bm->ships)
    {
        for(auto& sh : i.second)
        {
            add_ship(sh);
        }
    }
}

battle_manager* all_battles_manager::make_new()
{
    battle_manager* bm = new battle_manager;

    battles.push_back(bm);

    return bm;
}

void all_battles_manager::destroy(battle_manager* bm)
{
    for(int i=0; i<battles.size(); i++)
    {
        if(battles[i] == bm)
        {
            delete bm;
            battles.erase(battles.begin() + i);
            i--;

            return;
        }
    }
}

void all_battles_manager::tick(float step_s)
{
    for(auto& i : battles)
    {
        i->tick(step_s);
    }

    if(currently_viewing == nullptr && battles.size() > 0)
    {
        currently_viewing = battles[0];
    }
}

void all_battles_manager::draw_viewing(sf::RenderWindow& win)
{
    if(currently_viewing == nullptr)
        return;

    currently_viewing->draw(win);
}

void all_battles_manager::set_viewing(battle_manager* bm, system_manager& system_manage, bool jump)
{
    currently_viewing = bm;

    if(bm == nullptr)
        return;

    if(jump)
        bm->set_view(system_manage);
}

battle_manager* all_battles_manager::make_new_battle(std::vector<orbital*> t1)
{
    battle_manager* bm = make_new();

    //for(battle_manager* bfind : battles)
    for(int i=0; i<battles.size(); i++)
    {
        battle_manager* bfind = battles[i];

        bool term = false;

        for(orbital* o : t1)
        {
            if(o->type != orbital_info::FLEET)
                continue;

            ship_manager* sm = (ship_manager*)o->data;

            if(bfind->any_in_fleet_involved(sm))
            {
                bm->destructive_merge_into_me(bfind);

                destroy(bfind);

                i--;

                term = true;

                break;
            }
        }

        if(term)
            break;
    }


    int nships = 0;

    for(orbital* o : t1)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        for(ship* s : sm->ships)
        {
            bm->add_ship(s);

            nships++;
        }
    }

    //printf("made new battle with %i %i\n", t1.size(), nships);
}

void all_battles_manager::disengage(battle_manager* bm, empire* disengaging_empire)
{
    bm->do_disengage(disengaging_empire);

    bm->projectile_manage.destroy_all();

    if(bm == currently_viewing)
        currently_viewing = nullptr;

    destroy(bm);
}
