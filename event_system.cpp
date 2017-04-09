#include "event_system.hpp"
#include "system_manager.hpp"
#include "../../render_projects/imgui/imgui.h"
#include "util.hpp"
#include "ship.hpp"
#include "ship_definitions.hpp"
#include "empire.hpp"

bool wait_ev(game_event& event, float time_s)
{
    if(event.time_alive_s >= time_s)
    {
        return true;
    }

    return false;
}

bool transition_ev(game_event& event, float time_s, std::function<void(game_event&)> fevent)
{
    if(event.time_alive_s > time_s)
    {
        fevent(event);

        return true;
    }

    return false;
}

void add_wait(waiting_event ev, game_event& event)
{
    event.waiting_events.push_back(ev);
}

std::function<void(game_event&)> wait(float time_s, std::function<void(game_event&)> func)
{
    waiting_event ev;

    ev.is_finished = std::bind(transition_ev, std::placeholders::_1, time_s, func);

    return std::bind(add_wait, ev, std::placeholders::_1);
}

std::function<void(game_event&)> both(std::function<void(game_event&)> f1, std::function<void(game_event&)> f2)
{
    return [f1, f2](game_event& e){f1(e); f2(e);};
}

void transition_dialogue(game_event& event, dialogue_node* node)
{
    event.dialogue = *node;
}

std::function<void(game_event&)> dlge(dialogue_node& node)
{
    return std::bind(transition_dialogue, std::placeholders::_1, &node);
}

void terminate_quest(game_event& event)
{
    event.dialogue.is_open = false;
    event.parent->finished = true;
    event.alert_location->has_quest_alert = false;
}

void give_random_research_from_ancient(game_event& event)
{
    empire* derelict = event.parent->ancient_faction;
    empire* interactor = event.parent->interacting_faction;

    research& r = derelict->research_tech_level;

    float total_available_currency = r.level_to_currency() - r.level_to_currency();

    total_available_currency /= 10.f;

    total_available_currency = clamp(total_available_currency, 0.f, r.level_to_currency() / 2.f);

    if(total_available_currency < 50)
        total_available_currency = 50;

    interactor->add_resource(resource::RESEARCH, total_available_currency);
}

void give_random_research_currency(game_event& event)
{
    int amount = 100 + randf_s(-40.f, 40.f);

    int type = resource::RESEARCH;

    event.parent->interacting_faction->add_resource((resource::types)type, amount);
}

void give_random_resource(game_event& event)
{
    int amount = 100 + randf_s(-40.f, 40.f);

    int type = randf_s(0.f, resource::COUNT);

    event.parent->interacting_faction->add_resource((resource::types)type, amount);
}

void give_research_currency_proportion_lo(game_event& event)
{
    float research_frac = 0.05f;

    empire* interactor = event.parent->interacting_faction;

    research& r = interactor->research_tech_level;

    float total_currency = r.level_to_currency() * research_frac;

    if(total_currency < 50)
        total_currency = 50.f;

    interactor->add_resource(resource::RESEARCH, total_currency);
}

void give_research_currency_proportion_hi(game_event& event)
{
    float research_frac = 0.07f;

    empire* interactor = event.parent->interacting_faction;

    research& r = interactor->research_tech_level;

    float total_currency = r.level_to_currency() * research_frac;

    if(total_currency < 50)
        total_currency = 50.f;

    interactor->add_resource(resource::RESEARCH, total_currency);
}

void culture_shift(game_event& event)
{
    empire* derelict = event.parent->ancient_faction;
    empire* interactor = event.parent->interacting_faction;

    interactor->culture_shift(0.01f, derelict);
}

void hostile_interaction(game_event& event)
{
    empire* derelict = event.parent->ancient_faction;
    empire* interactor = event.parent->interacting_faction;

    derelict->negative_interaction(interactor);
}

void positive_interaction(game_event& event)
{
    empire* derelict = event.parent->ancient_faction;
    empire* interactor = event.parent->interacting_faction;

    derelict->positive_interaction(interactor);
}

void resupply_nearby_fleets(game_event& event)
{
    empire* interactor = event.parent->interacting_faction;

    ///do this but without empire affiliation?
    auto sms = event.parent->get_nearby_fleets(interactor);

    for(ship_manager* sm : sms)
    {
        sm->resupply_from_nobody();
    }
}

void damage_nearby_fleets(game_event& event)
{
    empire* interactor = event.parent->interacting_faction;

    auto sms = event.parent->get_nearby_fleets(interactor, 60.f);

    ///this is really unsatisfying as a way to do ship damage
    for(auto& sm : sms)
        sm->random_damage_ships(0.6f);
}

ship* spawn_ship_base(game_event& event)
{
    orbital* o = event.alert_location;
    empire* owner_faction = event.parent->ancient_faction;
    orbital_system* os = o->parent_system;
    fleet_manager* fleet_manage = event.parent->fleet_manage;

    ship_manager* derelict_fleet = fleet_manage->make_new();

    ship* new_ship = derelict_fleet->make_new_from(owner_faction->team_id, make_default());
    new_ship->name = "SS Toimplement name generation";

    orbital* onew_fleet = os->make_new(orbital_info::FLEET, 5.f);
    onew_fleet->orbital_angle = o->orbital_angle;
    onew_fleet->orbital_length = o->orbital_length + 40;
    onew_fleet->parent = os->get_base(); ///?
    onew_fleet->data = derelict_fleet;

    owner_faction->take_ownership(onew_fleet);
    owner_faction->take_ownership(derelict_fleet);

    new_ship->set_tech_level_from_empire(owner_faction);

    onew_fleet->tick(0.f);

    return new_ship;
}

ship* spawn_derelict_base(game_event& event)
{
    ship* s = spawn_ship_base(event);
    s->randomise_make_derelict();

    return s;
}

void spawn_derelict(game_event& event)
{
    spawn_derelict_base(event);
}

void spawn_derelict_and_terminate(game_event& event)
{
    spawn_derelict_base(event);

    event.parent->finished = true;
}

void spawn_hostile(game_event& event)
{
    spawn_ship_base(event);
}

void destroy_spawn_derelict_and_terminate(game_event& event)
{
    ship* s = spawn_derelict_base(event);

    research raw = s->get_research_real_for_empire(s->original_owning_race, event.parent->interacting_faction);
    auto res = s->resources_received_when_scrapped();

    res[resource::RESEARCH] = raw.units_to_currency(true);

    float destruction_penalty = 0.5f;

    for(auto& i : res)
    {
        i.second *= destruction_penalty;

        event.parent->interacting_faction->add_resource(i.first, i.second);
    }

    s->cleanup = true;

    event.parent->finished = true;
}

/*void spawn_hostile_and_terminate(game_event& event)
{
    spawn_hostile(event);

    event.parent->finished = true;
}*/

/*void do_salvage_resolution_hostile(game_event& event)
{
    event.dialogue = salvage_resolution_hostile;

    spawn_hostile(event);
}*/

/*void do_observation_resolution_salvage(game_event& event)
{
    event.dialogue = salvage_resolution;

    spawn_derelict(event);

    event.parent->finished = true;
}*/

namespace lone_derelict_dialogue
{
    dialogue_node observation_powerup
    {
        "Ongoing",
        "The derelict's power systems appear to be fluctuating",
        {
            "Take no chances, blow it up",
            "Keep observing",
            "Assess it for salvage value",
        },
        {
            dlge(resolution_destroyed), wait(30.f, both(dlge(observation_resolution_hostile), spawn_hostile)), both(dlge(salvage_resolution_hostile), spawn_hostile)
        }
    };

    dialogue_node observation_powerdown
    {
        "Ongoing",
        "The derelict's power systems are completely flatlined",
        {
            "Assess it for salvage value",
        },
        {
            both(dlge(salvage_resolution), spawn_derelict)
        }
    };

    void observation_wait(game_event& event)
    {
        bool hostile = randf_s(0.f, 1.f) < 0.5f;

        waiting_event ev;

        if(hostile)
            ev.is_finished = std::bind(transition_ev, std::placeholders::_1, 30.f, dlge(observation_powerup));
        else
            ev.is_finished = std::bind(transition_ev, std::placeholders::_1, 30.f, dlge(observation_powerdown));

        event.waiting_events.push_back(ev);
    }

    dialogue_node observation
    {
        "Ongoing",
        "You begin to watch carefully from afar",
        {

        },
        {
            observation_wait
        }
    };

    dialogue_node resolution_destroyed =
    {
        "Resolution",
        "The ship was blown out of the sky, you scrap the remains for materials",
        {

        },
        {
            destroy_spawn_derelict_and_terminate
        }
    };

    dialogue_node salvage_resolution =
    {
        "Resolution",
        "The ship appears badly damaged but could be made spaceworth again",
        {

        },
        {
            terminate_quest
        }
    };

    dialogue_node salvage_resolution_hostile =
    {
        "Alert!",
        "As the salvage drones approach the ship, it begins to power up and becomes hostile!",
        {

        },
        {
            terminate_quest
        }
    };

    dialogue_node observation_resolution_hostile
    {
        "Alert!",
        "The power fluctuations rapidly stabilise, the ship is active and hostile!",
        {

        },
        {
            terminate_quest
        }
    };

    static void decide_derelict_or_hostile(game_event& event)
    {
        if(randf_s(0.f, 1.f) < 0.9f)
        {
            event.dialogue = salvage_resolution;

            spawn_derelict(event);
        }
        else
        {
            event.dialogue = salvage_resolution_hostile;

            spawn_hostile(event);
        }
    };

    ///we need to data drive the event system from here somehow
    ///maybe have ptrs to bools that get set if we click one (or store internally)
    ///that way the next event can see what the previous event picked
    ///or like, trigger events somehow from here
    dialogue_node dia_first =
    {
        "Derelict Ship",
        "A derelict ship was detected drifting lazily in orbit",
        {
            "Take no chances, blow it up",
            "Observe it from a distance",
            "Assess it for salvage value",
        },
        {
            dlge(resolution_destroyed), dlge(observation), decide_derelict_or_hostile,
        },
    };

    dialogue_node get()
    {
        return dia_first;
    }
}

///we need factions to do this one
///might spawn a new faction due to us helping them
namespace alien_precursor_technology
{
    dialogue_node explosion
    {
        "Alert!",
        "We tripped their defences, a massive explosion has destroyed all traces of the facility and damaged any nearby fleet",
        {

        },
        {
            both(terminate_quest, hostile_interaction)
        }
    };

    dialogue_node blow_it_up
    {
        "Information",
        "You wipe the structure off the face of the planet",
        {
            "Good riddance"
        },
        {
            both(terminate_quest, hostile_interaction)
        }
    };

    dialogue_node power_fluctuations
    {
        "Information",
        "There appear to be massive power fluctuations originating from the surface",
        {
            "Blow it up from space",
            "Continue observation",
        },
        {
            dlge(blow_it_up), wait(5.f, both(dlge(explosion), damage_nearby_fleets))
        }
    };


    dialogue_node nothing_happens_again
    {
        "Resolution",
        "With no signals from the ground, it seems likely that there's nothing further to be gained here",
        {
            "Leave it alone",
            "Blow it up from space"
        },
        {
            dlge(nothing_happens_again), dlge(blow_it_up)
        }
    };

    void anything_happens_space_post_ground(game_event& event)
    {
        bool happens = randf_s(0.f, 1.f) < 0.7f;

        if(!happens)
        {
            event.dialogue = nothing_happens_again;
        }
        else
        {
            event.dialogue = power_fluctuations;
        }
    }

    dialogue_node no_return
    {
        "Information",
        "The ground team did not return, and we received no further signals or communication of any kind",
        {
            "Leave it alone",
            "Observe from space",
            "Blow it up",
        },
        {
            dlge(no_return), wait(10.f, anything_happens_space_post_ground), both(terminate_quest, hostile_interaction)
        }
    };

    dialogue_node found_technology
    {
        "Information",
        "We found no life, but a wealth of technology from an ancient civilisation",
        {

        },
        {
            both(terminate_quest, both(give_random_research_from_ancient, positive_interaction)) ///shift culture, bump interaction count
        }
    };

    dialogue_node mutated_f2
    {
        "Information",
        "What next?",
        no_return.options,
        no_return.onclick
    };

    dialogue_node mutated_flavour
    {
        "Information",
        "The mutated team members appear to have died agonisingly on the planet's surface",
        {
            "Interesting",
        },
        {
            both(dlge(mutated_f2), give_research_currency_proportion_lo)
        }
    };

    dialogue_node recovery
    {
        "Information",
        "The crew members died slowly and painfully in quarantine",
        {
            "Interesting",
        },
        {
            both(dlge(mutated_f2), both(give_research_currency_proportion_hi, culture_shift))
        }
    };

    dialogue_node team_mutated
    {
        "Information",
        "The team has emerged from the structure... horrifically mutated. The comms are full of nothing but garbled moans",
        {
            "Destroy everything",
            "Attempt to recover crew for study",
            "Continue observing from space",
        },
        {
            dlge(blow_it_up), dlge(recovery), dlge(mutated_flavour),
        }
    };


    /*dialogue_node first_contact
    {
        "Information","
        "We received garbled communication from the surface",
        {
            ""
        }
    }*/

    ///if we've observed from space, make it more likely that nothing happens on that quest chain
    void anything_happens_ground(game_event& event)
    {
        bool team_does_not_return = randf_s(0.f, 1.f) < 0.2f;

        if(team_does_not_return)
        {
            event.dialogue = no_return;

            return;
        }

        bool team_comes_back_horribly_mutated = randf_s(0.f, 1.f) < 0.1f;

        if(team_comes_back_horribly_mutated)
        {
            event.dialogue = team_mutated;

            return;
        }

        bool massive_explosion = randf_s(0.f, 1.f) < 0.05f;

        if(massive_explosion)
        {
            event.dialogue = explosion;

            return;
        }

        /*bool friendly_contact = randf_s(0.f, 1.f) < 0.2f;

        ///make this dependent on empire culture and number of interactions
        if(friendly_contact)
        {
            event.dialogue = first_contact;

            return;
        }*/

        event.dialogue = found_technology;
    }

    dialogue_node nothing_happens
    {
        "Information",
        "The appears to be no signals originating from the ground",
        {
            "Leave it alone",
            "Send in a ground team",
        },
        {
            dlge(nothing_happens), wait(5.f, anything_happens_ground)
        }
    };

    void anything_happens_space(game_event& event)
    {
        bool happens = randf_s(0.f, 1.f) < 0.3f;

        if(!happens)
        {
            event.dialogue = nothing_happens;
        }
        else
        {
            event.dialogue = power_fluctuations;
        }
    }

    dialogue_node first
    {
        "Information",
        "We have detected traces of an ancient alien civilisation on this planet",
        {
            "Observe it from space",
            "Send in a ground team",
        },
        {
            wait(10.f, anything_happens_space), wait(5.f, anything_happens_ground)
        },
    };

    dialogue_node get()
    {
        return first;
    }
}

namespace bountiful_resources
{
    dialogue_node first
    {
        "Information",
        "You have discovered a planet with bountiful natural resources, we have claimed a small portion for ourselves",
        {

        },
        {
            both(terminate_quest, give_random_resource)
        }
    };

    dialogue_node get()
    {
        return first;
    }
}

namespace volcanism
{
    dialogue_node first
    {
        "Information",
        "This planet is highly volcanically active, the sensor readings will produce invaluable research",
        {

        },
        {
            both(terminate_quest, give_random_research_currency)
        }
    };

    dialogue_node get()
    {
        return first;
    }
}

namespace abandoned_spacestation
{
    dialogue_node first
    {
        "Information",
        "A recently abandoned spacestation, of modern design, lies in orbit",
        {
            "Take any leftover supplies",
            "Scrap it for resources",
        },
        {
            both(terminate_quest, resupply_nearby_fleets), both(terminate_quest, give_random_resource)
        }
    };

    dialogue_node get()
    {
        return first;
    }
}

///need to be able to set up observation posts to give gradual tech
///ie creating an orbital
namespace neutron_star
{
    dialogue_node first
    {
        "Information",
        "The star of this system is a neutron star - this is rather uncommon",
        {
            "Log the sensor data",
        },
        {
            both(terminate_quest, give_random_research_currency)
        }
    };

    dialogue_node get()
    {
        return first;
    }
}

namespace shattered_planet
{
    dialogue_node first
    {
        "Information",
        "This planet was shattered by a massive impact, and is slowly reforming under its own gravitational pull",
        {
            "Log the sensor data",
            "Salvage some natural resources",
        },
        {
            both(terminate_quest, give_random_research_currency), both(terminate_quest, give_random_research_currency)
        }
    };

    dialogue_node get()
    {
        return first;
    }
}

/*namespace cusp_of_spaceflight
{
    dialogue_node first
    {
        "Information",
        ""
    }
}*/

int present_dialogue(const std::vector<std::string>& options)
{
    int selected = -1;
    int num = 0;

    for(auto& str : options)
    {
        //std::string display_str = "(" + std::to_string(num + 1) + ") " + str;
        std::string display_str = str;

        if(display_str.length() > 0 && num != 0)
            ImGui::BulletText(display_str.c_str());

        if(display_str.length() > 0 && num == 0)
        {
            ImGui::TextWrapped(display_str.c_str());
            ImGui::NewLine();
        }

        if(ImGui::IsItemClicked())
        {
            selected = num;
        }

        num++;
    }

    return selected;
}

///somehow implement branching in here. Maybe we should literally make like a ptr structure of dialogue options?
/*std::vector<std::string> get_dialogue_and_options(int arc_type, int event_offset)
{
    if(event_offset == 0)
    {
        return {"Sample Dialogue here, also there, etc i'm tired ok", "Ruh roh, click this to not die!",
        "Well.... I guess this is dialogue",
        "I'm not very good at writing, we're all going to die",};
    }
    if(event_offset == 1)
    {
        return {"Whelp"};
    }

    return {""};
}*/

void game_event::draw_ui()
{
    for(int i=0; i<waiting_events.size(); i++)
    {
        if(waiting_events[i].is_finished(*this))
        {
            waiting_events.erase(waiting_events.begin() + i);
            i--;
            continue;
        }
    }

    if(waiting_events.size() > 0)
        return;

    if(dialogue.header == "")
        return;

    if(!dialogue.is_open)
        return;

    std::string header_str = dialogue.header + " (" + alert_location->name + ")";

    ImGui::Begin((header_str + "###DLOGUE" + alert_location->name).c_str(), &dialogue.is_open);

    std::vector<std::string> dialogue_options = {dialogue.text};

    ///patch dialogue options here if necessary
    for(auto& i : dialogue.options)
    {
        dialogue_options.push_back(i);
    }

    if(dialogue.options.size() == 0)
    {
        dialogue_options.push_back("Ok");
    }

    int selected = present_dialogue(dialogue_options) - 1;

    if(selected >= 0)
    {
        game_event next = parent->make_next_event();

        if(selected < dialogue.onclick.size() && dialogue.onclick[selected] != nullptr)
        {
            dialogue.onclick[selected](next);
        }

        /*if(selected < dialogue.travel.size())
        {
            next.dialogue = *(dialogue.travel[selected]);
        }*/

        parent->event_history.push_back(next);
    }

    ImGui::End();
}

void game_event::tick(float step_s)
{
    if(parent->finished)
    {
        alert_location->has_quest_alert = false;
        return;
    }

    alert_location->has_quest_alert = true;

    time_alive_s += step_s;

    //set_dialogue_open_state(alert_location->dialogue_open);
}

void game_event::set_dialogue_open_state(bool open)
{
    dialogue.is_open = open;
}

game_event game_event_manager::make_next_event()
{
    game_event ret;
    ret.arc_type = arc_type;
    ret.alert_location = event_history.back().alert_location;
    ret.parent = this;
    ret.event_num = event_history.size();

    return ret;
}

game_event_manager::game_event_manager(game_event_info::types type, orbital* o, fleet_manager& pfleet_manage)
{
    arc_type = randf_s(0.f, game_event_info::COUNT);

    game_event first;
    first.alert_location = o;
    first.arc_type = arc_type;
    first.parent = this;
    first.event_num = 0;

    if(type == game_event_info::ANCIENT_PRECURSOR)
        first.dialogue = alien_precursor_technology::get(); ///set from map the starting dialogues?
    else if(type == game_event_info::LONE_DERELICT)
        first.dialogue = lone_derelict_dialogue::get();

    first.dialogue.is_open = false;

    event_history.push_back(first);

    ///fleet manager never expires, so this is fine
    fleet_manage = &pfleet_manage;
}

void game_event_manager::set_faction(empire* e)
{
    ancient_faction = e;
}

void game_event_manager::set_interacting_faction(empire* e)
{
    interacting_faction = e;
}

bool game_event_manager::can_interact(empire* e)
{
    return get_nearby_fleet(e) != nullptr;
}

ship_manager* game_event_manager::get_nearby_fleet(empire* e)
{
    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        bool any_valid = false;

        for(ship* s : sm->ships)
        {
            if(!s->fully_disabled())
            {
                any_valid = true;
                break;
            }
        }

        if(!any_valid)
            continue;

        if(o->parent_system != event_history.back().alert_location->parent_system)
            continue;

        float interact_distance = 120.f;

        if((o->absolute_pos - event_history.back().alert_location->absolute_pos).length() < interact_distance)
        {
            return sm;
        }
    }

    return nullptr;
}

std::vector<ship_manager*> game_event_manager::get_nearby_fleets(empire* e, float dist)
{
    std::vector<ship_manager*> ret;

    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        bool any_valid = false;

        for(ship* s : sm->ships)
        {
            if(!s->fully_disabled())
            {
                any_valid = true;
                break;
            }
        }

        if(!any_valid)
            continue;

        if(o->parent_system != event_history.back().alert_location->parent_system)
            continue;

        float interact_distance = dist;

        if((o->absolute_pos - event_history.back().alert_location->absolute_pos).length() < interact_distance)
        {
            ret.push_back(sm);
        }
    }

    return ret;
}

ship_manager* game_event_manager::get_nearest_fleet(empire* e)
{
    float min_dist = 9999999;
    ship_manager* min_sm = nullptr;

    for(orbital* o : e->owned)
    {
        if(o->type != orbital_info::FLEET)
            continue;

        ship_manager* sm = (ship_manager*)o->data;

        bool any_valid = false;

        for(ship* s : sm->ships)
        {
            if(!s->fully_disabled())
            {
                any_valid = true;
                break;
            }
        }

        if(!any_valid)
            continue;

        if(o->parent_system != event_history.back().alert_location->parent_system)
            continue;

        float interact_distance = 120.f;

        float len = (o->absolute_pos - event_history.back().alert_location->absolute_pos).length();

        if(len < interact_distance && len < min_dist)
        {
            min_dist = len;
            min_sm = sm;
        }
    }

    return min_sm;
}

void game_event_manager::toggle_dialogue_state()
{
    event_history.back().dialogue.is_open = !event_history.back().dialogue.is_open;
}

void game_event_manager::set_dialogue_state(bool d)
{
    event_history.back().dialogue.is_open = d;
}

void game_event_manager::draw_ui()
{
    event_history.back().draw_ui();
}

void game_event_manager::tick(float step_s)
{
    internal_time_s += step_s;

    event_history.back().tick(step_s);
}

game_event_manager* all_events_manager::make_new(game_event_info::types type, orbital* o, fleet_manager& fm)
{
    game_event_manager* gem = new game_event_manager(type, o, fm);

    events.push_back(gem);

    return gem;
}

game_event_manager* all_events_manager::orbital_to_game_event(orbital* o)
{
    for(auto& i : events)
    {
        if(i->event_history.back().alert_location == o)
            return i;
    }

    return nullptr;
}

void all_events_manager::tick(float step_s)
{
    for(auto& i : events)
    {
        i->tick(step_s);
    }
}

void all_events_manager::draw_ui()
{
    for(auto& i : events)
    {
        i->draw_ui();
    }
}
