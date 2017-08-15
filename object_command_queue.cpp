#include "object_command_queue.hpp"
#include "system_manager.hpp"
#include "ship.hpp"
#include <SFML/Graphics.hpp>
#include "ui_util.hpp"


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


///returns if finished
bool do_transfer(orbital* o, float step_s, queue_type& type)
{
    object_command_queue_info::queue_element_data& data = type.data;

    if(o->parent_system != data.transfer_within)
        return true;

    if(o->type == orbital_info::FLEET)
    {
        ship_manager* sm = (ship_manager*)o->data;

        if(sm->any_in_combat() && !data.combat_move)
            return true;

        if(!sm->can_move_in_system())
            return true;
    }

    vec2f end_pos = data.new_radius * (vec2f){cos(data.new_angle), sin(data.new_angle)};

    float straightline_distance = (end_pos - o->absolute_pos).length();


    float a1 = (o->absolute_pos - o->parent->absolute_pos).angle();
    float a2 = (end_pos - o->parent->absolute_pos).angle();

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

    float iangle = cur_angle + step_s * (a2 - a1);
    float irad = cur_rad + step_s * (data.new_radius - cur_rad);

    vec2f calculated_next = irad * (vec2f){cos(iangle), sin(iangle)};
    vec2f calculated_cur = o->orbital_length * (vec2f){cos(o->orbital_angle), sin(o->orbital_angle)};

    float speed = step_s * o->get_transfer_speed();

    ///this is the real speed here
    vec2f calc_dir = (calculated_next - calculated_cur).norm() * speed;

    vec2f calc_real_next = calculated_cur + calc_dir;

    iangle = calc_real_next.angle();
    irad = calc_real_next.length();

    o->orbital_angle = iangle;
    o->orbital_length = irad;

    if(straightline_distance < 2)
    {
        //o->transferring = false;
        return true;
    }

    //if(data.cancel_immediately)
    //    return true;

    return false;
}

///currently warp immediately terminates, whereas it should wait if we've shift clicked
///potentially should wait if we right click, but it needs ui feedback to do that (destination set)
bool do_warp(orbital* o, queue_type& type)
{
    if(o->type != orbital_info::FLEET)
        return true;

    ship_manager* sm = (ship_manager*)o->data;

    object_command_queue_info::queue_element_data& data = type.data;

    orbital_system* fin = data.fin;
    orbital_system* cur = o->parent_system;

    if(!sm->within_warp_distance(fin, o))
        return true;

    if(!sm->can_warp(fin, cur, o))
        return false;

    sm->force_warp(fin, o->parent_system, o);

    return true;
}

void object_command_queue::transfer(float pnew_rad, float pnew_angle, orbital* o, orbital_system* viewing_system, bool at_back, bool combat_move, bool target_drifts)
{
    queue_type next;

    next.data.old_radius = o->orbital_length;
    next.data.old_angle = o->orbital_angle;
    next.data.new_radius = pnew_rad;
    next.data.new_angle = pnew_angle;

    next.data.start_time_s = o->internal_time_s;
    next.data.transfer_within = viewing_system;

    next.data.combat_move = combat_move;
    next.data.target_drifts = target_drifts;
    //next.data.cancel_immediately = cancel_immediately;

    next.type = object_command_queue_info::IN_SYSTEM_PATH;

    while(command_queue.size() > 0 && command_queue.front().type == object_command_queue_info::IN_SYSTEM_PATH && !at_back)
    {
        command_queue.pop_front();
    }

    add(next, at_back);
}

void object_command_queue::transfer(vec2f pos, orbital* o, orbital_system* viewing_system, bool at_back, bool combat_move, bool target_drifts)
{
    vec2f base;

    if(o->parent)
        base = o->parent->absolute_pos;

    vec2f rel = pos - base;

    transfer(rel.length(), rel.angle(), o, viewing_system, at_back, combat_move, target_drifts);
}

bool object_command_queue::transferring()
{
    if(command_queue.size() == 0)
        return false;

    return command_queue.front().type == object_command_queue_info::IN_SYSTEM_PATH;
}

bool object_command_queue::trying_to_warp()
{
    if(command_queue.size() == 0)
        return false;

    return command_queue.front().type == object_command_queue_info::WARP;
}

bool object_command_queue::is_currently_colonising()
{
    if(command_queue.size() == 0)
        return false;

    return command_queue.front().type == object_command_queue_info::COLONISE;
}

orbital* object_command_queue::get_colonising_target()
{
    if(!is_currently_colonising())
        return nullptr;

    return command_queue.front().data.colony_target;
}

void object_command_queue::try_warp(const std::vector<orbital_system*>& systems, bool queue_at_back)
{
    for(auto& i : systems)
    {
        try_warp(i, queue_at_back);
    }
}

void object_command_queue::try_warp(orbital_system* fin, bool queue_to_back)
{
    queue_type next;

    next.data.fin = fin;

    next.type = object_command_queue_info::WARP;

    add(next, true, queue_to_back);
}

void object_command_queue::colonise(orbital* target, ship* colony_ship)
{
    queue_type next;

    next.data.colony_ship_id = colony_ship->id;
    next.data.colony_target = target;

    next.type = object_command_queue_info::COLONISE;

    add(next);
}

void object_command_queue::anchor(orbital* target)
{
    queue_type next;

    next.data.anchor_target = target;

    next.type = object_command_queue_info::ANCHOR;

    add(next);
}

void object_command_queue::anchor_ui_state()
{
    queue_type next;
    next.type = object_command_queue_info::ANCHOR_UI;

    add(next);
}

bool object_command_queue::is_currently(object_command_queue_info::queue_element_type type)
{
    if(command_queue.size() == 0)
        return false;

    return command_queue.front().type == type;
}

bool object_command_queue::is_ever(object_command_queue_info::queue_element_type type)
{
    if(command_queue.size() == 0)
        return false;

    for(auto& elem : command_queue)
    {
        if(elem.type == type)
            return true;
    }

    return false;
}

bool do_colonising(orbital* o, queue_type& type)
{
    if(o->type != orbital_info::FLEET)
        return true;

    ship_manager* sm = (ship_manager*)o->data;

    object_command_queue_info::queue_element_data& data = type.data;

    if(data.colony_ship_id == -1)
        return true;

    ship* colony = nullptr;

    for(ship* s : sm->ships)
    {
        if(s->id == data.colony_ship_id)
        {
            colony = s;
            break;
        }
    }

    if(colony == nullptr)
        return true;

    //if(sm->any_in_combat())
    //    return true;

    if(sm->any_derelict())
        return true;

    if(sm->any_colonising())
        return false;

    if(data.colony_target == nullptr)
        return true;

    if(data.colony_target->parent_system != o->parent_system)
        return true;

    colony->colonising = true;
    colony->colonise_target = data.colony_target;

    return !sm->any_colonising();
}

bool do_anchor(orbital* o, queue_type& type)
{
    if(o->type != orbital_info::FLEET)
        return true;

    ship_manager* sm = (ship_manager*)o->data;

    object_command_queue_info::queue_element_data& data = type.data;

    if(data.anchor_target == nullptr)
        return true;

    if(data.anchor_target->parent_system != o->parent_system)
        return true;

    float maintain_distance = 20.f;

    vec2f my_pos = o->absolute_pos;
    vec2f their_pos = data.anchor_target->absolute_pos;

    if((their_pos - my_pos).length() > maintain_distance)
    {
        ///sadly cancel immediately doesn't work correctly as transfer isn't constant time
        o->command_queue.transfer(data.anchor_target->absolute_pos, o, o->parent_system, false, false, true);
    }

    return false;
}

bool do_anchor_ui(orbital* o, queue_type& type)
{
    orbital_system* sys = o->parent_system;

    int id = 0;

    bool success = false;

    for(orbital* test_orbital : sys->orbitals)
    {
        if(test_orbital->type != orbital_info::ASTEROID)
            continue;

        if(!test_orbital->is_resource_object)
            continue;

        test_orbital->begin_render_asteroid_window();

        if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
            success = true;
            o->command_queue.anchor(test_orbital);
        }

        test_orbital->end_render_asteroid_window();

        if(success)
            return true;

        id++;
    }

    return false;
}

/*void object_command_queue::add(object_command_queue_info::queue_element_type type, const object_command_queue_info::queue_element_data& data)
{
    return add({type, data})
}*/

void object_command_queue::add(const queue_type& type, bool at_back, bool does_not_cancel_if_at_back)
{
    sf::Keyboard key;

    if(!key.isKeyPressed(sf::Keyboard::LShift) && at_back && !does_not_cancel_if_at_back)
         cancel();

    if(at_back)
        command_queue.push_back(type);
    else
        command_queue.push_front(type);
}

void object_command_queue::tick(orbital* o, float step_s)
{
    cancel_internal(o);

    if(command_queue.size() == 0)
        return;

    drift_applicable_transfer_targets(step_s);

    bool first_warp = true;

    orbital_system* last_warp = nullptr;

    for(int i=0; i<command_queue.size(); i++)
    {
        auto& to_test = command_queue.front();

        if(first_warp && to_test.type == object_command_queue_info::WARP)
        {
            if(to_test.data.fin == o->parent_system)
            {
                to_test.data.should_pop = true;
            }

            if(to_test.data.fin == last_warp)
            {
                to_test.data.should_pop = true;

                last_warp = to_test.data.fin;
            }

            first_warp = false;
        }
    }

    cancel_internal(o);

    auto& next = command_queue.front();
    object_command_queue_info::queue_element_type cur = next.type;

    ///just do like, ship->tick_path destination etc
    if(cur == object_command_queue_info::IN_SYSTEM_PATH)
    {
        if(do_transfer(o, step_s, next))
        {
            next.data.should_pop = true;
        }
    }

    if(cur == object_command_queue_info::WARP)
    {
        if(do_warp(o, next))
        {
            next.data.should_pop = true;
        }
    }

    if(cur == object_command_queue_info::COLONISE)
    {
        if(do_colonising(o, next))
        {
            next.data.should_pop = true;
        }
    }

    if(cur == object_command_queue_info::FIGHT)
    {

    }

    if(cur == object_command_queue_info::ANCHOR)
    {
        if(do_anchor(o, next))
        {
            next.data.should_pop = true;
        }
    }

    if(cur == object_command_queue_info::ANCHOR_UI)
    {
        if(do_anchor_ui(o, next))
        {
            next.data.should_pop = true;
        }
    }

    if(is_front_complete() && command_queue.size() > 0)
    {
        command_queue.pop_front();
    }

    cancel_internal(o);

    should_pop = false;
}

void object_command_queue::drift_applicable_transfer_targets(float step_s)
{
    for(auto& i : command_queue)
    {
        if(i.type != object_command_queue_info::IN_SYSTEM_PATH)
            continue;

        if(i.data.target_drifts == false)
            continue;

        float& target_angle = i.data.new_angle;

        target_angle += orbital::calculate_orbital_drift_angle(i.data.new_radius, step_s);
    }
}

bool object_command_queue::is_front_complete()
{
    return should_pop;
}

void object_command_queue::cancel()
{
    #if 0

    while(!command_queue.empty())
    {
        /*auto next = command_queue.front();
        object_command_queue_info::queue_element_type cur = next.type;

        if(cur == object_command_queue_info::COLONISE && o->type == orbital_info::FLEET)
        {
            ship_manager* sm = (ship_manager*)o->data;

            for(ship* s : sm->ships)
            {
                if(s->id == next.data.colony_ship_id)
                {
                    s->colonising = false;
                    s->colonise_target = nullptr;
                }
            }
        }*/

        //command_queue.pop_front();
    }
    #endif

    for(auto& next : command_queue)
    {
        next.data.should_pop = true;
    }
}

void object_command_queue::cancel_internal(orbital* o)
{
    for(int i=0; i<command_queue.size(); i++)
    {
        if(command_queue[i].data.should_pop)
        {
            if(command_queue[i].type == object_command_queue_info::COLONISE && o->type == orbital_info::FLEET)
            {
                ship_manager* sm = (ship_manager*)o->data;

                for(ship* s : sm->ships)
                {
                    if(s->id == command_queue[i].data.colony_ship_id)
                    {
                        s->colonising = false;
                        s->colonise_target = nullptr;
                    }
                }
            }

            command_queue.erase(command_queue.begin() + i);
            i--;
            continue;
        }
    }
}

void object_command_queue::remove_any_of(object_command_queue_info::queue_element_type type)
{
    for(auto& i : command_queue)
    {
        if(i.type == type)
            i.data.should_pop = true;
    }
}

std::vector<orbital_system*> object_command_queue::get_warp_destinations()
{
    std::vector<orbital_system*> systems;

    for(auto& i : command_queue)
    {
        if(i.type != object_command_queue_info::WARP)
            continue;

        object_command_queue_info::queue_element_data& data = i.data;

        systems.push_back(data.fin);
    }

    return systems;
}

orbital_system* object_command_queue::get_warp_destination()
{
    orbital_system* sys = nullptr;

    for(auto& i : command_queue)
    {
        if(i.type != object_command_queue_info::WARP)
            continue;

        object_command_queue_info::queue_element_data& data = i.data;

        sys = data.fin;
    }

    return sys;
}
