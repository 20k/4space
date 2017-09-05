#include "battle_manager.hpp"
#include <SFML/Graphics.hpp>
#include "system_manager.hpp"
#include "empire.hpp"
#include "util.hpp"
#include <set>

uint32_t battle_manager::gid = 0;

void tonemap(sf::Image& image, tonemap_options options)
{
    auto dim = image.getSize();

    vec2f image_center_bias = {0.0f, -0.2f};

    vec2f bias_factor = image_center_bias * (vec2f){dim.x, dim.y};

    vec2f center = {(dim.x - 1)/2.f, (dim.y - 1)/2.f};

    float radius = center.x();

    for(int y=0; y<dim.y; y++)
    {
        for(int x=0; x<dim.x; x++)
        {
            vec2f pos = {x, y};

            sf::Color col = image.getPixel(x, y);

            //float intensity = col.a / 255.f;

            vec2f vintensity = (pos - center) / (vec2f){radius, radius};
            float intensity = 1.f - vintensity.length();

            vec2f vbiased_intensity = (pos - center - bias_factor) / (vec2f){radius, radius};
            float biased_intensity = 1.f - vbiased_intensity.length();

            intensity = clamp(intensity, 0.f, 1.f);
            biased_intensity = clamp(biased_intensity, 0.f, 1.f);

            intensity = mix(intensity, biased_intensity, intensity);

            intensity = pow(intensity, 0.15f);

            vec3f intensity_weights = {1.f, 1.f, 1.f};

            vec3f power_weights = options.power_weights;
            //vec3f power_weights = {4, 4, 0.5};

            vec3f test_col = mix((vec3f){0,0,0}, (vec3f){1,1,1}, intensity * intensity_weights);

            test_col = pow(test_col, power_weights);

            sf::Color ncol(test_col.x() * 255, test_col.y() * 255, test_col.z() * 255, intensity * 255);

            image.setPixel(x, y, ncol);
        }
    }
}

void premultiply(sf::Image& image)
{
    auto dim = image.getSize();

    for(int y=0; y<dim.y; y++)
    {
        for(int x=0; x<dim.x; x++)
        {
            sf::Color col = image.getPixel(x, y);

            col.r *= col.a / 255.f;
            col.g *= col.a / 255.f;
            col.b *= col.a / 255.f;

            image.setPixel(x, y, col);
        }
    }
}

void projectile::load(int _type)
{
    using namespace ship_component_elements;

    type = _type;

    std::string str = "./pics/";

    bool experimental_particle = false;
    tonemap_options tone_options;

    if(type == RAILGUN)
    {
        //str += "railgun.png";
        str += "particle_base.png";

        experimental_particle = true;

        tone_options.power_weights = {4, 4, 0.5};

        options.scale = {1, 5};
        options.overall_scale = 1/5.f;
        options.blur = true;
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

    if(experimental_particle)
        tonemap(img, tone_options);
    else
        premultiply(img);

    tex.loadFromImage(img);

    tex.setSmooth(true);
}

void projectile::do_serialise(serialise& s, bool ser)
{
    ///ok so. Assume that disk mode is called
    ///whenever an item needs full sync
    ///and non disk mode is called per 'tick'
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(ship_fired_by, ser);
        s.handle_serialise(fired_by, ser);
        s.handle_serialise(base, ser);
        s.handle_serialise(pteam, ser);
        s.handle_serialise(velocity, ser);
        s.handle_serialise(type, ser);
        s.handle_serialise(options, ser);

        s.handle_serialise(dim, ser);
        s.handle_serialise(local_rot, ser);
        s.handle_serialise(local_pos, ser);
        s.handle_serialise(world_rot, ser);
        s.handle_serialise(world_pos, ser);

        if(handled_by_client == false)
        {
            load(type);
        }
    }

    if(serialise_data_helper::send_mode == 0)
    {
        s.handle_serialise(ship_fired_by, ser);
        s.handle_serialise(fired_by, ser);
        s.handle_serialise(base, ser);
        s.handle_serialise(pteam, ser);
        s.handle_serialise(velocity, ser);
        s.handle_serialise(type, ser);
        s.handle_serialise(options, ser);

        s.handle_serialise(dim, ser);
        s.handle_serialise(local_rot, ser);
        s.handle_serialise(local_pos, ser);
        s.handle_serialise(world_rot, ser);
        s.handle_serialise(world_pos, ser);

        if(handled_by_client == false)
        {
            load(type);
        }
    }

    handled_by_client = true;
}

projectile* projectile_manager::make_new()
{
    projectile* p = new projectile;

    projectiles.push_back(p);

    p->make_dirty();

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

void projectile_manager::tick(battle_manager& manage, float step_s, system_manager& system_manage)
{
    //for(auto& i : projectiles)
    for(int kk=0; kk < projectiles.size(); kk++)
    {
        projectile* p = projectiles[kk];

        p->local_pos = p->local_pos + p->velocity * step_s;

        /*for(auto& i : manage.ships)
        {
            bool term = false;

            std::vector<ship*>& slist = i.second;

            for(ship* found_ship : slist)
            {
                if(projectile_within_ship(p, found_ship))
                {
                    auto fully_merged = found_ship->get_fully_merged(1.f);

                    bool hp_condition = fully_merged[ship_component_element::HP].cur_amount < 1.f && fully_merged[ship_component_element::HP].max_amount > 1.f;

                    if(hp_condition)
                        continue;

                    found_ship->hit(p, system_manage);

                    destroy(p);
                    kk--;
                    term = true;
                    break;
                }
            }

            if(term)
                break;
        }*/

        for(orbital* o : manage.ship_map)
        {
            bool term = false;

            for(ship* found_ship : o->data->ships)
            {
                if(projectile_within_ship(p, found_ship))
                {
                    auto fully_merged = found_ship->get_fully_merged(1.f);

                    bool hp_condition = fully_merged[ship_component_element::HP].cur_amount < 1.f && fully_merged[ship_component_element::HP].max_amount > 1.f;

                    if(hp_condition)
                        continue;

                    found_ship->hit(p, system_manage);

                    ///problematic for network, defer calls
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
    static sf::Shader shader;
    static bool has_shader;

    if(!has_shader)
    {
        shader.loadFromFile("gauss.fglsl", sf::Shader::Type::Fragment);
        shader.setUniform("texture", sf::Shader::CurrentTexture);

        has_shader = true;
    }

    sf::RenderStates states;
    states.shader = &shader;

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

    sf::BlendMode mode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add);
    states.blendMode = mode;

    for(projectile* p : projectiles)
    {
        sf::Sprite spr(p->tex);

        spr.setOrigin(spr.getLocalBounds().width/2, spr.getLocalBounds().height/2);

        spr.setPosition(p->local_pos.x(), p->local_pos.y());

        spr.setRotation(r2d(p->local_rot + M_PI/2));

        float scale = 1.f;

        if(p->base.has_tag(component_tag::SCALE))
            scale = p->base.get_tag(component_tag::SCALE);

        spr.setScale(scale * p->options.scale.x() * p->options.overall_scale, scale * p->options.scale.y() * p->options.overall_scale);

        if(!p->options.blur)
            win.draw(spr, mode);
        else
            win.draw(spr, states);
    }
}

void projectile_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(projectiles, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        s.handle_serialise(projectiles, ser);
    }
}

void tick_ship_ai(battle_manager& battle_manage, ship& s, float step_s)
{
    ship* nearest = battle_manage.get_nearest_hostile(&s);

    ///this is what caused the crash if no hostiles
    if(nearest == nullptr)
        return;

    if(s.fully_disabled())
        return;

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
    for(orbital* o : battle_manage.ship_map)
    {
        for(ship* s : o->data->ships)
        {
            tick_ship_ai(battle_manage, *s, step_s);
        }
    }
}

void battle_manager::keep_fleets_together(system_manager& system_manage)
{
    /*std::set<ship_manager*> fleets;

    for(auto& i : ships)
    {
        for(ship* s : i.second)
        {
            fleets.insert(s->owned_by);
        }
    }

    std::vector<orbital*> orbitals;

    for(ship_manager* sm : fleets)
    {
        orbitals.push_back(system_manage.get_by_element_orbital((void*)sm));
    }*/

    vec2f avg = {0,0};
    int avg_num = 0;

    int sm1 = 0;

    for(orbital* o : ship_map)
    {
        avg += o->absolute_pos;
        avg_num ++;
    }

    if(avg_num == 0)
        return;

    avg = avg / (float)avg_num;

    for(orbital* o : ship_map)
    {
        vec2f m_pos = o->absolute_pos;

        float dist = (o->absolute_pos - avg).length();

        if(dist < 40.f)
            continue;

        o->transfer(avg, o->parent_system, false, true);
    }
}

void battle_manager::tick(float step_s, system_manager& system_manage)
{
    tick_ai(*this, step_s);

    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            s->tick_combat(step_s);

            if(get_nearest_hostile(s) == nullptr)
                continue;

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

                p->fired_by = s->owned_by->parent_empire;
                p->ship_fired_by = s;
            }
        }
    }

    uint32_t keep_frames = 10;

    if(((frame_counter + unique_battle_id) % keep_frames) == 0)
        keep_fleets_together(system_manage);

    projectile_manage.tick(*this, step_s, system_manage);

    frame_counter++;
}

void battle_manager::draw(sf::RenderWindow& win)
{
    projectile_manage.draw(win);

    std::vector<ship*> linear_vec;

    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            linear_vec.push_back(s);
        }
    }

    ///There must be a reason that this is backwards, there's no way I'd do this
    ///Its because its the opposite order that highlighting is done, highlight does the first found
    ///so this follows that convention
    for(int i = ((int)linear_vec.size())-1; i >= 0; i--)
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

vec2f battle_manager::get_avg_centre_global()
{
    if(ship_map.size() == 0)
        return {0,0};

    vec2f avg = {0,0};
    int num = 0;

    for(orbital* o : ship_map)
    {
        avg += o->absolute_pos;
        num++;
    }

    if(num > 0)
    {
        avg = avg / (float)num;
    }

    return avg;
}

///Ok. I'm designing this with rts view in mind
///Still need to avoid pileups though
void battle_manager::add_fleet(orbital* o)
{
    if(o->type != orbital_info::FLEET)
        return;

    ship_map.push_back(o);

    ship_manager* sm = o->data;

    int sm_num = 0;

    for(ship* s : sm->ships)
    {
        s->enter_combat();

        vec2f to_center = o->absolute_pos - get_avg_centre_global();

        vec2f perp = perpendicular(to_center);

        if(perp.length() <= FLOAT_BOUND)
        {
            perp = {1, 0};
        }

        //printf("%f %f\n", EXPAND_2(perp));

        float battle_rad = 400;

        vec2f spos = to_center.norm() * battle_rad + perp.norm() * 50.f * sm_num;

        s->local_pos = spos;
        s->dim = {100, 40};

        s->check_load({s->dim.x(), s->dim.y()});

        sm_num++;
    }
}

ship* battle_manager::get_nearest_hostile(ship* s)
{
    empire* my_empire = s->owned_by->parent_empire;

    float min_dist = FLT_MAX;
    ship* found_ship = nullptr;

    for(orbital* o : ship_map)
    {
        for(ship* os : o->data->ships)
        {
            empire* other = o->parent_empire;

            if(!my_empire->is_hostile(other))
                continue;

            if(os->fully_disabled())
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
    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            if(point_within_ship(pos, s))
                return s;
        }
    }

    return nullptr;
}

void battle_manager::set_view(system_manager& system_manage)
{
    vec2f avg = {0,0};
    int num = 0;

    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            avg += s->local_pos;

            num++;
        }
    }

    if(num > 0)
        avg = avg / (float)num;

    /*sf::View v1 = win.getDefaultView();
    v1.setCenter(avg.x(), avg.y());

    win.setView(v1);*/

    system_manage.camera = avg;
}

bool battle_manager::can_disengage(empire* disengaging_empire)
{
    if(disengaging_empire == nullptr)
        return true;

    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            if(s->owned_by->parent_empire == disengaging_empire)
            {
                if(!s->can_disengage())
                    return false;
            }
        }
    }

    return true;
}

void battle_manager::do_disengage(empire* disengaging_empire)
{
    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            s->leave_combat();

            ///nobody initiated this, ie we're leaving combat in an acceptable way
            if(disengaging_empire == nullptr)
                continue;

            if(s->owned_by->parent_empire != disengaging_empire)
                continue;

            s->apply_disengage_penalty();
        }
    }
}

std::string battle_manager::get_disengage_str(empire* disengaging_empire)
{
    if(disengaging_empire == nullptr)
        return "";

    float time_remaining = 0.f;

    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            if(s->owned_by->parent_empire != disengaging_empire)
                continue;

            time_remaining = std::max(time_remaining, combat_variables::mandatory_combat_time_s - s->time_in_combat_s);
        }
    }

    return to_string_with_enforced_variable_dp(time_remaining) + "s";
}

///when we implement alliances, this will need to change
bool battle_manager::can_end_battle_peacefully(empire* leaving_empire)
{
    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            if(s->owned_by->parent_empire == leaving_empire)
                continue;

            if(s->owned_by->parent_empire != nullptr && !s->owned_by->parent_empire->is_hostile(leaving_empire))
                continue;

            if(!s->fully_disabled())
            {
                return false;
            }
        }
    }

    return true;
}

void battle_manager::end_battle_peacefully()
{
    do_disengage(nullptr);
}

bool battle_manager::any_in_fleet_involved(ship_manager* sm)
{
    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
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

bool battle_manager::any_in_empire_involved(empire* e)
{
    for(orbital* o : ship_map)
    {
        for(ship* s : o->data->ships)
        {
            if(s->owned_by->parent_empire == e)
                return true;
        }
    }

    return false;
}

void battle_manager::destructive_merge_into_me(battle_manager* bm, all_battles_manager& all_battles)
{
    for(auto& i : bm->projectile_manage.projectiles)
    {
        projectile_manage.projectiles.push_back(i);
    }

    for(orbital* o : ship_map)
    {
        add_fleet(o);
    }

    /*if(all_battles.currently_viewing == bm)
    {
        all_battles.currently_viewing = this;
    }*/

    if(all_battles.viewing(*bm))
    {

    }
}

orbital_system* battle_manager::get_system_in(system_manager& system_manage)
{
    if(ship_map.size() == 0)
    {
        return nullptr;
    }

    return ship_map[0]->parent_system;
}

void battle_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(is_ui_opened, ser);
        s.handle_serialise(ship_map, ser);
        s.handle_serialise(projectile_manage, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        s.handle_serialise(ship_map, ser);
        s.handle_serialise(projectile_manage, ser);
    }
}

battle_manager* all_battles_manager::make_new()
{
    battle_manager* bm = new battle_manager;

    battles.push_back(bm);

    bm->make_dirty();

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

void all_battles_manager::tick(float step_s, system_manager& system_manage)
{
    for(auto& i : battles)
    {
        i->tick(step_s, system_manage);
    }

    bool any = false;

    for(battle_manager* bm : battles)
    {
        if(viewing(*bm))
        {
            current_view.involved_orbitals = bm->ship_map;

            any = true;
        }
    }

    if(!any)
    {
        current_view.stop();
    }

    /*if(currently_viewing == nullptr && battles.size() > 0)
    {
        currently_viewing = battles[0];
    }*/

    ///build list of battles here
}

void all_battles_manager::draw_viewing(sf::RenderWindow& win)
{
    if(!current_view.any())
        return;

    for(battle_manager* bm : battles)
    {
        if(viewing(*bm))
        {
            bm->draw(win);
        }
    }
}

void all_battles_manager::set_viewing(battle_manager* bm, system_manager& system_manage, bool jump)
{
    if(bm == nullptr)
        return;

    current_view.involved_orbitals = bm->ship_map;

    if(jump)
        bm->set_view(system_manage);
}

bool all_battles_manager::viewing(battle_manager& battle)
{
    ///look through known battles, pick best fit
    ///for the moment just pick any fit :)
    for(orbital* obm : battle.ship_map)
    {
        for(orbital* mbm : current_view.involved_orbitals)
        {
            if(obm == mbm)
            {
                return true;
            }
        }
    }

    return false;
}

battle_manager* all_battles_manager::get_currently_viewing()
{
    for(battle_manager* bm : battles)
    {
        if(viewing(*bm))
        {
            return bm;
        }
    }

    return nullptr;
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
                bm->destructive_merge_into_me(bfind, *this);

                destroy(bfind);

                i--;

                term = true;

                break;
            }
        }

        if(term)
            break;
    }


    for(orbital* o : t1)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        bm->add_fleet(o);
    }

    return bm;

    //printf("made new battle with %i %i\n", t1.size(), nships);
}

void all_battles_manager::disengage(battle_manager* bm, empire* disengaging_empire)
{
    if(!bm->can_disengage(disengaging_empire))
        return;

    bm->do_disengage(disengaging_empire);

    bm->projectile_manage.destroy_all();

    /*if(bm == currently_viewing)
        currently_viewing = nullptr;*/

    if(viewing(*bm))
    {
        current_view.stop();
    }

    destroy(bm);
}

void all_battles_manager::end_battle_peacefully(battle_manager* bm)
{
    disengage(bm, nullptr);
}

battle_manager* all_battles_manager::get_battle_involving(ship_manager* ship_manage)
{
    for(battle_manager* bm : battles)
    {
        if(bm->any_in_fleet_involved(ship_manage))
        {
            return bm;
        }
    }

    return nullptr;
}

void all_battles_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(request_leave_battle_view, ser);
        s.handle_serialise(request_enter_battle_view, ser);
        s.handle_serialise(request_stay_in_battle_system, ser);
        s.handle_serialise(request_stay_id, ser);
        s.handle_serialise(current_view.involved_orbitals, ser);
        s.handle_serialise(battles, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        s.handle_serialise(battles, ser);
    }
}

void all_battles_manager::erase_all()
{
    for(battle_manager* battle : battles)
    {
        for(projectile* proj : battle->projectile_manage.projectiles)
        {
            delete proj;
        }

        delete battle;
    }

    battles.clear();
}
