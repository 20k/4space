#include "ship.hpp"
#include "battle_manager.hpp"
#include "empire.hpp"
#include "text.hpp"
#include "system_manager.hpp"
#include "util.hpp"
#include "top_bar.hpp"
#include "procedural_text_generator.hpp"
#include <unordered_map>
#include "profile.hpp"
#include <iterator>
#include "popup.hpp"
#include <imgui/imgui_internal.h>
#include "tooltip_handler.hpp"

int ship_manager::gid;

uint32_t ship::gid;

std::vector<ship_component_elements::component_element_info> ship_component_elements::element_infos;
std::vector<ship_component_elements::types> ship_component_elements::allowed_skip_repair_def;

std::vector<int> ship_component_elements::repair_in_combat_map;
std::vector<int> ship_component_elements::repair_out_combat_map;
std::vector<int> ship_component_elements::allowed_skip_repair;
std::vector<int> ship_component_elements::skippable_in_display;
std::vector<int> ship_component_elements::weapons_map;

std::vector<std::string> ship_component_elements::display_strings;
//std::vector<float> ship_component_elements::base_cost_of_component_with_this_primary_attribute;
std::vector<research_info::types> ship_component_elements::component_element_to_research_type;


using component_info_t = ship_component_elements::component_element_info;

float get_tech_type_cost(ship_component_elements::tech_type type)
{
    using namespace ship_component_elements;

    if(type & LOW)
        return 1;

    if(type & MEDIUM)
        return 2;

    if(type & HIGH)
        return 4;

    if(type & SUPER_HIGH)
        return 8;

    return 1;
}

std::map<resource::types, float> get_tech_type_resource_ratio(ship_component_elements::tech_type type)
{
    using namespace ship_component_elements;

    std::map<resource::types, float> ret;

    if(type & COMMON)
    {
        ret[resource::IRON] = 1;
        ret[resource::COPPER] = 0.5f;
    }

    if(type & RARE)
    {
        ret[resource::IRON] = 1;
        ret[resource::COPPER] = 0.5;
        ret[resource::TITANIUM] = 0.25;
    }

    if(type & HARD_RARE)
    {
        ret[resource::IRON] = 0.5;
        ret[resource::COPPER] = 0.75;
        ret[resource::TITANIUM] = 0.55;
    }

    if(type & LOW_VOLUME)
    {
        ret[resource::IRON] = 0.5f;
        ret[resource::URANIUM] = 0.25f;
    }

    if(type & ALL_COMMON)
    {
        ret[resource::COPPER] = 1;
        ret[resource::HYDROGEN] = 1;
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
    }

    return ret;
}

void ship_component_elements::generate_element_infos()
{
    /*COOLING_POTENTIAL,
    ENERGY,
    OXYGEN,
    AMMO,
    FUEL,
    CARGO,
    SHIELD_POWER,
    ARMOUR,
    HP,
    ENGINE_POWER,
    WARP_POWER,
    SCANNING_POWER,
    COMMAND,
    STEALTH,
    COLONISER,
    RAILGUN,
    TORPEDO,
    PLASMAGUN,
    COILGUN,*/

    using namespace ship_component_elements;

    element_infos.resize(NONE);

    std::vector<component_element_info>& ei = element_infos;

    ei[COOLING_POTENTIAL].display_name = "Cooling";
    ei[COOLING_POTENTIAL].research_type = research_info::MATERIALS;
    ei[COOLING_POTENTIAL].negative_is_bad = true;


    ei[ENERGY].display_name = "Energy";
    ei[ENERGY].research_type = research_info::MATERIALS;
    ei[ENERGY].negative_is_bad = true;



    ei[OXYGEN].display_name = "Oxygen";
    ei[OXYGEN].research_type = research_info::MATERIALS;
    ei[OXYGEN].allowed_skip_in_repair = true;
    ei[OXYGEN].negative_is_bad = true;
    //ei[OXYGEN].resource_type = resource::OXYGEN;
    ei[OXYGEN].combat_view_if_not_full = true;



    ei[AMMO].display_name = "Ammo";
    ei[AMMO].research_type = research_info::MATERIALS;


    ei[FUEL].display_name = "Fuel";
    ei[FUEL].research_type = research_info::MATERIALS;


    ei[CARGO].display_name = "Cargo";
    ei[CARGO].research_type = research_info::MATERIALS;
    ei[CARGO].combat_view_if_damaged = false;


    ei[SHIELD_POWER].display_name = "Shields";
    ei[SHIELD_POWER].research_type = research_info::MATERIALS;
    ei[SHIELD_POWER].combat_view_mandatory = true;


    ei[ARMOUR].display_name = "Armour";
    ei[ARMOUR].research_type = research_info::MATERIALS;
    ei[ARMOUR].combat_view_mandatory = true;


    ei[HP].display_name = "HP";
    ei[HP].research_type = research_info::MATERIALS;
    ei[HP].negative_is_bad = true;
    ei[HP].combat_view_mandatory = true;



    ei[ENGINE_POWER].display_name = "Engines";
    ei[ENGINE_POWER].research_type = research_info::PROPULSION;


    ei[WARP_POWER].display_name = "Warp";
    ei[WARP_POWER].research_type = research_info::PROPULSION;
    ei[WARP_POWER].combat_view_if_damaged = false;


    ei[SCANNING_POWER].display_name = "Scanning";
    ei[SCANNING_POWER].research_type = research_info::SCANNERS;
    ei[SCANNING_POWER].combat_view_if_damaged = false;


    ei[COMMAND].display_name = "Command";
    ei[COMMAND].research_type = research_info::MATERIALS;
    ei[COMMAND].negative_is_bad = true;



    ei[STEALTH].display_name = "Stealth";
    ei[STEALTH].research_type = research_info::MATERIALS;


    ei[COLONISER].display_name = "Habitation Systems";
    ei[COLONISER].research_type = research_info::MATERIALS;
    ei[COLONISER].combat_view_if_damaged = false;


    ei[RAILGUN].display_name = "Railgun";
    ei[RAILGUN].research_type = research_info::WEAPONS;
    ei[RAILGUN].combat_view_mandatory = true;


    ei[TORPEDO].display_name = "Torpedo";
    ei[TORPEDO].research_type = research_info::WEAPONS;
    ei[TORPEDO].combat_view_mandatory = true;


    ei[PLASMAGUN].display_name = "Plasmagun";
    ei[PLASMAGUN].research_type = research_info::WEAPONS;
    ei[PLASMAGUN].combat_view_mandatory = true;


    ei[COILGUN].display_name = "Coilgun";
    ei[COILGUN].research_type = research_info::WEAPONS;
    ei[COILGUN].combat_view_mandatory = true;


    ei[RESOURCE_PRODUCTION].display_name = "Resource Producer";
    ei[RESOURCE_PRODUCTION].research_type = research_info::MATERIALS;
    ei[RESOURCE_PRODUCTION].combat_view_if_damaged = false;


    ei[RESOURCE_STORAGE].display_name = "Storage";
    ei[RESOURCE_STORAGE].research_type = research_info::MATERIALS;
    ei[RESOURCE_STORAGE].combat_view_if_damaged = false;


    ei[SHIPYARD].display_name = "Shipyard";
    ei[SHIPYARD].research_type = research_info::MATERIALS;
    ei[SHIPYARD].combat_view_if_damaged = false;

    ei[MASS_ANCHOR].display_name = "Mass Anchor";
    ei[MASS_ANCHOR].research_type = research_info::MATERIALS;
    ei[MASS_ANCHOR].combat_view_if_damaged = false;

    ei[ORE_HARVESTER].display_name = "Ore Miner";
    ei[ORE_HARVESTER].research_type = research_info::MATERIALS;
    ei[ORE_HARVESTER].combat_view_if_damaged = false;

    ei[RESOURCE_PULLER].display_name = "Resource Requester";
    ei[RESOURCE_PULLER].research_type = research_info::MATERIALS;
    ei[RESOURCE_PULLER].combat_view_if_damaged = false;

    ///why is this a macro?
    #define DEFINE_RESOURCE(name, rarity) \
                                        ei[name].display_name = #name; \
                                        ei[name].combat_view_if_damaged = false; \
                                        ei[name].resource_rarity = rarity; \
                                        ei[name].research_type = research_info::MATERIALS; \
                                        ei[name].resource_type = resource::name;

    DEFINE_RESOURCE(COPPER, 0.5);
    DEFINE_RESOURCE(HYDROGEN, 1);
    DEFINE_RESOURCE(IRON, 1);
    DEFINE_RESOURCE(TITANIUM, 0.25);
    DEFINE_RESOURCE(URANIUM, 0.1);
    DEFINE_RESOURCE(RESEARCH, 0.25);

    int num = 0;

    for(component_element_info& i : ei)
    {
        if(i.resource_type != resource::COUNT)
        {
            for(int kk=1; kk < i.display_name.size(); kk++)
            {
                i.display_name[kk] = tolower(i.display_name[kk]);
            }
        }

        if(i.allowed_skip_in_repair)
        {
            allowed_skip_repair_def.push_back((types)num);
        }

        display_strings.push_back(i.display_name);
        //base_cost_of_component_with_this_primary_attribute.push_back(i.base_cost);
        component_element_to_research_type.push_back(i.research_type);

        num++;
    }

    repair_in_combat_map = generate_repair_priorities(repair_priorities_in_combat_def);
    repair_out_combat_map = generate_repair_priorities(repair_priorities_out_combat_def);
    allowed_skip_repair = generate_repair_priorities(allowed_skip_repair_def);
    skippable_in_display = generate_repair_priorities(skippable_in_display_def);
    weapons_map = generate_repair_priorities(weapons_map_def);
}

std::map<resource::types, float> ship_component_elements::component_storage_to_resources(const types& type)
{
    std::map<resource::types, float> ret;

    if(type == AMMO)
    {
        ret[resource::IRON] = 1;
    }

    if(type == FUEL)
    {
        ret[resource::HYDROGEN] = 1;
    }

    if(type == ARMOUR)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.5f;
    }

    if(type == HP)
    {
        ret[resource::IRON] = 1;
    }

    if(element_infos[(int)type].resource_type != resource::COUNT)
    {
        ret[element_infos[(int)type].resource_type] = 1;
    }

    return ret;
}

///this will all be obsolete soon
/*std::map<resource::types, float> ship_component_elements::component_base_construction_ratio(const types& type, component& c)
{
    std::map<resource::types, float> ret;

    ret[resource::IRON] = 1;

    if(type == COOLING_POTENTIAL)
    {
        ret[resource::IRON] = 1;
    }

    if(type == ENERGY)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.5f;
        ret[resource::COPPER] = 0.2f;
    }

    if(type == OXYGEN)
    {
        ret[resource::IRON] = 1;
        ret[resource::COPPER] = 0.5f;
    }

    if(type == AMMO)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
    }

    if(type == FUEL)
    {
        ret[resource::IRON] = 1;
    }

    if(type == CARGO)
    {
        ret[resource::IRON] = 1;
    }

    if(type == SHIELD_POWER)
    {
        ret[resource::IRON] = 0.5f;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
    }

    if(type == ARMOUR)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
    }

    if(type == HP)
    {
        ret[resource::IRON] = 1;
    }

    if(type == ENGINE_POWER)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
    }

    if(type == WARP_POWER)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
    }

    if(type == SCANNING_POWER)
    {
        ret[resource::IRON] = 0.3f;
        ret[resource::COPPER] = 1;
    }

    if(type == COMMAND)
    {
        ret[resource::IRON] = 1;
    }

    if(type == STEALTH)
    {
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
        ret[resource::URANIUM] = 0.2f;
    }

    if(type == COLONISER)
    {
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
        ret[resource::URANIUM] = 1;
        ret[resource::OXYGEN] = 1;
        ret[resource::HYDROGEN] = 1;
        ret[resource::IRON] = 1;
    }

    if(type == RAILGUN)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.55f;
        ret[resource::COPPER] = 0.15f;
    }

    if(type == TORPEDO)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.25f;
        ret[resource::COPPER] = 0.5f;
    }

    if(type == PLASMAGUN)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.8f;
        ret[resource::COPPER] = 1;
    }

    if(type == COILGUN)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 0.25f;
        ret[resource::COPPER] = 0.2f;
    }

    if(type == RESOURCE_PRODUCTION)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
        ret[resource::URANIUM] = 0.1;
    }

    if(type == RESOURCE_STORAGE)
    {
        ret[resource::IRON] = 1;
    }

    if(type == SHIPYARD)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
    }

    if(type == ORE_HARVESTER)
    {
        ret[resource::IRON] = 1;
        ret[resource::TITANIUM] = 1;
        ret[resource::COPPER] = 1;
    }

    for(auto& i : c.extra_resources_ratio)
    {
        ret[i.first] += i.second;
    }

    return ret;
}*/

float get_tech_level_value(float level)
{
    if(level <= 0)
        return ship_component_elements::tech_upgrade_effectiveness;

    return get_tech_level_value(level-1) + ship_component_elements::tech_upgrade_effectiveness;
}

float ship_component_elements::upgrade_value(float value, float old_tech, float new_tech)
{
    new_tech += 1;
    old_tech += 1;

    float nratio = pow(ship_component_elements::tech_upgrade_effectiveness, new_tech) / pow(ship_component_elements::tech_upgrade_effectiveness, old_tech);
    float nratio_cooldown = pow(ship_component_elements::tech_cooldown_upgrade_effectiveness, new_tech) / pow(ship_component_elements::tech_cooldown_upgrade_effectiveness, old_tech);


    return value * nratio;
}

void ship_component_elements::upgrade_component(component_attribute& in, int type, float old_tech, float new_tech)
{
    //float divisor = pow(old_tech + 6, ship_component_elements::tech_upgrade_effectiveness);
    //float multiplier = pow(new_tech + 6, ship_component_elements::tech_upgrade_effectiveness);

    //float nratio = multiplier / divisor;

    //float nratio = pow((new_tech + 1) / (old_tech + 1), ship_component_elements::tech_upgrade_effectiveness);

    new_tech += 1;
    old_tech += 1;

    float nratio = pow(ship_component_elements::tech_upgrade_effectiveness, new_tech) / pow(ship_component_elements::tech_upgrade_effectiveness, old_tech);
    float nratio_cooldown = pow(ship_component_elements::tech_cooldown_upgrade_effectiveness, new_tech) / pow(ship_component_elements::tech_cooldown_upgrade_effectiveness, old_tech);

    ///at infinity, nratio = nratio cooldown
    ///what we really wanna do is calculate backwards from infinity
    ///so at tech level 1, we take small steps
    ///and at tech level 10, all drain is at like, 0
    ///hmm. Keep this for the moment

    in.produced_per_s *= nratio;
    in.produced_per_use *= nratio;

    in.drained_per_s /= nratio_cooldown;
    in.drained_per_use /= nratio_cooldown;


    in.time_between_uses_s /= nratio_cooldown;
    in.max_amount *= nratio;
}

float component_attribute::add_amount(float amount)
{
    cur_amount += amount;

    if(cur_amount > max_amount)
    {
        float extra = cur_amount - max_amount;

        cur_amount = max_amount;

        return extra;
    }

    if(cur_amount < 0)
    {
        float extra = cur_amount;

        cur_amount = 0;

        return extra;
    }

    return 0.f;
}

bool component_attribute::can_use()
{
    bool time_valid = current_time_s >= time_last_used_s + time_between_uses_s;
    bool efficiency_valid = cur_efficiency > 0.75f;

    return time_valid && efficiency_valid;
}

void component_attribute::use()
{
    if(!can_use())
        return;

    time_last_used_s = current_time_s;
}

float component_attribute::get_available_capacity() const
{
    return max_amount - cur_amount;
}

float component_attribute::get_total_capacity(float step_s)
{
    return get_available_capacity() + (drained_per_s * step_s - currently_drained);
}

float component_attribute::get_drain_capacity(float step_s)
{
    return drained_per_s * step_s - currently_drained;
}

float component_attribute::get_produced_amount(float step_s)
{
    return produced_per_s * cur_efficiency * step_s;
}

/*float component_attribute::get_net()
{
    return produced_per_s - drained_per_s;
}*/

void component_attribute::update_time(float step_s)
{
    current_time_s += step_s;

    currently_drained = 0;

    available_for_consumption = get_produced_amount(step_s);
}

float component_attribute::consume_max(float amount_to_try)
{
    float available = available_for_consumption;

    available -= amount_to_try;

    float amount_valid = amount_to_try;

    if(available < 0)
    {
        float extra_needed = fabs(available);

        available_for_consumption = 0.f;

        cur_amount -= extra_needed;

        if(cur_amount < 0)
        {
            float extra = fabs(cur_amount);

            cur_amount = 0;

            amount_valid -= extra;
        }
    }

    cur_amount = clamp(cur_amount, 0.f, max_amount);

    return amount_valid;
}

float component_attribute::consume_max_available(float amount_to_try)
{
    float available = available_for_consumption;

    available -= amount_to_try;

    if(available < 0)
    {
        available_for_consumption = 0;

        return amount_to_try - fabs(available);
    }

    available_for_consumption = available;

    return amount_to_try;
}

float component_attribute::consume_max_stored(float amount_to_try)
{
    float available = cur_amount;

    available -= amount_to_try;

    if(available < 0)
    {
        cur_amount = 0;

        return amount_to_try - fabs(available);
    }

    cur_amount = available;

    return amount_to_try;
}

void component_attribute::calculate_efficiency(float step_s)
{
    cur_efficiency = get_efficiency(step_s);
}

float component_attribute::get_efficiency(float step_s)
{
    if(drained_per_s * step_s < FLOAT_BOUND)
        return 1.f;

    float eff = currently_drained / (drained_per_s * step_s);

    ///stops small floating point errors causing jitter if efficiency almost precisely 1
    eff = eff * 1000.f;
    eff = round(eff);
    eff = eff / 1000.f;

    return eff;
}

void component_attribute::upgrade_tech_level(int type, float from, float to)
{
    ship_component_elements::upgrade_component(*this, type, from, to);
}

void component_attribute::set_max_tech_level_from_empire_and_component_attribute(int type, empire* e)
{
    int appropriate_tech = ship_component_elements::component_element_to_research_type[type];

    int new_tech_level = e->research_tech_level.categories[appropriate_tech].amount;

    if(tech_level >= new_tech_level)
        return;

    upgrade_tech_level(type, tech_level, new_tech_level);

    tech_level = new_tech_level;
}

void component_attribute::set_tech_level_from_empire(int type, empire* e)
{
    int appropriate_tech = ship_component_elements::component_element_to_research_type[type];

    int new_tech_level = e->research_tech_level.categories[appropriate_tech].amount;

    upgrade_tech_level(type, tech_level, new_tech_level);

    tech_level = new_tech_level;
}

void component_attribute::set_tech_level_from_research(int type, research& r)
{
    int appropriate_tech = ship_component_elements::component_element_to_research_type[type];

    int new_tech_level = r.categories[appropriate_tech].amount;

    upgrade_tech_level(type, tech_level, new_tech_level);

    tech_level = new_tech_level;
}

/*void component_attribute::set_tech_level(float ctech_level)
{
    tech_level = ctech_level;
}*/

float component_attribute::get_tech_level()
{
    return tech_level;
}

void component_attribute::upgrade_size(float old_size, float new_size)
{
    float new_ratio = new_size / old_size;

    produced_per_s *= new_ratio;
    produced_per_use *= new_ratio;

    drained_per_s *= new_ratio;
    drained_per_use *= new_ratio;

    //time_between_uses_s *= new_ratio;
    max_amount *= new_ratio;

    if(cur_amount > max_amount)
    {
        cur_amount = max_amount;
    }
}

float component_attribute::consume_from(component_attribute& other, float max_proportion, float step_s)
{
    if(drained_per_s <= FLOAT_BOUND)
        return 0.f;

    float max_drain_amount = drained_per_s * max_proportion * step_s;

    float needed_drain = drained_per_s * step_s - currently_drained;

    needed_drain = std::max(needed_drain, 0.f);

    float to_drain = std::min(max_drain_amount, needed_drain);

    float amount_drained = other.consume_max(to_drain);

    currently_drained += amount_drained;
    //cur_efficiency = currently_drained / (drained_per_s * step_s);

    return amount_drained;
}

float component_attribute::consume_from_amount_available(component_attribute& other, float amount, float step_s)
{
    if(drained_per_s <= FLOAT_BOUND)
        return 0.f;

    float max_drain_amount = amount;

    float needed_drain = drained_per_s * step_s - currently_drained;

    needed_drain = std::max(needed_drain, 0.f);

    float to_drain = std::min(max_drain_amount, needed_drain);

    float amount_drained = other.consume_max_available(to_drain);

    currently_drained += amount_drained;
    //cur_efficiency = currently_drained / (drained_per_s * step_s);

    return amount_drained;

}
float component_attribute::consume_from_amount_stored(component_attribute& other, float amount, float step_s)
{
    if(drained_per_s <= FLOAT_BOUND)
        return 0.f;

    float max_drain_amount = amount;

    float needed_drain = drained_per_s * step_s - currently_drained;

    needed_drain = std::max(needed_drain, 0.f);

    float to_drain = std::min(max_drain_amount, needed_drain);

    float amount_drained = other.consume_max_stored(to_drain);

    currently_drained += amount_drained;
    //cur_efficiency = currently_drained / (drained_per_s * step_s);

    return amount_drained;
}

/*float component_attribute::drain(float amount)
{
    if(drained_per_s <= 0.001f)
        return amount;

    currently_drained += amount;

    float extra = currently_drained - drained_per_s;

    extra = std::max(extra, 0.f);

    if(currently_drained > drained_per_s)
        currently_drained = drained_per_s;

    cur_efficiency = currently_drained / drained_per_s;

    return extra;
}*/

void component::set_tech_type(int tt)
{
    tech_type = (ship_component_elements::tech_type)tt;
}

float component::get_hp_frac()
{
    if(!has_element(ship_component_element::HP))
    {
        return 1.f;
    }

    float frac = get_stored()[ship_component_element::HP] / get_stored_max()[ship_component_element::HP];

    return frac;
}

void component::upgrade_size(float old_size, float new_size)
{
    if(new_size <= 0.f)
        return;

    //for(auto& elem : components)
    for(auto& attr : components)
    {
        if(!attr.present)
            continue;

        attr.upgrade_size(old_size, new_size);
    }

    if(has_tag(component_tag::DAMAGE))
    {
        float dam = get_tag(component_tag::DAMAGE) * (new_size / old_size);

        set_tag(component_tag::DAMAGE, dam);
    }

    if(has_tag(component_tag::WARP_DISTANCE))
    {
        float wrp = get_tag(component_tag::WARP_DISTANCE) * (new_size / old_size);

        set_tag(component_tag::WARP_DISTANCE, wrp);
    }
}

void component::set_size(float new_size)
{
    upgrade_size(current_size, new_size);

    current_size = new_size;
}

void component::set_ship_size(float new_size)
{
    upgrade_size(ship_size, new_size);

    ship_size = new_size;
}

component component::with_size(float new_size)
{
    component c = *this;
    c.set_size(new_size);

    //for(auto& item : c.components)

    for(int type = 0; type < c.components.size(); type++)
    {
        if(type == (int)ship_component_elements::HP)
        {
            component_attribute& attr = c.components[type];

            if(!attr.present)
                continue;

            attr.cur_amount = attr.max_amount;
        }
    }

    return c;
}

component::component()
{
    components.resize(ship_component_elements::NONE);
}

/*bool component::has_element(const ship_component_element& type)
{
    return components[type].present;
}*/

component_attribute component::get_element(const ship_component_element& type)
{
    if(!has_element(type))
        return component_attribute();

    return components[type];
}

std::map<ship_component_element, float> merge_diffs(std::map<ship_component_element, float> ret, const std::map<ship_component_element, float>& two)
{
    ///will correctly expand to everything in both of the elements
    for(auto& i : two)
    {
        ret[i.first] += i.second;
    }

    return ret;
}

///how to account for max storage? Apply_diff?
/*std::map<ship_component_element, float> component::get_timestep_diff(float step_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float net = (attr.produced_per_s - attr.drained_per_s) * step_s * attr.cur_efficiency;

        ret[type] = net;
    }

    return ret;
}*/

std::map<ship_component_element, float> component::get_timestep_production_diff(float step_s)
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        float net = attr.get_produced_amount(step_s);

        ret[(ship_component_element)type] = net;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_timestep_consumption_diff(float step_s)
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        float net = (attr.drained_per_s) * step_s;

        ret[(ship_component_element)type] = net;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored()
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        float val = attr.cur_amount;

        ret[(ship_component_element)type] = val;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored_max()
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        float val = attr.max_amount;

        ret[(ship_component_element)type] = val;
    }

    return ret;
}

///how to account for max storage? Apply_diff?
///need to keep track of time
std::map<ship_component_element, float> component::get_use_diff()
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        float net = 0.f;

        if(attr.can_use())
            net = (attr.produced_per_use - attr.drained_per_use);

        ret[(ship_component_element)type] = net;
    }

    return ret;
}

float component::get_use_frac()
{
    std::map<ship_component_element, float> use_diff = get_use_diff();
    std::map<ship_component_element, float> available_diff = get_stored();

    int num = 0;
    float accum = 0.f;

    for(auto& i : use_diff)
    {
        //printf("%f %f %i\n", i.second, available_diff[i.first], i.first);

        if(fabs(i.second) < FLOAT_BOUND)
            continue;

        accum += fabs(available_diff[i.first]) / fabs(i.second);

        num++;
    }

    if(num > 0)
        accum /= num;

    return accum;
}

/*std::pair<component, std::map<ship_component_element, float>> component::apply_diff(const std::map<ship_component_element, float>& diff)
{
    component ret_comp = *this;

    std::map<ship_component_element, float> ret_extra;

    for(const auto& i : diff)
    {
        auto type = i.first;
        float diff_amount = i.second;

        ///don't apply something to me if we can't take it
        if(!has_element(type))
        {
            ret_extra[type] += diff_amount;
            continue;
        }

        auto attr = components[type];

        float extra = attr.add_amount(diff_amount);

        ret_comp.components[type] = attr;
        ret_extra[type] += extra;
    }

    return {ret_comp, ret_extra};
}*/

std::map<ship_component_element, float> component::apply_diff(const std::map<ship_component_element, float>& available)
{
    std::map<ship_component_element, float> ret_extra;

    for(const auto& i : available)
    {
        auto type = i.first;
        float diff_amount = i.second;

        ///don't apply something to me if we can't take it
        if(!has_element(type))
        {
            ret_extra[type] += diff_amount;
            continue;
        }

        /*auto attr = components[type];

        float extra = attr.add_amount(diff_amount);

        components[type] = attr;
        ret_extra[type] += extra;*/

        float extra = components[type].add_amount(diff_amount);

        ret_extra[type] += extra;
    }

    return ret_extra;
}

void component::apply_diff_single(const ship_component_element& type, float amount)
{
    if(!has_element(type))
    {
        return;
    }

    float extra = components[type].add_amount(amount);
}

void component::update_time(float step_s)
{
    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.update_time(step_s);
    }
}

/*std::map<ship_component_element, float> component::get_available_capacities()
{
    std::map<ship_component_element, float> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        ret[i.first] = attr.get_available_capacity();
    }

    return ret;
}*/

std::vector<std::pair<ship_component_element, float>> component::get_available_capacities_vec()
{
    std::vector<std::pair<ship_component_element, float>> ret;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        ret.push_back({(ship_component_element)type, attr.get_available_capacity()});
    }

    return ret;
}

std::vector<float> component::get_available_capacities_linear_vec()
{
    std::vector<float> ret;
    ret.resize(ship_component_elements::NONE);

    for(int type = 0; type < components.size(); type++)
    {
        const component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        ret[type] = attr.get_available_capacity();
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored_and_produced_resources(float time_s)
{
    std::map<ship_component_element, float> ret = get_timestep_production_diff(time_s);

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        ret[(ship_component_element)type] += attr.cur_amount;
    }

    return ret;
}

/*std::map<ship_component_element, float> component::get_needed_resources()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        ret[i.first] = i.second.drained_per_s;
    }

    return ret;
}*/

void component::add(ship_component_element element, const component_attribute& attr)
{
    if(has_element(element))
    {
        printf("DUPLICATE ELEMENT ADDED RUH ROH");
        throw;
    }

    components[element] = attr;
    components[element].present = true;
}

float component::calculate_total_efficiency(float step_s)
{
    float min_eff = 1.f;

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.calculate_efficiency(step_s);

        if(attr.cur_efficiency < min_eff)
        {
            min_eff = attr.cur_efficiency;
        }
    }

    float max_hp = components[ship_component_element::HP].max_amount;
    float cur_hp = components[ship_component_element::HP].cur_amount;

    float frac = 1.f;

    if(max_hp > FLOAT_BOUND)
    {
        frac = cur_hp / max_hp;
    }

    return min_eff * frac;
}

void component::propagate_total_efficiency(float step_s)
{
    float min_eff = calculate_total_efficiency(step_s);

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.cur_efficiency = min_eff;
    }
}

ship_component_element component::get_weapon_type()
{
    using namespace ship_component_elements;

    if(has_element(RAILGUN))
        return RAILGUN;

    if(has_element(TORPEDO))
        return TORPEDO;

    if(has_element(PLASMAGUN))
        return PLASMAGUN;

    if(has_element(COILGUN))
        return COILGUN;

    return NONE;
}

int component::get_tag_offset(component_tag::tag tag)
{
    for(int i=0; i<tag_list.size(); i++)
    {
        if(tag_list[i].first == tag)
            return i;
    }

    return -1;
}


bool component::has_tag(component_tag::tag tag)
{
    //return tag_list.find(tag) != tag_list.end();

    for(auto& i : tag_list)
    {
        if(i.first == tag)
            return true;
    }

    return false;
}

float component::get_tag(component_tag::tag tag)
{
    int tag_id = get_tag_offset(tag);

    ///preserve integrity of map?
    if(tag_id == -1)
    {
        printf("warning: Tag fuckup in get_tag %i %s\n", tag, name.c_str());

        return 0.f;
    }

    return tag_list[tag_id].second;
}

void component::set_tag(component_tag::tag tag, float val)
{
    //tag_list[tag] = val;

    int offset = get_tag_offset(tag);

    component_tag_type* cur_tag = nullptr;

    if(offset == -1)
    {
        tag_list.resize(tag_list.size() + 1);
        cur_tag = &tag_list.back();
    }
    else
    {
        cur_tag = &tag_list[offset];
    }

    *cur_tag = {tag, val};

}

bool component::is_weapon()
{
    return ship_component_elements::weapons_map[primary_attribute] >= 0;
}

/*void component::set_tech_level(float tech_level)
{
    for(auto& i : components)
    {
        i.second.set_tech_level(tech_level);
    }
}*/

void component::set_tech_level_from_empire(empire* e)
{
    if(primary_attribute == ship_component_elements::WARP_POWER)
    {
        float tl = get_tech_level_of_primary();

        int appropriate_tech = ship_component_elements::component_element_to_research_type[primary_attribute];
        float new_tech_level = e->research_tech_level.categories[appropriate_tech].amount;

        float old_value = get_tag(component_tag::WARP_DISTANCE);

        float new_value = ship_component_elements::upgrade_value(old_value, tl, new_tech_level);

        set_tag(component_tag::WARP_DISTANCE, new_value);
    }

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.set_tech_level_from_empire(type, e);
    }
}

void component::set_max_tech_level_from_empire_and_component(empire* e)
{
    if(primary_attribute == ship_component_elements::WARP_POWER)
    {
        float tl = get_tech_level_of_primary();

        int appropriate_tech = ship_component_elements::component_element_to_research_type[primary_attribute];
        float new_tech_level = e->research_tech_level.categories[appropriate_tech].amount;

        if(tl < new_tech_level)
        {
            float old_value = get_tag(component_tag::WARP_DISTANCE);

            float new_value = ship_component_elements::upgrade_value(old_value, tl, new_tech_level);

            set_tag(component_tag::WARP_DISTANCE, new_value);
        }
    }

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.set_max_tech_level_from_empire_and_component_attribute(type, e);
    }
}

///TODO: OBSOLETE THIS
///use upgrade value for size scaling though
void component::set_tech_level_from_research(research& r)
{
    if(primary_attribute == ship_component_elements::WARP_POWER)
    {
        float tl = get_tech_level_of_primary();

        int appropriate_tech = ship_component_elements::component_element_to_research_type[primary_attribute];
        float new_tech_level = r.categories[appropriate_tech].amount;

        float old_value = get_tag(component_tag::WARP_DISTANCE);

        float new_value = ship_component_elements::upgrade_value(old_value, tl, new_tech_level);

        set_tag(component_tag::WARP_DISTANCE, new_value);
    }

    for(int type = 0; type < components.size(); type++)
    {
        component_attribute& attr = components[type];

        if(!attr.present)
            continue;

        attr.set_tech_level_from_research(type, r);
    }
}

/*void component::set_tech_level_of_element(ship_component_elements::types type, float tech_level)
{
    if(!has_element(type))
        return;

    components[type].set_tech_level(tech_level);
}*/

float component::get_tech_level_of_element(ship_component_elements::types type)
{
    //if(!has_element(type))
    //    return -1;

    float val = components[type].get_tech_level();

    //printf("%f val %i\n", val, type);

    return val;
}

float component::get_tech_level_of_primary()
{
    return get_tech_level_of_element(primary_attribute);
}

float component::get_base_component_cost()
{
    //if(primary_attribute == ship_component_elements::NONE)
    //    return 0.f;

    //return ship_component_elements::base_cost_of_component_with_this_primary_attribute[primary_attribute] * cost_mult * ship_component_elements::construction_cost_mult;

    return get_tech_type_cost(tech_type);
}

float component::get_component_cost()
{
    //printf("%i\n", primary_attribute);

    ///so for some reason, primary_attribute not being networked even though we're receiving it successfully

    float tech_level = get_tech_level_of_primary();
    float base_cost = get_base_component_cost();

    float cost = research_info::get_cost_scaling(tech_level, base_cost) * current_size * ship_size;

    //std::cout << tech_level << " " << base_cost << " " << current_size << " " << ship_size << std::endl;

    /*if(tech_level > 0)
    {
        printf("%f %f %f cost\n", cost, tech_level, base_cost);
    }*/

    return cost;
}

///fractional based on hp
float component::get_real_component_cost()
{
    if(!has_element(ship_component_elements::HP))
        return 0.f;

    component_attribute& hp_element = components[ship_component_elements::HP];

    if(hp_element.max_amount < 0.001f)
        return 0.f;

    return get_component_cost() * (hp_element.cur_amount / hp_element.max_amount);
}

std::map<resource::types, float> component::resources_cost()
{
    std::map<resource::types, float> res;

    if(!has_element(ship_component_elements::HP))
        return res;

    component_attribute& hp_elem = components[ship_component_elements::HP];

    if(hp_elem.max_amount < 0.001f)
        return res;

    res = get_tech_type_resource_ratio(tech_type);

    //res = ship_component_elements::component_base_construction_ratio(primary_attribute, *this);

    for(auto& i : res)
    {
        i.second *= get_component_cost();
    }

    return res;
}

std::map<resource::types, float> component::resources_received_when_scrapped()
{
    std::map<resource::types, float> res;

    if(!has_element(ship_component_elements::HP))
        return res;

    component_attribute& hp_elem = components[ship_component_elements::HP];

    if(hp_elem.max_amount < 0.001f)
        return res;

    res = get_tech_type_resource_ratio(tech_type);

    //res = ship_component_elements::component_base_construction_ratio(primary_attribute, *this);

    for(auto& i : res)
    {
        i.second *= get_real_component_cost();
    }

    return res;
}

///wtf, HP always costs iron to repair? WE'VE DUN FUCKED UP
std::map<resource::types, float> component::resources_needed_to_repair()
{
    std::map<resource::types, float> res;

    if(!has_element(ship_component_elements::HP))
        return res;

    component_attribute& hp_elem = components[ship_component_elements::HP];

    if(hp_elem.max_amount < 0.001f)
        return res;

    res = get_tech_type_resource_ratio(tech_type);

    //res = ship_component_elements::component_base_construction_ratio(primary_attribute, *this);

    for(auto& i : res)
    {
        //i.second *= std::max(hp_elem.max_amount - hp_elem.cur_amount, 0.f);

        i.second *= get_component_cost() - get_real_component_cost();
    }

    return res;
}

///MAIN DEFINITION OF TECH DISTANCES
research_category component::get_research_base_for_empire(empire* owner, empire* claiming_empire)
{
    ///everything *essentially* should have a primary attribute
    if(primary_attribute == ship_component_elements::NONE)
        return {research_info::MATERIALS, 0.f};

    float culture_distance_between_empires = claiming_empire->empire_culture_distance(owner);

    float tech_level_of_component = get_tech_level_of_primary();

    research_info::types research_type = ship_component_elements::component_element_to_research_type[primary_attribute];

    float claiming_tech_level = claiming_empire->get_research_level(research_type);

    float tech_distance = culture_distance_between_empires * (tech_level_of_component - claiming_tech_level + 2);

    //float amount = research_info::tech_unit_to_research_currency(tech_distance);

    float amount = tech_distance * current_size;

    return {research_type, amount};
}

research_category component::get_research_real_for_empire(empire* owner, empire* claiming_empire)
{
    auto cat = get_research_base_for_empire(owner, claiming_empire);

    cat.amount = safe_hp_frac_modify(cat.amount);

    return cat;
}

float component::safe_hp_frac_modify(float in)
{
    if(!has_element(ship_component_elements::HP))
        return in;

    float max_val = components[ship_component_element::HP].max_amount;

    if(max_val < 0.001f)
        return in;

    float cur_val = components[ship_component_element::HP].cur_amount;

    return in * (cur_val / max_val);
}

void component::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(cost_mult, ser);
        s.handle_serialise(repair_this_when_recrewing, ser);
        s.handle_serialise(skip_in_derelict_calculations, ser);
        s.handle_serialise(scanning_difficulty, ser);
        s.handle_serialise(clicked, ser);
        s.handle_serialise_no_clear(tag_list, ser);
        s.handle_serialise_no_clear(components, ser);
        s.handle_serialise(name, ser);
        s.handle_serialise(ship_size, ser);
        s.handle_serialise(current_size, ser);
        s.handle_serialise(tech_type, ser);
        s.handle_serialise(primary_attribute, ser);
    }

    if(serialise_data_helper::send_mode == 0 || serialise_data_helper::send_mode == 2)
    {
        s.handle_serialise_no_clear(components, ser);
        s.handle_serialise_no_clear(tag_list, ser);
    }
}

std::string get_component_attribute_display_string(const component& c, int type)
{
    std::string component_str;

    const component_attribute& attr = c.components[type];

    if(!attr.present)
        return component_str;

    std::string header = ship_component_elements::display_strings[type];

    float net_usage = attr.produced_per_s - attr.drained_per_s;
    float net_per_use = attr.produced_per_use - attr.drained_per_use;

    float time_between_uses = attr.time_between_uses_s;
    float time_until_next_use = std::max(0.f, attr.time_between_uses_s - (attr.current_time_s - attr.time_last_used_s));

    float max_amount = attr.max_amount;
    float cur_amount = attr.cur_amount;

    std::string use_string = "" + to_string_with_precision(net_usage, 3) + "(/s)";
    std::string per_use_string = "" + to_string_with_precision(net_per_use, 3) + "(/use)";
    std::string fire_time_remaining = "Time Left (s): " + to_string_with_enforced_variable_dp(time_until_next_use) + "/" + to_string_with_precision(time_between_uses, 3);

    std::string storage_str = "(" + to_string_with_enforced_variable_dp(cur_amount) + "/" + to_string_with_variable_prec(max_amount) + ")";

    component_str += header;

    if(net_usage != 0)
        component_str += " " + use_string;

    if(net_per_use != 0)
        component_str += " " + per_use_string;

    if(time_between_uses > 0)
        component_str += " " + fire_time_remaining;

    if(max_amount > 0)
        component_str += " " + storage_str;

    return component_str;
}

///so display this (ish) on mouseover for a component
std::string get_component_display_string(component& c)
{
    std::string component_str = "";//c.name + ":\n";

    int num = 0;


    float tech_level = c.get_tech_level_of_primary();

    std::string tech_str = "Tech Level: " + std::to_string((int)tech_level) + "\n";

    component_str += tech_str;


    std::string eff = "";

    float efficiency = 1.f;

    for(auto& i : c.components)
    {
        if(i.present)
        {
            efficiency = i.cur_efficiency * 100.f;
            break;
        }
    }

    if(efficiency < 0.001f)
        efficiency = 0.f;

    if(efficiency < 99.9f)
        eff = "Efficiency %%: " + to_string_with_enforced_variable_dp(efficiency) + "\n";

    component_str += eff;

    //for(auto& i : c.components)
    for(int type = 0; type < c.components.size(); type++)
    {
        const component_attribute& attr = c.components[type];

        if(!attr.present)
            continue;

        component_str += get_component_attribute_display_string(c, type);

        if(num != c.components.size() - 1)
        {
            component_str += "\n";
        }

        num++;
    }

    return component_str;
}

///vertical list of attributes, going horizontally with separators between components
///we'll also want a systems level version of this, ie overall not specific
/*std::vector<std::string> get_components_display_string(ship& s)
{
    std::vector<component> components = s.entity_list;

    std::vector<std::string> display_str;

    for(component& c : components)
    {
        std::string component_str = get_component_display_string(c);

        display_str.push_back(component_str);
    }

    return display_str;
}*/


ship ship::duplicate() const
{
    ship s = *this;
    s.id = ship::get_new_id();
    s.get_new_serialise_id();

    return s;
}

ship::ship()
{
    has_element.resize(ship_component_elements::NONE + 1);
    type_to_component_offsets.resize(ship_component_elements::NONE + 1);

    dim = {100, 40};
    cached_fully_merged = get_fully_merged(1.f);
}

void ship::sort_components()
{
    std::sort(entity_list.begin(), entity_list.end(), [](component& c1, component& c2)
              {
                    if(c1.ui_category != c2.ui_category)
                    {
                        return c1.ui_category < c2.ui_category;
                    }

                    return c1.primary_attribute < c2.primary_attribute;
              });
}

ship_type::types ship::estimate_ship_type()
{
    float total_scanner_size = 0.f;
    float total_weapon_size = 0.f;

    for(component& c : entity_list)
    {
        if(c.primary_attribute == ship_component_elements::COLONISER)
            return ship_type::COLONY;

        if(c.primary_attribute == ship_component_elements::ORE_HARVESTER)
            return ship_type::MINING;

        if(c.is_weapon())
        {
            total_weapon_size += c.current_size;
        }

        if(c.primary_attribute == ship_component_elements::SCANNING_POWER)
        {
            total_scanner_size += c.current_size;
        }
    }


    if(total_scanner_size == total_weapon_size)
    {
        return ship_type::SCOUT;
    }

    if(total_scanner_size > total_weapon_size)
    {
        return ship_type::SCOUT;
    }

    else if(total_scanner_size < total_weapon_size)
    {
        return ship_type::MILITARY;
    }

    ///just in case
    return ship_type::MILITARY;
}

void do_recrew_str(ship* s, empire* player_empire)
{
    auto res = s->resources_needed_to_recrew_total();

    ///crew research has 0 bound, scrapped research has minimum bound
    float recrew_research_currency = s->get_recrew_potential_research(player_empire).units_to_currency(false);

    resource_manager rm;

    for(auto& i : res)
    {
        rm.resources[i.first].amount = -i.second;
    }

    rm.render_tooltip(true);

    ImGui::Text(("Potential Re: " + std::to_string((int)recrew_research_currency)).c_str());
}

void do_recrew_str(ship_manager* sm, empire* player_empire)
{
    resource_manager rm;
    float recrew_research_currency = 0.f;

    for(ship* s : sm->ships)
    {
        if(!s->fully_disabled())
            continue;

        auto res = s->resources_needed_to_recrew_total();

        recrew_research_currency += s->get_recrew_potential_research(player_empire).units_to_currency(false);

        for(auto& i : res)
        {
            rm.resources[i.first].amount += -i.second;
        }
    }

    rm.render_tooltip(true);

    ImGui::Text(("Potential Re: " + std::to_string((int)recrew_research_currency)).c_str());;
}

std::map<resource::types, float> get_upgrade_cost(ship* s)
{
     ship c_cpy = *s;

    if(s->owned_by->parent_empire != nullptr)
        c_cpy.set_max_tech_level_from_empire_and_ship(s->owned_by->parent_empire);

    ///this doesn't include armour, so we're kind of getting it for free atm
    auto repair_resources = c_cpy.resources_needed_to_repair_total();

    //c_cpy.intermediate_texture = nullptr;

    auto res_cost = c_cpy.resources_cost();

    for(auto& i : repair_resources)
    {
        res_cost[i.first] += i.second;
    }

    auto current_resources = s->resources_cost();

    for(auto& i : current_resources)
    {
        res_cost[i.first] -= i.second;
    }

    return res_cost;
}

/*std::string get_upgrade_str(const std::map<resource::types, float>& cost)
{
    resource_manager rm;
    rm.add(cost);

    for(auto& i : rm.resources)
    {
        i.amount = -i.amount;
    }

    std::string str = rm.get_formatted_str(true);

    return str;
}*/

void do_upgrade_str(const std::map<resource::types, float>& cost)
{
    resource_manager rm;
    rm.add(cost);

    for(auto& i : rm.resources)
    {
        i.amount = -i.amount;
    }

    rm.render_tooltip(true);
}

///ok so. If we click anywhere outside the box we want to cancel
bool handle_rename(std::string& to_handle, bool going)
{
    if(!going)
        return false;

    to_handle.resize(100);

    bool term = ImGui::InputText("###renamer_context", &to_handle[0], 99, ImGuiInputTextFlags_EnterReturnsTrue);

    if(ImGui::IsItemActive())
    {
        ImGui::suppress_keyboard = true;
    }

    to_handle.resize(strlen(to_handle.c_str()));

    if(ImGui::IsMouseClicked(0) && !ImGui::IsItemClicked_Registered())
    {
        term = true;
    }

    return term;
}

///may need to migrate context handling above input handling
///so that keyboard input can be suppressed
void ship::context_handle_menu(orbital* o, empire* player_empire, fleet_manager& fleet_manage, popup_info& popup)
{
    context_tick_menu();

    if(context_request_open)
    {
        context_request_open = false;

        ImGui::OpenPopup("TestPopup");
    }

    if(!context_is_open)
        return;

    bool open = ImGui::BeginPopup("TestPopup");

    if(!open)
    {
        context_are_you_sure_war = false;
        context_are_you_sure_scrap = false;
        context_is_open = false;
        context_request_close = false;
        context_renaming = false;
        return;
    }

    bool friendly = owned_by->parent_empire == player_empire || player_empire->is_allied(owned_by->parent_empire);

    bool owned = owned_by->parent_empire == player_empire;

    bool no_hostiles_in_system = !o->hostiles_in_system();

    ///RESUPPLY
    if(friendly && !fully_disabled() && no_hostiles_in_system)
    {
        ImGui::OutlineHoverTextAuto("(Resupply)", popup_colour_info::good_ui_colour, true, {0,0}, 1, owned_by->auto_resupply);

        if(ImGui::IsItemClicked_Registered())
        {
            resupply(player_empire);
        }

        if(owned_by->parent_empire == player_empire)
        {
            if(ImGui::IsItemClicked_Registered(1))
            {
                owned_by->auto_resupply = !owned_by->auto_resupply;
            }

            if(ImGui::IsItemHovered())
            {
                if(owned_by->auto_resupply)
                    tooltip::add("Right click to disable auto resupply for fleet");
                else
                    tooltip::add("Right click to enable auto resupply for fleet");
            }
        }
    }

    ///REPAIR
    if(friendly && damaged() && !fully_disabled() && no_hostiles_in_system)
    {
        ImGui::GoodText("(Repair)");

        if(ImGui::IsItemClicked_Registered())
        {
            repair(player_empire);
        }
    }

    if(friendly && !fully_disabled() && no_hostiles_in_system)
    {
        ImGui::GoodText("(Refill Cargo)");

        if(ImGui::IsItemClicked_Registered())
        {
            refill_resources(player_empire);
        }
    }

    bool not_busy_and_in_friendly_territory = o->in_friendly_territory_and_not_busy();

    bool get_research = original_owning_race != player_empire;

    bool can_claim_hostile = (player_empire == o->parent_system->get_base()->parent_empire ||
                              player_empire->is_allied(o->parent_system->get_base()->parent_empire)) &&
                              !o->data->any_in_combat();

    if(!fully_disabled() && can_claim_hostile && owned_by->parent_empire == player_empire && can_be_upgraded() && no_hostiles_in_system)
    {
        ///this is expensive and involves a ship copy
        auto res = get_upgrade_cost(this);

        if(owned_by->parent_empire->can_fully_dispense(res))
        {
            ImGui::NeutralText("(Upgrade)");

            if(ImGui::IsItemClicked_Registered())
            {
                owned_by->parent_empire->dispense_resources(res);

                set_max_tech_level_from_empire_and_ship(owned_by->parent_empire);
            }
        }
        else
        {
            ImGui::BadText("(Upgrade)");
        }

        if(ImGui::IsItemHovered())
        {
            do_upgrade_str(res);
        }
    }

    if(fully_disabled() && can_claim_hostile && no_hostiles_in_system)
    {
        if(can_recrew(player_empire))
            ImGui::NeutralText("(Recrew)");
        else
            ImGui::BadText("(Recrew)");

        if(ImGui::IsItemHovered())
        {
            do_recrew_str(this, player_empire);
        }

        ///if originating empire is not the claiming empire, get some tech
        if(ImGui::IsItemClicked_Registered() && can_recrew(player_empire))
        {
            ///depletes resources
            ///should probably pull the resource stuff outside of here as there might be other sources of recrewing
            recrew_derelict(owned_by->parent_empire, player_empire);

            orbital* new_orbital = o->parent_system->make_fleet(fleet_manage, o->orbital_length, o->orbital_angle, player_empire);

            ship* s = new_orbital->data->make_new_from(player_empire, *this);
            s->name = name;

            cleanup = true;

            if(popup.going)
                popup.insert(new_orbital);
        }
    }

    if(((owned && not_busy_and_in_friendly_territory) || (fully_disabled() && can_claim_hostile)) && no_hostiles_in_system)
    {
        research research_raw;

        if(get_research)
        {
            research_raw = get_research_real_for_empire(owned_by->parent_empire, player_empire);
        }

        auto res = resources_received_when_scrapped();

        if(get_research)
        {
            res[resource::RESEARCH] = research_raw.units_to_currency(true);
        }

        resource_manager rm;

        rm.add(res);

        ImGui::BadText("(Scrap Ship)");

        if(ImGui::IsItemHovered())
        {
            rm.render_tooltip(true);
        }

        if(ImGui::IsItemClicked_Registered())
        {
            context_are_you_sure_scrap = true;
        }

        if(context_are_you_sure_scrap)
        {
            ImGui::BadText("(Are you sure?)");

            if(ImGui::IsItemClicked())
            {
                context_are_you_sure_scrap = false;

                for(auto& i : res)
                {
                    player_empire->add_resource(i.first, i.second);
                }

                cleanup = true;
            }
        }
    }

    bool neutral_or_allied = (!player_empire->is_hostile(owned_by->parent_empire) || player_empire->is_allied(owned_by->parent_empire)) && player_empire != owned_by->parent_empire;

    if(neutral_or_allied)
    {
        ImGui::BadText("(Declare War)");

        if(ImGui::IsItemClicked_Registered())
            context_are_you_sure_war = true;

        if(context_are_you_sure_war)
        {
            ImGui::BadText("(Are you sure?)");

            if(ImGui::IsItemClicked_Registered())
            {
                if(!player_empire->is_hostile(owned_by->parent_empire))
                {
                    player_empire->become_hostile(owned_by->parent_empire);
                }

                context_are_you_sure_war = false;
            }
        }
    }

    ///ordering of functions here is extremely strict
    ///designed to never let two items be drawn on the same frame
    ///also has to take into account that the item is clicked to open the input box
    if(owned)
    {
        bool cancel = handle_rename(name, context_renaming);

        if(!context_renaming)
        {
            ImGui::NeutralText("(Rename)");

            if(ImGui::IsItemClicked_Registered())
            {
                context_renaming = true;
            }
        }

        if(cancel)
        {
            context_renaming = false;
        }
    }

    if((ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(0)) && !ImGui::IsWindowHovered() && !ImGui::suppress_clicks || context_request_close)
    {
        ImGui::CloseCurrentPopup();
        ImGui::suppress_clicks = true;
        context_request_close = false;
    }

    ImGui::EndPopup();
}

void ship_manager::context_handle_menu(orbital* o, empire* player_empire, fleet_manager& fleet_manage, popup_info& popup)
{
    context_tick_menu();

    if(context_request_open)
    {
        context_request_open = false;

        ImGui::OpenPopup("TestPopup");
    }

    if(!context_is_open)
        return;

    bool open = ImGui::BeginPopup("TestPopup");

    if(!open)
    {
        context_are_you_sure_war = false;
        context_is_open = false;
        context_request_close = false;
        context_renaming = false;
        return;
    }

    bool friendly = parent_empire == player_empire || player_empire->is_allied(parent_empire);

    bool owned = parent_empire == player_empire;

    bool not_busy_and_in_friendly_territory = o->in_friendly_territory_and_not_busy();

    bool can_claim_hostile = (player_empire == o->parent_system->get_base()->parent_empire ||
                              player_empire->is_allied(o->parent_system->get_base()->parent_empire)) &&
                              !any_in_combat();

    bool no_hostiles_in_system = !o->hostiles_in_system();

    ///RESUPPLY
    if(friendly && !any_derelict() && no_hostiles_in_system)
    {
        ImGui::OutlineHoverTextAuto("(Resupply)", popup_colour_info::good_ui_colour, true, {0,0}, 1, auto_resupply);

        if(ImGui::IsItemClicked_Registered())
        {
            resupply(player_empire);
        }

        if(parent_empire == player_empire)
        {
            if(ImGui::IsItemClicked_Registered(1))
            {
                auto_resupply = !auto_resupply;
            }

            if(ImGui::IsItemHovered())
            {
                if(auto_resupply)
                    tooltip::add("Right click to disable auto resupply");
                else
                    tooltip::add("Right click to enable auto resupply");
            }
        }
    }

    ///REPAIR
    if(friendly && any_damaged() && !any_derelict() && no_hostiles_in_system)
    {
        ImGui::GoodText("(Repair)");

        if(ImGui::IsItemClicked_Registered())
        {
            repair(player_empire);
        }
    }

    if(friendly && !any_derelict() && no_hostiles_in_system)
    {
        ImGui::GoodText("(Refill Cargo)");

        if(ImGui::IsItemClicked_Registered())
        {
            refill_resources(player_empire);
        }
    }

    bool any_upgrade = false;

    for(ship* s : ships)
    {
        if(s->can_be_upgraded())
        {
            any_upgrade = true;
            break;
        }
    }

    if(any_upgrade && parent_empire == player_empire && can_claim_hostile && no_hostiles_in_system)
    {
        bool all_can_be_upgraded = true;

        std::map<resource::types, float> cost;

        for(ship* s : ships)
        {
            auto res = get_upgrade_cost(s);

            for(auto& i : res)
            {
                cost[i.first] += i.second;
            }
        }

        if(parent_empire->can_fully_dispense(cost))
        {
            ImGui::NeutralText("(Upgrade Fleet)");

            if(ImGui::IsItemClicked_Registered())
            {
                parent_empire->dispense_resources(cost);

                for(ship* s : ships)
                {
                    s->set_max_tech_level_from_empire_and_ship(parent_empire);
                }
            }
        }
        else
        {
            ImGui::BadText("(Upgrade Fleet)");
        }

        if(ImGui::IsItemHovered())
        {
            do_upgrade_str(cost);
        }
    }

    if(any_derelict() && can_claim_hostile && no_hostiles_in_system)
    {
        bool can_recrew_all = true;

        for(ship* s : ships)
        {
            if(s->fully_disabled() && !s->can_recrew(player_empire))
            {
                can_recrew_all = false;
                break;
            }
        }

        if(can_recrew_all)
            ImGui::NeutralText("(Recrew)");
        else
            ImGui::BadText("(Recrew)");

        if(ImGui::IsItemHovered())
        {
            do_recrew_str(this, player_empire);
        }

        if(ImGui::IsItemClicked_Registered() && can_recrew_all)
        {
            bool can_recrew_any = false;

            for(ship* s : ships)
            {
                if(s->can_recrew(player_empire))
                {
                    can_recrew_any = true;
                    break;
                }
            }

            if(can_recrew_any)
            {
                orbital* new_orbital = o->parent_system->make_fleet(fleet_manage, o->orbital_length, o->orbital_angle, player_empire);

                for(ship* s : ships)
                {
                    if(!s->fully_disabled())
                        continue;

                    ///if originating empire is not the claiming empire, get some tech
                    if(ImGui::IsItemClicked_Registered() && s->can_recrew(player_empire))
                    {
                        ///depletes resources
                        ///should probably pull the resource stuff outside of here as there might be other sources of recrewing
                        s->recrew_derelict(s->owned_by->parent_empire, player_empire);

                        ship* ns = new_orbital->data->make_new_from(player_empire, *s);
                        ns->name = s->name;

                        s->cleanup = true;

                        if(popup.going)
                            popup.insert(new_orbital);
                    }
                }

            }
        }
    }

    bool neutral_or_allied = (!player_empire->is_hostile(parent_empire) || player_empire->is_allied(parent_empire)) && player_empire != parent_empire;

    if(neutral_or_allied)
    {
        ImGui::BadText("(Declare War)");

        if(ImGui::IsItemClicked_Registered())
            context_are_you_sure_war = true;

        if(context_are_you_sure_war)
        {
            ImGui::BadText("(Are you sure?)");

            if(ImGui::IsItemClicked_Registered())
            {
                if(!player_empire->is_hostile(parent_empire))
                {
                    player_empire->become_hostile(parent_empire);
                }

                context_are_you_sure_war = false;
            }

        }
    }

    if((owned && not_busy_and_in_friendly_territory) || (all_derelict() && can_claim_hostile) && no_hostiles_in_system)
    {
        resource_manager rm;
        std::map<resource::types, float> accum_res;

        for(ship* s : ships)
        {
            bool get_research = s->original_owning_race != player_empire;
            research research_raw;

            if(get_research)
            {
                research_raw = s->get_research_real_for_empire(parent_empire, player_empire);
            }

            auto res = s->resources_received_when_scrapped();

            if(get_research)
            {
                res[resource::RESEARCH] = research_raw.units_to_currency(true);
            }

            for(auto& i : res)
            {
                rm.resources[i.first].amount += i.second;
                accum_res[i.first] += i.second;
            }
        }

        ImGui::BadText("(Scrap All Ships)");

        std::string rstr = rm.get_formatted_str(true);

        if(ImGui::IsItemHovered())
        {
            tooltip::add(rstr);
        }

        if(ImGui::IsItemClicked_Registered())
        {
            context_are_you_sure_scrap = true;
        }

        if(context_are_you_sure_scrap)
        {
            ImGui::BadText("(Are you sure?)");

            if(ImGui::IsItemClicked())
            {
                context_are_you_sure_scrap = false;

                for(auto& i : accum_res)
                {
                    player_empire->add_resource(i.first, i.second);
                }

                for(ship* s : ships)
                {
                    s->cleanup = true;
                }
            }
        }
    }

    if(owned)
    {
        bool cancel = handle_rename(o->name, context_renaming);

        if(!context_renaming)
        {
            ImGui::NeutralText("(Rename)");

            if(ImGui::IsItemClicked_Registered())
            {
                context_renaming = true;
            }
        }

        if(cancel)
        {
            context_renaming = false;
        }
    }

    if((ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(0)) && !ImGui::IsWindowHovered() && !ImGui::suppress_clicks || context_request_close)
    {
        ImGui::CloseCurrentPopup();
        ImGui::suppress_clicks = true;
        context_request_close = false;
    }

    ImGui::EndPopup();
}

void ship::tick_all_components(float step_s)
{
    auto timer = MAKE_AUTO_TIMER();

    timer.start();

    //if(step_s < 0.1)
    //    step_s = 0.1;

    for(auto& i : entity_list)
    {
        i.update_time(step_s);
    }

    ///this covers the drain side
    /*std::map<ship_component_element, float> stored_and_produced = get_stored_and_produced_resources(step_s);
    std::map<ship_component_element, float> needed = get_needed_resources(step_s);
    std::map<ship_component_element, float> produced = get_produced_resources(step_s);
    std::map<ship_component_element, float> stored = get_stored_resources();*/

    std::vector<component_attribute> fully_merge = get_fully_merged(step_s);
    std::vector<component_attribute> fully_merge_no_eff_with_hpfrac = get_fully_merged_no_efficiency_with_hpfrac(step_s);


    ///HACK ALERT
    if(fully_disabled())
    {
        fully_merge[ship_component_elements::HP].produced_per_s = 0;
    }

    /*

    for(component& c : entity_list)
    {
        //to_repair_sorted.push_back(c.primary_attribute);
    }

    if(in_combat())
    {
        to_repair_sorted.resize(ship_component_elements::repair_priorities_in_combat.size());

        for(component& c : entity_list)
        {
            if(c.primary_attribute == ship_component_elements::NONE)
                continue;
        }
    }
    else
    {

    }*/

    auto timer_initial = MAKE_AUTO_TIMER();
    timer_initial.start();

    std::vector<component_attribute*> attributes[ship_component_elements::NONE];

    for(auto& i : entity_list)
    {
        for(int type = 0; type < i.components.size(); type++)
        {
            component_attribute& attr = i.components[type];

            if(!attr.present)
                continue;

            attributes[type].push_back(&attr);
        }
    }

    timer_initial.finish();

    auto timer_repair2 = MAKE_AUTO_TIMER();
    timer_repair2.start();

    std::vector<std::vector<component*>> to_repair_sorted;

    std::vector<int>* ref_vec;

    if(in_combat())
    {
        ref_vec = &ship_component_elements::repair_in_combat_map;

    }
    else
    {
        ref_vec = &ship_component_elements::repair_out_combat_map;
    }

    to_repair_sorted.resize(ref_vec->size());

    for(component& c : entity_list)
    {
        int val = (*ref_vec)[c.primary_attribute];

        if(val != -1)
        {
            to_repair_sorted[val].push_back(&c);
        }
    }

    timer_repair2.finish();

    auto timer_breakdown = MAKE_AUTO_TIMER();
    timer_breakdown.start();

    for(auto& i : to_repair_sorted)
    {
        for(auto& kk : i)
        {
            component& c = *kk;

            ///due to skip check
            if(c.primary_attribute == ship_component_elements::NONE)
                continue;

            ///skipping efficiency on producing_ps as it means something else is the bottleneck
            ///and we'd probably gain more by repairing that as that's the limiting factor
            ///want to factor hp efficiency in
            float producing_ps = fully_merge_no_eff_with_hpfrac[c.primary_attribute].produced_per_s;
            float using_ps = fully_merge[c.primary_attribute].drained_per_s;

            if(producing_ps > using_ps && ship_component_elements::allowed_skip_repair[c.primary_attribute] != -1)
                continue;

            bool in_starvation = false;


            if(c.has_tag(component_tag::DAMAGED_WITHOUT_O2))
            {
                float val = c.get_tag(component_tag::OXYGEN_STARVATION);

                if(val > 0.99f && val < 1.01f)
                {
                    continue;
                }
            }

            float available = fully_merge[ship_component_elements::HP].produced_per_s;


            component_attribute& hp_attr = c.components[ship_component_elements::HP];

            float damage = hp_attr.get_available_capacity();

            float to_consume = std::min(damage, available);

            hp_attr.add_amount(to_consume);

            available -= to_consume;

            fully_merge[ship_component_elements::HP].produced_per_s = available;
        }
    }

    timer_breakdown.finish();

    auto timer_owned = MAKE_AUTO_TIMER();
    timer_owned.start();

    ///do resource pulling from empire
    if(owned_by != nullptr && owned_by->parent_empire != nullptr)
    {
        //auto available = get_available_capacities_vec();
        auto available = get_available_capacities_linear_vec();

        const std::vector<int>& component_offsets = type_to_component_offsets[ship_component_elements::RESOURCE_PULLER];

        for(int component_offset : component_offsets)
        {
            component& c = entity_list[component_offset];

            component_attribute& cattr = c.components[ship_component_elements::RESOURCE_PULLER];

            float resources_to_dispense = cattr.get_produced_amount(step_s);

            for(int type = 0; type < c.components.size(); type++)
            {
                component_attribute& attr = c.components[type];

                if(!attr.present)
                    continue;

                auto res_type = ship_component_elements::element_infos[(int)type].resource_type;

                if(res_type == resource::COUNT)
                    continue;

                attr.produced_per_s = 0.f;

                float to_dispense = resources_to_dispense;

                to_dispense = std::min(to_dispense, available[(int)type]);

                bool valid = owned_by->in_friendly_territory && !owned_by->any_in_combat();

                if(valid && owned_by->parent_empire->can_fully_dispense(res_type, to_dispense))
                {
                    owned_by->parent_empire->dispense_resource(res_type, to_dispense);

                    attr.produced_per_s = to_dispense / step_s;
                }
            }
        }
    }

    timer_owned.finish();

    auto timer_toapply = MAKE_AUTO_TIMER();
    timer_toapply.start();

    ///DIRTY HACK ALERT

    //std::map<ship_component_element, float> to_apply_prop;

    /*std::vector<float> to_apply_prop;
    to_apply_prop.resize(ship_component_elements::NONE + 1);*/

    float to_apply_prop[ship_component_elements::NONE + 1] = {};

    //for(auto& i : needed)

    int type = 0;

    ///so next up, we want drained to only actually drain a % of its usage based on cur production but maintain 100%
    ///I guess we could do the similar opposite of currently drained (with current production drained, or we calc efficiency as)
    ///available / production. We could just cheat and make max available every time, and update drain efficiency
    ///based on how much is used. Straightforward, minimal hackiness
    ///we'll need to play it into storage somehow as well
    ///Ignore my brain when it says drain, is unncessary faff
    int fm_size = fully_merge.size();
    for(int type = 0; type < fm_size; type++)
    {
        if(fully_merge[type].produced_per_s <= FLOAT_BOUND || fully_merge[type].drained_per_s <= FLOAT_BOUND)
            continue;

        float frac = fully_merge[type].drained_per_s / fully_merge[type].produced_per_s;

        to_apply_prop[type] = frac;
    }

    ///change to take first from production, then from storage instead of weird proportional
    ///optimise this bit. Take out the n^2 if we can!
    ///if we reduce each list down to linear first, then the n^2 might be much smaller, then apply the linear result to all the components
    ///would also have the added side effect that all production would be eated proportionally
    for(int i=0; i<ship_component_elements::NONE; i++)
    {
        const std::vector<component_attribute*>& current_set = attributes[i];

        for(int kk = 0; kk < current_set.size(); kk++)
        {
            float my_proportion_of_total = to_apply_prop[i];
            float frac = my_proportion_of_total;

            component_attribute& me = *current_set[kk];

            float extra = 0;

            for(int jj = 0; jj < current_set.size(); jj++)
            {
                component_attribute& other = *current_set[jj];

                float other_production = other.get_produced_amount(step_s);

                if(other_production == 0)
                    continue;

                float take_amount = frac * other_production + extra;

                ///for fractional drainage
                if(frac > 1)
                {
                    take_amount = (1.f / frac) * me.get_drain_capacity(step_s) + extra;
                }

                if(take_amount > me.get_total_capacity(step_s))
                    take_amount = me.get_total_capacity(step_s);

                ///ie the amount we actually took from other
                float drained = me.consume_from_amount_available(other, take_amount, step_s);

                fully_merge[i].produced_per_s -= drained;
                fully_merge[i].drained_per_s -= drained;

                ///the conditional fixes specifically fractional drainage
                if(frac <= 1)
                    extra += (take_amount - drained);
            }
        }
    }

    for(auto& i : to_apply_prop)
    {
        i = 0;
    }

    int cur = 0;

    for(auto& i : fully_merge)
    {
        if(i.cur_amount <= FLOAT_BOUND)
        {
            cur++;
            continue;
        }

        if(i.drained_per_s <= FLOAT_BOUND)
        {
            cur++;
            continue;
        }
        ///no produced left if we're dipping into stored
        //float frac = stored[i.first] / i.second;

        ///take frac from all
        ///ok we can't just use stored because there might still be some left in the produced section
        ///because we're taking proportionally
        float frac = i.drained_per_s / i.cur_amount;

        to_apply_prop[cur] = frac;

        cur++;
    }

    timer_toapply.finish();

    auto t2 = MAKE_AUTO_TIMER();
    t2.start();

    for(int i=0; i<ship_component_elements::NONE; i++)
    {
        const std::vector<component_attribute*>& current_set = attributes[i];

        for(int kk = 0; kk < current_set.size(); kk++)
        {
            float my_proportion_of_total = to_apply_prop[i];
            float frac = my_proportion_of_total;

            if(frac > 1)
                frac = 1;

            component_attribute& me = *current_set[kk];

            float extra = 0;

            for(int jj = 0; jj < current_set.size(); jj++)
            {
                component_attribute& other = *current_set[jj];

                float take_amount = frac * other.cur_amount + extra;

                if(take_amount > me.get_total_capacity(step_s))
                    take_amount = me.get_total_capacity(step_s);

                ///ie the amount we actually took from other
                float drained = me.consume_from_amount_stored(other, take_amount, step_s);

                extra += (take_amount - drained);
            }
        }
    }


    /*for(auto& i : needed)
    {
        if()
    }*/

    for(auto& i : fully_merge)
    {
        //if(i.produced_per_s < -0.0001f)
        if(i.produced_per_s < 0)
        {
            //printf("Logic error somewhere\n");

            i.produced_per_s = 0;
        }
    }

    t2.finish();

    //printf("leftover %f\n", produced[ship_component_element::ENERGY]);

    auto t3 = MAKE_AUTO_TIMER();
    t3.start();

    //std::map<ship_component_element, float> available_capacities = get_available_capacities();

    auto left_after_storage = fully_merge;

    auto available_capacities = get_available_capacities_linear_vec();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        auto this_entity_available = c.get_available_capacities_linear_vec();

        //for(auto& i : this_entity_available)
        for(int kk=0; kk < this_entity_available.size(); kk++)
        {
            if(available_capacities[kk] <= FLOAT_BOUND)
                continue;

            float proportion = this_entity_available[kk] / available_capacities[kk];

            float applying_to_this = proportion * fully_merge[kk].produced_per_s;


            left_after_storage[kk].produced_per_s -= applying_to_this;

            left_after_storage[kk].produced_per_s = std::max(left_after_storage[kk].produced_per_s, 0.f);

            ///this is slow
            //std::map<ship_component_element, float> tmap;

            //tmap[i.first] = applying_to_this;

            /*if(i.first == ship_component_elements::OXYGEN)
            {
                printf("test %f\n", applying_to_this);
            }*/

            //auto r = c.apply_diff(tmap);

            c.apply_diff_single((ship_component_element)kk, applying_to_this);

            ///can be none left over as we're using available capacities
            //auto left_over = r;
        }
    }


    for(component& c : entity_list)
    {
        if(c.has_tag(component_tag::DAMAGED_WITHOUT_O2))
        {
            float eff_without_hp = c.components[ship_component_element::OXYGEN].get_efficiency(step_s);

            if(eff_without_hp < 1)
            {
                float damage = (1.f - eff_without_hp) * step_s * c.get_tag(component_tag::DAMAGED_WITHOUT_O2);

                c.apply_diff_single(ship_component_element::HP, -damage);

                c.set_tag(component_tag::OXYGEN_STARVATION, 1.f);
            }
            else
            {
                c.set_tag(component_tag::OXYGEN_STARVATION, 0.f);
            }

        }

        c.propagate_total_efficiency(step_s);
    }

    ///and in friendly territory?
    ///atm this is going to double produce if we have storage for this resource
    if(owned_by != nullptr && owned_by->parent_empire != nullptr)
    {
        empire* parent_empire = owned_by->parent_empire;

        for(int i=0; i < left_after_storage.size(); i++)
        {
            if(ship_component_elements::element_infos[i].resource_type != resource::COUNT)
            {
                float produced = left_after_storage[i].produced_per_s;

                owned_by->parent_empire->add_resource(ship_component_elements::element_infos[i].resource_type, produced);
            }
        }
    }

    for(component& c : entity_list)
    {
        if(!c.has_element(ship_component_elements::ORE_HARVESTER))
            continue;

        for(int i=0; i<ship_component_elements::element_infos.size(); i++)
        {
            if(ship_component_elements::element_infos[i].resource_type == resource::COUNT)
                continue;

            c.components[(ship_component_elements::types)i].produced_per_s = 0.f;
        }
    }

    t3.finish();


    /*for(component& c : entity_list)
    {
        if(!c.has_element(ship_component_element::RESOURCE_STORAGE))
            continue;

        component_attribute attr = c.get_element(ship_component_element::RESOURCE_STORAGE);

        float summed_res = 0.f;

        for(auto& res : attr.resources_cur_stored)
        {
            summed_res += res.second;
        }

        c.components[ship_component_element::RESOURCE_STORAGE].cur_amount = summed_res;
    }*/

    timer.finish();

    ///hooray! Cheat and blatantly get this for free :)
    cached_fully_merged = std::move(fully_merge);

    ///so amount left over is total_to_apply - available_capacities

    /*std::map<ship_component_element, float> left_over;

    for(auto& i : produced)
    {
        left_over[i.first] = i.second - available_capacities[i.first];
    }*/


    ///so now we've put production into storage,

    ///this is capacites shit, ignore me
   // return left_over;
}

void ship::tick_other_systems(float step_s)
{
    auto timer = MAKE_AUTO_TIMER();
    timer.start();

    disengage_clock_s += step_s;

    test_set_disabled();

    if(is_alien)
        past_owners_research_left[owned_by->parent_empire] = research_left_from_crewing;
}

research ship::tick_drain_research_from_crew(float step_s)
{
    research ret;

    if(!is_alien)
        return ret;

    if(research_left_from_crewing.units_to_currency(false) <= 0.0001f)
        return ret;

    for(int i=0; i<research_left_from_crewing.categories.size(); i++)
    {
        research_category& cat = research_left_from_crewing.categories[i];

        float amount_to_drain = 1.f * step_s * 0.01f;

        amount_to_drain = std::min(amount_to_drain, cat.amount);

        ret.add_amount(cat.type, amount_to_drain);

        cat.amount -= amount_to_drain;

        cat.amount = std::max(cat.amount, 0.f);
    }

    return ret;
}

void ship::tick_combat(float step_s)
{
    time_in_combat_s += step_s;
}

void ship::enter_combat()
{
    if(currently_in_combat)
        return;

    time_in_combat_s = 0.f;
    currently_in_combat = true;
}

void ship::leave_combat()
{
    currently_in_combat = false;
    //owned_by->requesting_or_in_battle = false;
}

bool ship::in_combat()
{
    return currently_in_combat;
}

std::vector<float> ship::get_produced(float step_s)
{
    std::vector<float> ret;

    ret.resize((int)ship_component_elements::NONE);

    for(const auto& i : entity_list)
    {
        for(int type = 0; type < i.components.size(); type++)
        {
            const component_attribute& attr = i.components[type];

            if(!attr.present)
                continue;

            float& which = ret[type];

            const component_attribute& other = attr;

            which += other.produced_per_s * step_s * other.cur_efficiency;
        }
    }

    return ret;
}

std::vector<component_attribute> ship::get_fully_merged(float step_s)
{
    std::vector<component_attribute> ret;

    ret.resize((int)ship_component_elements::NONE);

    for(const auto& i : entity_list)
    {
        for(int type = 0; type < i.components.size(); type++)
        {
            const component_attribute& attr = i.components[type];

            if(!attr.present)
                continue;

            component_attribute& which = ret[type];

            const component_attribute& other = attr;

            which.cur_amount += other.cur_amount;
            which.max_amount += other.max_amount;

            which.produced_per_s += other.produced_per_s * step_s * other.cur_efficiency;
            which.drained_per_s += other.drained_per_s * step_s;
        }
    }

    return ret;
}

component_attribute ship::get_fully_merged_single(const ship_component_element& type, float step_s)
{
    component_attribute ret;

    for(const auto& i : entity_list)
    {
        const component_attribute& attr = i.components[type];

        if(!attr.present)
            continue;

        component_attribute& which = ret;

        const component_attribute& other = attr;

        which.cur_amount += other.cur_amount;
        which.max_amount += other.max_amount;

        which.produced_per_s += other.produced_per_s * step_s * other.cur_efficiency;
        which.drained_per_s += other.drained_per_s * step_s;
    }

    return ret;
}

std::vector<component_attribute> ship::get_fully_merged_no_efficiency_with_hpfrac(float step_s)
{
    std::vector<component_attribute> ret;

    ret.resize((int)ship_component_elements::NONE);

    for(auto& i : entity_list)
    {
        float hp_eff = 1.f;

        if(i.has_element(ship_component_element::HP))
        {
            component_attribute elem = i.get_element(ship_component_element::HP);

            hp_eff = elem.cur_amount / elem.max_amount;
        }

        for(int type = 0; type < i.components.size(); type++)
        {
            component_attribute& attr = i.components[type];

            if(!attr.present)
                continue;

            component_attribute& which = ret[type];

            component_attribute& other = attr;

            which.cur_amount += other.cur_amount;
            which.max_amount += other.max_amount;

            which.produced_per_s += other.produced_per_s * step_s * hp_eff;
            which.drained_per_s += other.drained_per_s * step_s;
        }
    }

    return ret;
}

/*std::map<ship_component_element, float> ship::get_available_capacities()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_available_capacities());
    }

    return ret;
}*/

std::vector<std::pair<ship_component_element, float>> ship::get_available_capacities_vec()
{
    std::vector<std::pair<ship_component_element, float>> ret;

    for(int i=0; i<(int)ship_component_elements::NONE; i++)
    {
        ret.push_back({(ship_component_element)i, 0.f});
    }

    for(component& c : entity_list)
    {
        for(int i=0; i<(int)ship_component_elements::NONE; i++)
        {
            if(!c.components[i].present)
                continue;

            component_attribute& attr = c.components[i];

            ret[i].second += attr.max_amount - attr.cur_amount;

            //ret.push_back({(ship_component_elements::types)i, c.max_amount - c.cur_amount});
        }
    }

    return ret;
}

std::vector<float> ship::get_available_capacities_linear_vec()
{
    std::vector<float> ret;
    ret.resize(ship_component_elements::NONE);

    for(component& c : entity_list)
    {
        for(int i=0; i<(int)ship_component_elements::NONE; i++)
        {
            if(!c.components[i].present)
                continue;

            component_attribute& attr = c.components[i];

            ret[i] += attr.max_amount - attr.cur_amount;
        }
    }

    return ret;
}

/*std::map<ship_component_element, float> ship::get_needed_resources()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_needed_resources());
    }

    return ret;
}

std::map<ship_component_element, float> ship::get_produced_resources()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_produced_resources());
    }

    return ret;
}*/

std::map<ship_component_element, float> ship::get_stored_and_produced_resources(float time_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_stored_and_produced_resources(time_s));
    }

    return ret;
}

std::map<ship_component_element, float> ship::get_needed_resources(float time_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_timestep_consumption_diff(time_s));
    }

    return ret;
}

std::map<ship_component_element, float> ship::get_produced_resources(float time_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_timestep_production_diff(time_s));
    }

    return ret;
}

std::map<ship_component_element, float> ship::get_stored_resources()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_stored());
    }

    return ret;
}

std::map<ship_component_element, float> ship::get_max_resources()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_stored_max());
    }

    return ret;
}

/*void ship::update_efficiency()
{
    auto needed = get_needed_resources();
    auto produced = get_produced_resources();
}*/

void ship::refill_all_components()
{
    for(component& c : entity_list)
    {
        for(int type = 0; type < c.components.size(); type++)
        {
            component_attribute& attr = c.components[type];

            if(!attr.present)
                continue;

            attr.cur_amount = attr.max_amount;
            attr.time_last_used_s = 0.f;
            attr.current_time_s = attr.time_last_used_s + attr.time_between_uses_s + 1.f;
        }
    }
}

std::map<ship_component_element, float> ship::get_use_frac(component& c)
{
    std::map<ship_component_element, float> use_diff = c.get_use_diff();
    std::map<ship_component_element, float> available_diff = get_stored_resources();

    std::map<ship_component_element, float> ret;

    for(auto& i : use_diff)
    {
        //printf("%f %f %i\n", i.second, available_diff[i.first], i.first);

        if(fabs(i.second) < FLOAT_BOUND)
            continue;

        float frac = fabs(available_diff[i.first]) / fabs(i.second);

        frac = clamp(frac, 0.f, 1.f);

        ret[i.first] = frac;
    }

    return ret;
}

float ship::get_min_use_frac(component& c)
{
    std::map<ship_component_element, float> use_diff = c.get_use_diff();
    std::map<ship_component_element, float> available_diff = get_stored_resources();

    int num = 0;
    float accum = 1.f;

    for(auto& i : use_diff)
    {
        //printf("%f %f %i\n", i.second, available_diff[i.first], i.first);

        if(fabs(i.second) < FLOAT_BOUND)
            continue;

        float frac = fabs(available_diff[i.first]) / fabs(i.second);

        frac = clamp(frac, 0.f, 1.f);

        //accum += frac;

        accum = std::min(accum, frac);

        num++;
    }

    //if(num > 0)
    //    accum /= num;

    if(num == 0)
        accum = 0.f;

    return accum;
}

bool ship::can_use(component& c)
{
    if(fully_disabled())
        return false;

    std::map<ship_component_element, float> requirements;

    ///+ve means we need input from the ship to power it
    for(int type = 0; type < c.components.size(); type++)
    {
        component_attribute& attr = c.components[type];

        if(!attr.present)
            continue;

        requirements[(ship_component_element)type] = attr.drained_per_use - attr.produced_per_use;

        if(!attr.can_use())
            return false;
    }

    //auto stored = get_stored_resources();

    auto fully_merged = get_fully_merged(1.f);

    for(auto& i : requirements)
    {
        if(fully_merged[i.first].cur_amount < requirements[i.first])
            return false;
    }

    return true;
}

void ship::use(component& c)
{
    if(!can_use(c))
        return;

    auto diff = c.get_use_diff();

    for(int type = 0; type < c.components.size(); type++)
    {
        component_attribute& attr = c.components[type];

        if(!attr.present)
            continue;

        attr.use();
    }

    //add_negative_resources(diff);

    std::map<ship_component_element, float> positive;
    std::map<ship_component_element, float> negative;

    for(auto& i : diff)
    {
        if(i.second < 0)
        {
            negative[i.first] = i.second;
        }
        else
        {
            positive[i.first] = i.second;
        }
    }

    distribute_resources(positive);
    add_negative_resources(negative);
}

component* ship::get_component_with_primary(ship_component_elements::types type)
{
    for(component& c : entity_list)
    {
        if(c.primary_attribute == type)
            return &c;
    }

    return nullptr;
}

component* ship::get_component_with(ship_component_elements::types type)
{
    for(component& c : entity_list)
    {
        if(c.has_element(type))
            return &c;
    }

    return nullptr;
}

std::vector<component> ship::fire()
{
    std::vector<component> ret;

    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_element::RAILGUN) ||
           c.has_element(ship_component_element::COILGUN) ||
           c.has_element(ship_component_element::PLASMAGUN) ||
           c.has_element(ship_component_element::TORPEDO))
        {
            if(can_use(c))
            {
                use(c);

                ret.push_back(c);
            }
        }
    }

    return ret;
}

bool ship::can_use_warp_drives()
{
    if(fully_disabled())
        return false;

    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_element::WARP_POWER))
        {
            if(!can_use(c))
            {
                return false;
            }
        }
    }

    return true;
}

void ship::use_warp_drives()
{
    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_element::WARP_POWER))
        {
            if(can_use(c))
            {
                use(c);
            }
        }
    }
}

float ship::get_warp_distance()
{
    float rad = 0.f;
    float ship_mass = current_size;

    for(component& c : entity_list)
    {
        if(!c.has_tag(component_tag::WARP_DISTANCE))
            continue;

        float val = c.get_tag(component_tag::WARP_DISTANCE) / ship_mass;

        rad = std::max(rad, val);
    }

    return rad;
}

///this is slow
void ship::distribute_resources(std::map<ship_component_element, float> res)
{
    auto available_capacities = get_available_capacities_vec();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        const auto& this_entity_available = c.get_available_capacities_vec();

        for(const auto& i : this_entity_available)
        {
            if(available_capacities[(int)i.first].second <= FLOAT_BOUND)
                continue;

            float proportion = i.second / available_capacities[i.first].second;

            float applying_to_this = proportion * res[i.first];

            std::map<ship_component_element, float> tmap;

            tmap[i.first] = applying_to_this;

            auto r = c.apply_diff(tmap);

            ///can be none left over as we're using available capacities
            auto left_over = r;
        }
    }

    /*std::vector<std::pair<int, float>> proportions;

    for(int i=0; i<entity_list.size(); i++)
    {
        component& c = entity_list[i];

        const auto& this_entity_available = c.get_available_capacities();
    }*/
}

void ship::add_negative_resources(std::map<ship_component_element, float> res)
{
    std::map<ship_component_element, float> available_capacities = get_stored_resources();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        std::map<ship_component_element, float> this_entity_available = c.get_stored();

        for(auto& i : this_entity_available)
        {
            if(available_capacities[i.first] <= FLOAT_BOUND)
                continue;

            float proportion = i.second / available_capacities[i.first];

            float applying_to_this = proportion * res[i.first];

            std::map<ship_component_element, float> tmap;

            tmap[i.first] = applying_to_this;

            /*if(i.first == ship_component_elements::ENERGY)
            {
                printf("test %f\n", applying_to_this);
            }*/

            auto r = c.apply_diff(tmap);

            ///can be none left over as we're using available capacities
            auto left_over = r;
        }
    }
}

void ship::add(const component& c)
{
    //primary_to_component_offsets[c.primary_attribute].push_back(entity_list.size());

    for(int type = 0; type < c.components.size(); type++)
    {
        const component_attribute& attr = c.components[type];

        if(!attr.present)
            continue;

        type_to_component_offsets[(ship_component_element)type].push_back(entity_list.size());
        has_element[type] = 1;
    }

    entity_list.push_back(c);

    estimated_type = estimate_ship_type();
}

void ship::hit(projectile* p, orbital* associated)
{
    return hit_raw_damage(p->base.get_tag(component_tag::DAMAGE), p->fired_by, p->ship_fired_by, associated);
}

void distribute_damage(float hp_damage, int num, ship* s, bool hit_crew = false)
{
    for(int i=0; i<num; i++)
    {
        int tcp = 0;

        do
        {
            tcp = randf_s(0.f, s->entity_list.size());
        }
        while(s->entity_list[tcp].primary_attribute == ship_component_elements::COMMAND && !hit_crew);

        std::map<ship_component_element, float> nhp_diff;
        nhp_diff[ship_component_element::HP] = -hp_damage / num;

        s->entity_list[tcp].apply_diff(nhp_diff);

        //printf("rejig %i\n", i);
    }
}

///should we spread damage over multiple components?
void ship::hit_raw_damage(float damage, empire* hit_by, ship* ship_hit_by, orbital* associated)
{
    float shields = get_stored_resources()[ship_component_element::SHIELD_POWER];
    float armour = get_stored_resources()[ship_component_element::ARMOUR];

    float sdamage = std::min(damage, shields);

    float raw_armour_damage = damage - sdamage;

    float armour_damage = std::min(raw_armour_damage, armour);

    float hp_damage = raw_armour_damage - armour_damage;

    std::map<ship_component_element, float> diff;
    diff[ship_component_element::SHIELD_POWER] = -sdamage;
    diff[ship_component_element::ARMOUR] = -armour_damage;

    add_negative_resources(diff);

    if(entity_list.size() == 0)
    {
        printf("filthy degenerate in ship::hit\n");
    }

    int random_component = (int)randf_s(0.f, entity_list.size());

    component* h = &entity_list[random_component];

    ///make it less likely that crew will die
    if(h->primary_attribute == ship_component_elements::COMMAND)
    {
        std::map<ship_component_element, float> hp_diff;
        hp_diff[ship_component_element::HP] = -hp_damage/2;

        h->apply_diff(hp_diff);

        distribute_damage(hp_damage/2, 1, this);

        test_set_disabled();

        if(hit_by == nullptr || ship_hit_by == nullptr || associated == nullptr)
            return;

        empire* being_hit_empire = owned_by->parent_empire;

        float information = being_hit_empire->available_scanning_power_on(associated);

        if(information <= 0)
            return;

        hit_by->propagage_relationship_modification_from_damaging_ship(being_hit_empire);

        return;
    }

    /*if(h->get_available_capacities()[ship_component_elements::HP] < 0.0001f)
    {
        distribute_damage(hp_damage, 2, this);

        return;
    }*/

    std::map<ship_component_element, float> hp_diff;
    hp_diff[ship_component_element::HP] = -hp_damage;

    auto res = h->apply_diff(hp_diff);

    float leftover = res[ship_component_element::HP];

    distribute_damage(-leftover, 1, this);

    test_set_disabled();

    empire* being_hit_empire = owned_by->parent_empire;

    if(hit_by == nullptr || ship_hit_by == nullptr || associated == nullptr)
        return;

    ///do faction relations here because they might change later
    float information = being_hit_empire->available_scanning_power_on(associated);

    if(information <= 0)
        return;

    hit_by->propagage_relationship_modification_from_damaging_ship(being_hit_empire);
}

void set_center_sfml(sf::RectangleShape& shape)
{
    shape.setOrigin(shape.getLocalBounds().width/2.f, shape.getLocalBounds().height/2.f);
}

void ship::check_load(vec2i dim)
{
    if(is_loaded)
        return;

    generate_image(dim);
}

sf::RenderTexture* ship::intermediate = nullptr;

void ship::generate_image(vec2i dim)
{
    float min_bound = std::min(dim.x(), dim.y());

    tex.setSmooth(true);

    ///this is whats slow on adding ships in combat
    ///also crashes for some reason. Too many contexts?
    ///should do this all manually
    /*intermediate_texture = new sf::RenderTexture;
    intermediate_texture->create(dim.x(), dim.y());
    intermediate_texture->setSmooth(true);*/

    if(intermediate == nullptr)
    {
        intermediate = new sf::RenderTexture;

        if(intermediate->getSize().x < dim.x() || intermediate->getSize().y < dim.y())
        {
            intermediate->create(dim.x(), dim.y());

            intermediate->setSmooth(true);
        }
    }

    intermediate->clear(sf::Color(0,0,0,0));


    sf::BlendMode blend(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero);

    sf::RectangleShape r1;

    r1.setFillColor(sf::Color(255, 255, 255));
    r1.setSize({dim.x(), dim.y()});
    r1.setPosition(0, 0);

    //intermediate_texture->draw(r1);

    intermediate->draw(r1);


    int granularity = min_bound/4;

    sf::RectangleShape corner;
    corner.setSize({granularity, granularity});
    corner.setFillColor(sf::Color(0,0,0,0));
    corner.setOrigin(corner.getLocalBounds().width/2, corner.getLocalBounds().height/2);

    vec2f corners[] = {{0,0}, {0, dim.y()}, {dim.x(), 0}, {dim.x(), dim.y()}};
    float rotations[] = {0, -M_PI/2, M_PI/2, M_PI};
    bool done[] = {false, false, false, false};

    int num_corners = randf_s(1, 5);

    std::vector<vec2f> exclusion;

    for(int i=0; i<num_corners; i++)
    {
        int element = randf_s(0, 4);

        while(done[element])
            element = randf_s(0, 4);

        vec2f c = corners[element];
        float rot = rotations[element] + M_PI/4;

        exclusion.push_back(c);

        corner.setPosition(c.x(), c.y());
        corner.setRotation(r2d(rot));

        intermediate->draw(corner, blend);
    }

    sf::RectangleShape chunk = corner;

    int chunks = randf_s(1, 20);

    for(int i=0; i<chunks; i++)
    {
        float len_frac = randf_s(0.f, 1.f);

        int side = randf_s(0.f, 4.f);

        vec2f pos;

        if(side == 0)
            pos = {randf_s(0.f, dim.x()), 0.f};

        if(side == 1)
            pos = {randf_s(0.f, dim.x()), dim.y()};

        if(side == 2)
            pos = {0.f, randf_s(0.f, dim.y())};

        if(side == 3)
            pos = {dim.x(), randf_s(0.f, dim.y())};

        pos = round_to_multiple(pos, granularity);

        bool skip = false;

        for(auto& kk : exclusion)
        {
            if((pos - kk).length() < granularity*1.5f)
            {
                skip = true;
                break;
            }
        }

        if(skip)
            continue;

        chunk.setPosition(pos.x(), pos.y());

        if(randf_s(0.f, 1.f) < 0.5f)
        {
            chunk.setRotation(45);
        }
        else
        {
            chunk.setRotation(0.f);
        }

        intermediate->draw(chunk, blend);

        exclusion.push_back(pos);
    }

    intermediate->display();

    tex = intermediate->getTexture();

    is_loaded = true;

    //delete intermediate_texture;
    //intermediate_texture = nullptr;
}

ship::~ship()
{
    //if(intermediate_texture)
    //    delete intermediate_texture;
}

///resupply probably needs to distribute fairly from global, or use different resources
///modify this to take a parameter for types
void ship::resupply_elements(empire* emp, const std::vector<ship_component_element>& to_resupply, int num)
{
    /*std::vector<ship_component_elements::types> types =
    {
        ship_component_elements::HP,
        ship_component_elements::FUEL,
        ship_component_elements::AMMO,
        ship_component_elements::ARMOUR,
    };*/

    std::map<resource::types, float> hp_repair_costs = resources_needed_to_repair_total();

    auto capacities = get_available_capacities_vec();

    for(const ship_component_element& type : to_resupply)
    {
        float current_capacity = capacities[type].second;

        std::map<resource::types, float> requested_resource_amounts = ship_component_elements::component_storage_to_resources(type);

        ///need to integrate module costs in here somehow
        ///can we apply this on the top for HP and armour?

        ///so we request twice as much... but we'll only take vanilla amounts
        for(auto& i : requested_resource_amounts)
        {
            i.second *= current_capacity*num;
        }

        if(type == ship_component_elements::HP)
        {
            requested_resource_amounts = hp_repair_costs;
        }

        std::map<ship_component_element, float> to_add;

        if(emp != nullptr)
        {
            float efficiency_frac;

            std::map<resource::types, float> gotten = emp->dispense_resources_proportionally(requested_resource_amounts, 1.f/num, efficiency_frac);

            to_add[type] = efficiency_frac * current_capacity;
        }
        else
        {
            to_add[type] = current_capacity;
        }

        distribute_resources(to_add);
    }
}

void ship::refill_resources(empire* emp, int num)
{
    for(int i=0; i<ship_component_elements::element_infos.size(); i++)
    {
        auto& comp = ship_component_elements::element_infos[i];

        if(comp.resource_type != resource::COUNT)
        {
            resupply_elements(emp, {(ship_component_elements::types)i}, num);
        }
    }
}

void ship::resupply(empire* emp, int num)
{
    return resupply_elements(emp, {ship_component_elements::FUEL, ship_component_elements::AMMO}, num);
}

void ship::repair(empire* emp, int num)
{
    return resupply_elements(emp, {ship_component_elements::HP, ship_component_elements::ARMOUR}, num);
}

bool ship::damaged()
{
    auto fully_merged = get_fully_merged(1.f);

    if(fully_merged[ship_component_element::HP].cur_amount < fully_merged[ship_component_element::HP].max_amount)
        return true;

    return false;
}

bool ship::can_move_in_system()
{
    if(fully_disabled())
        return false;

    float threshold_working_efficiency = 0.75f;

    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_elements::ENGINE_POWER))
        {
            const component_attribute& elem = c.components[ship_component_element::ENGINE_POWER];//c.get_element(ship_component_elements::ENGINE_POWER);

            if(elem.cur_efficiency >= threshold_working_efficiency)
                return true;
        }
    }

    return false;
}

float ship::get_move_system_speed()
{
    float thruster_amount = get_fully_merged_single(ship_component_elements::ENGINE_POWER, 1.f).produced_per_s;

    float internal_size = get_total_components_size() * current_size;

    if(thruster_amount <= FLOAT_BOUND)
        return 0.f;

    if(internal_size <= FLOAT_BOUND)
        return 0.f;

    return ((thruster_amount / internal_size) + 1.f) / 2.f;
}

void ship::apply_disengage_penalty()
{
    if(fully_disabled())
        return;

    std::map<ship_component_elements::types, float> damage_fractions;

    damage_fractions[ship_component_elements::HP] = 0.45f;
    damage_fractions[ship_component_elements::WARP_POWER] = 0.5f;
    damage_fractions[ship_component_elements::SHIELD_POWER] = 0.5f;
    damage_fractions[ship_component_elements::FUEL] = 0.3f;

    std::map<ship_component_elements::types, float> to_apply_negative;

    for(auto& i : damage_fractions)
    {
        float val = get_stored_resources()[i.first];

        float real_neg = - val * i.second;

        to_apply_negative[i.first] = real_neg;
    }

    add_negative_resources(to_apply_negative);

    is_disengaging = true;
    disengage_clock_s = 0;
}

bool ship::can_disengage()
{
    if(fully_disabled())
        return true;

    const float mandatory_combat_time_s = combat_variables::mandatory_combat_time_s;

    if(time_in_combat_s < mandatory_combat_time_s)
        return false;

    return true;
}

bool ship::can_engage()
{
    if(fully_disabled())
        return false;

    const float disengagement_timer_s = combat_variables::disengagement_time_s;

    if(is_disengaging)
    {
        if(disengage_clock_s < disengagement_timer_s)
            return false;

        return true;
    }

    return true;
}

bool ship::should_be_removed_from_combat()
{
    const float disengagement_timer_s = combat_variables::disengagement_time_s;

    if(is_disengaging)
    {
        if(disengage_clock_s < disengagement_timer_s)
            return true;

        return false;
    }

    return false;
}

bool ship::fully_disabled()
{
    return is_fully_disabled;
}

void ship::force_fully_disabled(bool disabled)
{
    is_fully_disabled = disabled;
}

void ship::test_set_disabled()
{
    auto timer = MAKE_AUTO_TIMER();
    timer.start();

    //float cur_hp = get_stored_resources()[ship_component_element::HP];
    //float max_hp = get_max_resources()[ship_component_element::HP];

    auto full_merge = get_fully_merged(1.f);

    float cur_hp = full_merge[ship_component_element::HP].cur_amount;
    float max_hp = full_merge[ship_component_element::HP].max_amount;

    bool full_disabled = false;

    if(max_hp < 0.0001f)
    {
        full_disabled = true;
    }

    float disabled_frac = 0.15f;
    float eff_disabled_frac = 0.10f;

    if((cur_hp / max_hp) < disabled_frac)
    {
        full_disabled = true;
    }

    timer.finish();

    float avg_efficiency = 0.f;
    int num_components = 0;

    int num_crew = 0;
    int num_crew_dying = 0;

    for(component& c : entity_list)
    {
        if(c.skip_in_derelict_calculations)
            continue;

        float ceff = 0.f;
        int cnum = 0;

        //for(auto& kk : c.components)

        if(c.components.size() > 0)
        {
            component_attribute& attr = *c.components.begin();

            ceff += attr.cur_efficiency;
            cnum++;
        }

        if(cnum > 0)
        {
            ceff /= cnum;

            avg_efficiency += ceff;
            num_components++;
        }

        float oxygen_starvation_level = 0.f;

        if(c.has_tag(component_tag::OXYGEN_STARVATION))
        {
            oxygen_starvation_level = c.get_tag(component_tag::OXYGEN_STARVATION);

            if(oxygen_starvation_level > 0.99f && oxygen_starvation_level < 1.01f)
            {
                if(c.get_stored()[ship_component_element::HP] / c.get_stored_max()[ship_component_element::HP] < 0.2f)
                {
                    num_crew_dying++;
                }
            }

            num_crew++;
        }
    }

    if(num_crew != 0 && num_crew == num_crew_dying)
    {
        full_disabled = true;
    }

    if(num_components > 0)
    {
        avg_efficiency /= num_components;
    }

    /*if(avg_efficiency < eff_disabled_frac)
    {
        full_disabled = true;
    }*/

    float available_command = full_merge[ship_component_elements::COMMAND].produced_per_s;

    if(available_command <= 0.001f)
    {
        full_disabled = true;
    }

    force_fully_disabled(full_disabled);
}

bool ship::can_be_upgraded()
{
    for(component& c : entity_list)
    {
        int appropriate_tech = ship_component_elements::component_element_to_research_type[c.primary_attribute];
        float new_tech_level = owned_by->parent_empire->research_tech_level.categories[appropriate_tech].amount;

        float tl = c.get_tech_level_of_primary();

        if(tl < new_tech_level)
        {
            return true;
        }
    }

    return false;
}

/*void ship::set_tech_level_of_component(int component_offset, float tech_level)
{
    if(component_offset < 0 || component_offset >= entity_list.size())
        return;

    component_offset[i].set_tech_level
}*/

void ship::set_max_tech_level_from_empire_and_ship(empire* e)
{
    for(component& c : entity_list)
    {
        c.set_max_tech_level_from_empire_and_component(e);
    }

    repair(nullptr, 1);
}

void ship::set_tech_level_from_empire(empire* e)
{
    for(component& c : entity_list)
    {
        c.set_tech_level_from_empire(e);
    }

    repair(nullptr, 1);
}

void ship::set_tech_level_from_research(research& r)
{
    for(component& c : entity_list)
    {
        c.set_tech_level_from_research(r);
    }

    repair(nullptr, 1);
}

///randomise tech?
void ship::randomise_make_derelict()
{
    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_elements::COMMAND))
        {
            if(!c.has_element(ship_component_elements::HP))
                continue;

            c.components[ship_component_elements::HP].cur_amount = 0.f;
        }
    }

    while(!is_fully_disabled)
    {
        int cid = randf_s(0.f, entity_list.size());

        component& c = entity_list[cid];

        if(!c.has_element(ship_component_elements::HP))
            continue;

        component_attribute& hp_attr = c.components[ship_component_elements::HP];

        hp_attr.cur_amount /= randf_s(5.f, 10.f);

        if(hp_attr.cur_amount < 0)
            hp_attr.cur_amount = 0.f;

        test_set_disabled();
    }
}

void ship::random_damage(float frac)
{
    int cid = randf_s(0.f, entity_list.size());

    float hp = get_stored_resources()[ship_component_elements::HP];

    for(int i=0; i<10.f; i++)
    {
        hit_raw_damage(hp * frac / 10.f, nullptr, nullptr, nullptr);
    }
}

float ship::get_total_cost()
{
    float accum = 0.f;

    for(component& c : entity_list)
    {
        accum += c.get_component_cost();
    }

    return accum;
}

float ship::get_real_total_cost()
{
    float accum = 0.f;

    for(component& c : entity_list)
    {
        accum += c.get_real_component_cost();
    }

    return accum;
}

float ship::get_tech_adjusted_military_power()
{
    float accum = 0.f;

    for(component& c : entity_list)
    {
        if(!c.is_weapon())
            continue;

        accum += (c.get_tech_level_of_primary() + 1) * c.current_size * current_size;

        //accum += c.current_size * current_size;
    }

    return accum;
}

std::map<resource::types, float> ship::resources_needed_to_recrew_total()
{
    std::map<resource::types, float> ret;

    for(component& c : entity_list)
    {
        if(c.repair_this_when_recrewing)
        {
            auto res = c.resources_needed_to_repair();

            for(auto& i : res)
            {
                ret[i.first] += i.second;
            }
        }
    }

    return ret;
}

std::map<resource::types, float> ship::resources_needed_to_repair_total()
{
    std::map<resource::types, float> ret;

    for(component& c : entity_list)
    {
        auto res = c.resources_needed_to_repair();

        for(auto& i : res)
        {
            ret[i.first] += i.second;
        }
    }

    return ret;
}

std::map<resource::types, float> ship::resources_received_when_scrapped()
{
    std::map<resource::types, float> ret;

    for(component& c : entity_list)
    {
        auto res = c.resources_received_when_scrapped();

        for(auto& i : res)
        {
            ret[i.first] += i.second;
        }
    }

    return ret;
}

std::map<resource::types, float> ship::resources_cost()
{
    std::map<resource::types, float> ret;

    for(component& c : entity_list)
    {
        auto res = c.resources_cost();

        for(auto& i : res)
        {
            ret[i.first] += i.second;
        }
    }

    return ret;
}

int ship::number_of_times_can_fully_dispense(const std::map<resource::types, float>& resources)
{
    auto res = get_fully_merged(1.f);

    int min_num = 9999999;
    bool has_any = false;

    for(int i=0; i<(int)ship_component_elements::NONE; i++)
    {
        component_attribute& attr = res[i];

        resource::types type = ship_component_elements::element_infos[i].resource_type;

        if(type == resource::COUNT)
            continue;

        auto found = resources.find(type);

        if(found == resources.end())
            continue;

        if(attr.cur_amount < found->second)
            return 0;

        int num = floor(attr.cur_amount / found->second);

        min_num = std::min(num, min_num);
        has_any = true;
    }

    if(!has_any)
        return 0;

    return min_num;
}

bool ship::can_fully_dispense(const std::map<resource::types, float>& resources)
{
    auto res = get_fully_merged(1.f);

    for(int i=0; i<(int)ship_component_elements::NONE; i++)
    {
        component_attribute& attr = res[i];

        resource::types type = ship_component_elements::element_infos[i].resource_type;

        if(type == resource::COUNT)
            continue;

        auto found = resources.find(type);

        if(found == resources.end())
            continue;

        if(attr.cur_amount < found->second)
            return false;
    }

    return true;
}


void ship::fully_dispense(const std::map<resource::types, float>& resources)
{
    auto res = get_fully_merged(1.f);

    for(int i=0; i<(int)ship_component_elements::NONE; i++)
    {
        component_attribute& attr = res[i];

        resource::types type = ship_component_elements::element_infos[i].resource_type;

        if(type == resource::COUNT)
            continue;

        auto found = resources.find(type);

        float amount = 0.f;

        if(found != resources.end())
        {
            amount = found->second;
        }

        if(attr.cur_amount >= amount)
        {
            std::map<ship_component_element, float> vals;
            vals[(ship_component_elements::types)i] = -amount;

            add_negative_resources(vals);
        }
    }
}

void ship::empty_resources()
{
    for(component& c : entity_list)
    {
        for(int type = 0; type < c.components.size(); type++)
        {
            component_attribute& attr = c.components[type];

            if(!attr.present)
                continue;

            if(ship_component_elements::element_infos[type].resource_type != resource::COUNT)
            {
                attr.cur_amount = 0;
            }
        }
    }
}

research ship::get_research_base_for_empire(empire* owner, empire* claiming)
{
    research r;

    for(component& c : entity_list)
    {
        research_category cat = c.get_research_base_for_empire(owner, claiming);

        r.add_amount(cat);
    }

    return r;
}

research ship::get_research_real_for_empire(empire* owner, empire* claiming)
{
    research r;

    for(component& c : entity_list)
    {
        research_category cat = c.get_research_real_for_empire(owner, claiming);

        if(past_owners_research_left.find(owned_by->parent_empire) != past_owners_research_left.end() && owned_by->parent_empire != nullptr)
        {
            const research& fr = past_owners_research_left[owned_by->parent_empire];

            cat.amount = std::max(cat.amount, fr.categories[cat.type].amount);
        }

        r.add_amount(cat);
    }

    return r;
}

research ship::get_recrew_potential_research(empire* claiming)
{
    research res;

    if(claiming != original_owning_race)
    {
        if(past_owners_research_left.find(claiming) == past_owners_research_left.end())
        {
            res = get_research_base_for_empire(original_owning_race, claiming).div(2.f);
        }
        else
        {
            res = past_owners_research_left[claiming];
        }
    }

    return res;
}

std::string ship::get_resource_str(const ship_component_element& type)
{
    auto fully_merged = get_fully_merged(1.f);

    auto res = fully_merged[type].cur_amount;
    auto max_res = fully_merged[type].max_amount;

    return "(" + to_string_with_enforced_variable_dp(res) + "/" + to_string_with_variable_prec(max_res) + ")";
}

float ship::get_fuel_frac()
{
    auto fully_merged = get_fully_merged(1.f);

    auto res = fully_merged[ship_component_element::FUEL].cur_amount;
    auto max_res = fully_merged[ship_component_element::FUEL].max_amount;

    if(max_res < 0.0001f)
        return 0.f;

    return res / max_res;
}

/*std::map<ship_component_element, float> ship::get_warp_use_frac()
{
    std::map<ship_component_element, float> ret;

    for(component& c : entity_list)
    {
        if(c.primary_attribute == ship_component_element::WARP_POWER)
        {
            auto res =
        }
    }

    if(num > 0)
        accum /= num;

    return accum;
}*/

float ship::get_min_warp_use_frac()
{
    int num = 0;
    float accum = 1.f;

    for(component& c : entity_list)
    {
        if(c.primary_attribute == ship_component_element::WARP_POWER)
        {
            accum = std::min(accum, get_min_use_frac(c));

            num++;
        }
    }

    if(num == 0)
        accum = 0.f;

    return accum;
}

void ship::recrew_derelict(empire* owner, empire* claiming)
{
    if(claiming == nullptr)
        return;

    if(!can_recrew(claiming))
        return;

    if(claiming != original_owning_race)
    {
        is_alien = true;

        crew_effectiveness = 1.f - clamp(claiming->empire_culture_distance(claiming), 0.0f, 0.8f);

        if(past_owners_research_left.find(claiming) == past_owners_research_left.end())
        {
            research_left_from_crewing = get_research_base_for_empire(original_owning_race, claiming).div(2.f);

            past_owners_research_left[claiming] = research_left_from_crewing;
        }
        else
        {
            research_left_from_crewing = past_owners_research_left[claiming];
        }

        team = claiming->team_id;
    }
    else
    {
        is_alien = false;
        crew_effectiveness = 1.f;
        team = claiming->team_id;
        research_left_from_crewing = research();
    }

    auto res_needed = resources_needed_to_recrew_total();

    for(auto& i : res_needed)
    {
        claiming->dispense_resource(i.first, i.second);
    }

    for(component& c : entity_list)
    {
        if(!c.repair_this_when_recrewing)
            continue;

        if(!c.has_element(ship_component_elements::HP))
            continue;

        component_attribute& attr = c.components[ship_component_elements::HP];

        attr.cur_amount = attr.max_amount;
    }

    //damage_taken.clear();
}

bool ship::can_recrew(empire* claiming)
{
    if(claiming == nullptr)
        return false;

    auto res_needed = resources_needed_to_recrew_total();

    return claiming->can_fully_dispense(res_needed);
}

float default_scanning_power_curve(float scanner_modified_power)
{
    /*if(scanner_modified_power < 0)
        return 0.f;

    if(scanner_modified_power < 0.5f)
        return 0.25f;

    if(scanner_modified_power >= 0.5f && scanner_modified_power < 1.5f)
        return 0.75f;

    if(scanner_modified_power >= 1.5f)
        return 1.f;*/

    //if(scanner_modified_power < 0)
    //    return 0.f;

    //if(scanner_modified_power > 1.f)
    //    return 1.f;

    return scanner_modified_power;
}

///nax tech level
float get_default_scanning_power(ship* s)
{
    float scanner_modified_power = 0.f;

    for(component& c : s->entity_list)
    {
        if(c.primary_attribute != ship_component_elements::SCANNING_POWER)
            continue;

        ///not operating well enough
        if(c.components[c.primary_attribute].cur_efficiency < 0.25f)
            continue;

        float tech_level = c.get_tech_level_of_primary() + 0.5f;

        tech_level = tech_level * c.components[c.primary_attribute].cur_efficiency;
        scanner_modified_power = std::max(scanner_modified_power, tech_level);
        //scanner_modified_power += tech_level;
    }

    return scanner_modified_power;
}

///max tech level
float get_stealth_power(ship* s)
{
    float accum_tech_stealth_system_modified = 0.f;

    for(int i=0; i<s->entity_list.size(); i++)
    {
        component& c = s->entity_list[i];

        if(c.primary_attribute != ship_component_elements::STEALTH)
            continue;

        float tech_level = c.get_tech_level_of_primary() + 0.5f;

        tech_level = tech_level * c.components[c.primary_attribute].cur_efficiency;

        accum_tech_stealth_system_modified = std::max(accum_tech_stealth_system_modified, tech_level);

        //accum_tech_stealth_system_modified += tech_level;
    }

    return accum_tech_stealth_system_modified;
}

float ship::get_scanning_power_on_ship(ship* s, int difficulty_modifier)
{
    float empire_culture_distance = owned_by->parent_empire->empire_culture_distance(s->original_owning_race);

    ///0 = not far, > 1 = far
    empire_culture_distance = clamp(1.f - empire_culture_distance, 0.f, 1.f);

    float culture_distance_mod = 0.f;

    if(empire_culture_distance < 0.5f)
        culture_distance_mod = -(empire_culture_distance) * 0.10f;
    if(empire_culture_distance >= 0.5f)
        culture_distance_mod = (empire_culture_distance - 0.5f) * 0.10f;

    float disabled_bonus = s->fully_disabled() ? 0.25f : 0.f;

    /*float max_tech_stealth_system_modified = get_stealth_power(s);

    float modified_scanning_power = get_default_scanning_power(this) - difficulty_modifier;

    float disabled_mod = s->fully_disabled() ? 0.25f : 0.f;

    float end_val = modified_scanning_power - max_tech_stealth_system_modified;

    end_val += disabled_mod + culture_distance_mod;

    float drive_signature = get_drive_signature();

    ///drive signature / 100

    end_val += clamp((get_scanning_ps() - drive_signature) / 100.f, -4.f, 4.f);

    end_val = clamp(end_val, 0.f, 1.f);

    return end_val;*/

    ///so. This is the cause of why available scanning power on is slow
    const auto& res = cached_fully_merged;
    const auto& res2 = s->cached_fully_merged;

    float their_power_excess = res2[ship_component_elements::ENERGY].produced_per_s - res2[ship_component_elements::STEALTH].produced_per_s;

    float diff = res[ship_component_elements::SCANNING_POWER].produced_per_s + their_power_excess;

    float total_scanning_power = (diff / 100.f) + empire_culture_distance + disabled_bonus + culture_distance_mod;


    ///so. Say we produce 90 power and have 30 stealth
    ///excess power over stealth is 60
    ///enemy scanning power is 30
    ///that means that the total scan power is 90 (60 power over stealth, 30 scan power)

    return clamp(total_scanning_power, 0.f, 1.f);
}

float ship::get_scanning_power_on(orbital* o, int difficulty_modifier)
{
    ///naive
    ///if my == theirs, total information
    ///if my == theirs stealth, little to no information
    ///for every level of difference, we lose 50%. 2 levels below we get very little? 50% of remaining 50%?
    if(o->type != orbital_info::FLEET)
    {
        float scanner_modified_power = get_default_scanning_power(this) - difficulty_modifier;

        return clamp(default_scanning_power_curve(scanner_modified_power), 0.f, 1.f);
    }
    else
    {
        float max_emissions = 0.f;

        ship_manager* sm = (ship_manager*)o->data;

        for(ship* s : sm->ships)
        {
            max_emissions = std::max(max_emissions, get_scanning_power_on_ship(s, difficulty_modifier));
        }

        return max_emissions;
    }
}

float ship::get_drive_signature()
{
    ///should we use drained for power? Or just take into account etc?
    auto store = get_produced_resources(1.f);

    float power = store[ship_component_elements::ENERGY];
    float stealth = store[ship_component_element::STEALTH];

    return power - stealth;
}

float ship::get_scanning_ps()
{
    auto store = get_produced_resources(1.f);

    float scan_ps = store[ship_component_elements::SCANNING_POWER];

    return scan_ps;
}

bool ship::can_colonise()
{
    for(component& i : entity_list)
    {
        if(i.primary_attribute == ship_component_elements::COLONISER)
            return true;
    }

    return false;
}

bool ship::is_military()
{
    for(component& i : entity_list)
    {
        if(i.is_weapon())
            return true;
    }

    return false;
}

float ship::get_total_storage_of_components_with_this_primary(ship_component_element primary, ship_component_element resource_to_get)
{
    float amount = 0.f;

    for(component& c : entity_list)
    {
        if(c.primary_attribute != primary)
            continue;

        if(!c.has_element(resource_to_get))
            continue;

        amount += c.get_element(resource_to_get).cur_amount;
    }

    return amount;
}

float ship::get_max_storage_of_components_with_this_primary(ship_component_element primary, ship_component_element resource_to_get)
{
    float amount = 0.f;

    for(component& c : entity_list)
    {
        if(c.primary_attribute != primary)
            continue;

        if(!c.has_element(resource_to_get))
            continue;

        amount += c.get_element(resource_to_get).max_amount;
    }

    return amount;
}

float ship::get_total_components_size()
{
    float accum = 0.f;

    for(component& c : entity_list)
    {
        accum += c.current_size;
    }

    return accum;
}

bool ship::is_ship_design_valid()
{
    float max_space = ship_component_elements::max_components_total_size;

    return get_total_components_size() <= max_space + FLOAT_BOUND;
}

sf::Texture* ship::get_world_texture(ship_type::types type)
{
    static sf::Texture textures[ship_type::COUNT];

    for(int i=0; i<ship_type::COUNT; i++)
    {
        if(textures[i].getSize().x == 0)
        {
            sf::Image img;
            img.loadFromFile(ship_type::type_to_png[(ship_type::types)i]);
            premultiply(img);

            textures[i].loadFromImage(img);
            textures[i].setSmooth(true);
        }
    }

    return &textures[type];
}

sf::Texture* ship::get_world_texture()
{
    return get_world_texture(estimated_type);
}

float ship::get_current_size()
{
    return current_size;
}

float ship::get_visual_scale()
{
    float min_size = 0.25f;
    float max_size = 4.f;

    float csize = get_current_size();

    float scale = log10(csize) + 1;

    if(csize < 1)
    {
        csize = mix(min_size, 1.f, (csize - min_size) / (1.f - min_size));
    }

    return clamp(scale, min_size, max_size);
}

void ship::set_size(float new_size)
{
    new_size = clamp(new_size, 0.1f, 1000.f);

    for(component& c : entity_list)
    {
        c.set_ship_size(new_size);
    }

    current_size = new_size;
}

void ship::do_serialise(serialise& s, bool ser)
{
    #if 1
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(estimated_type, ser);
        s.handle_serialise(editor_size_storage, ser);
        s.handle_serialise(colonise_target, ser);
        s.handle_serialise(colonising, ser);

        s.handle_serialise(past_owners_research_left, ser);
        s.handle_serialise(original_owning_race, ser);
        s.handle_serialise(crew_effectiveness, ser);
        s.handle_serialise(is_alien, ser);

        s.handle_serialise(research_left_from_crewing, ser);
        s.handle_serialise(cleanup, ser);
        s.handle_serialise(is_fully_disabled, ser);
        s.handle_serialise(currently_in_combat, ser);

        s.handle_serialise(currently_in_combat, ser);
        s.handle_serialise(time_in_combat_s, ser);
        s.handle_serialise(is_disengaging, ser);
        s.handle_serialise(disengage_clock_s, ser);
        s.handle_serialise(display_weapon, ser);
        s.handle_serialise(display_popout, ser);
        s.handle_serialise(display_ui, ser);
        s.handle_serialise(owned_by, ser);
        s.handle_serialise(tex, ser);
        s.handle_serialise(is_loaded, ser);
        s.handle_serialise(highlight, ser);
        s.handle_serialise(has_element, ser);

        ///ok so the problem is
        ///new updates aren't updating the current elements, they're clearing the vector
        ///and then doing it again
        ///potential fix: New mode to address this situation
        ///s.handle_serialise_update
        s.handle_serialise_no_clear(entity_list, ser);

        s.handle_serialise(type_to_component_offsets, ser);
        s.handle_serialise(team, ser);
        s.handle_serialise(name, ser);
        s.handle_serialise(ai_fleet_type, ser);

        s.handle_serialise(dim, ser);
        s.handle_serialise(local_rot, ser);
        s.handle_serialise(local_pos, ser);
        s.handle_serialise(world_rot, ser);
        s.handle_serialise(world_pos, ser);
    }

    if(serialise_data_helper::send_mode == 0 || serialise_data_helper::send_mode == 2)
    {
        //s.handle_serialise(editor_size_storage, ser);
        s.handle_serialise(colonise_target, ser);
        s.handle_serialise(colonising, ser);

        s.handle_serialise(past_owners_research_left, ser);
        s.handle_serialise(original_owning_race, ser);
        s.handle_serialise(crew_effectiveness, ser);
        s.handle_serialise(is_alien, ser);

        s.handle_serialise(research_left_from_crewing, ser);
        s.handle_serialise(cleanup, ser);
        s.handle_serialise(is_fully_disabled, ser);
        s.handle_serialise(currently_in_combat, ser);

        s.handle_serialise(currently_in_combat, ser);
        s.handle_serialise(time_in_combat_s, ser);
        s.handle_serialise(is_disengaging, ser);
        s.handle_serialise(disengage_clock_s, ser);
        //s.handle_serialise(display_weapon, ser);
        //s.handle_serialise(display_popout, ser);
        //s.handle_serialise(display_ui, ser);
        s.handle_serialise(owned_by, ser);
        //s.handle_serialise(tex, ser);
        //s.handle_serialise(is_loaded, ser);
        //s.handle_serialise(highlight, ser);
        //s.handle_serialise(has_element, ser);

        s.handle_serialise_no_clear(entity_list, ser);

        //s.handle_serialise(type_to_component_offsets, ser);
        //s.handle_serialise(team, ser);
        //s.handle_serialise(name, ser);
        //s.handle_serialise(ai_fleet_type, ser);

        //s.handle_serialise(dim, ser);
        s.handle_serialise(local_rot, ser);
        s.handle_serialise(local_pos, ser);
        s.handle_serialise(world_rot, ser);
        s.handle_serialise(world_pos, ser);

        ///ok, but this will break when/if we swap dirty to using disk serialisation
        if(!handled_by_client)
        {
            handled_by_client = true;

            owned_by->ships.push_back(this);
        }
    }

    handled_by_client = true;

    #endif
}

/*ship* ship_manager::make_new(int team)
{
    ship* s = new ship;

    ships.push_back(s);

    procedural_text_generator generator;

    s->team = team;
    s->name = generator.generate_ship_name();

    s->owned_by = this;

    return s;
}*/

ship* ship_manager::make_new_from(empire* e, const ship& ns)
{
    ship* s = new ship(ns.duplicate());

    ships.push_back(s);

    procedural_text_generator generator;

    s->team = e->team_id;
    //s->name = e->ship_prefix + " " + generator.generate_ship_name();
    s->name = ns.name;

    s->owned_by = this;

    s->make_dirty();

    return s;
}

void ship_manager::destroy(ship* s)
{
    for(int i=0; i<ships.size(); i++)
    {
        if(ships[i] == s)
        {
            ships.erase(ships.begin() + i);
            delete s;

            return;
        }
    }
}

std::vector<std::string> ship_manager::get_info_strs()
{
    std::vector<std::string> ret;

    for(ship* s : ships)
    {
        std::string name_str = s->name;

        if(s->fully_disabled())
        {
            name_str += " (Derelict)";
        }

        ret.push_back(name_str);
    }

    return ret;
}

std::vector<std::string> ship_manager::get_info_strs_with_info_warfare(empire* viewing, orbital* my_orbital, bool full_detail)
{
    bool should_obfuscate = false;

    float scanning_power = viewing->available_scanning_power_on(my_orbital);

    if(scanning_power < ship_info::ship_obfuscation_level)
        should_obfuscate = true;

    bool obfuscate_hp = scanning_power < ship_info::misc_resources_obfuscation_level;

    if(viewing->is_allied(my_orbital->parent_empire))
    {
        obfuscate_hp = false;
        should_obfuscate = false;
    }


    std::vector<std::string> ret;

    for(ship* s : ships)
    {
        std::string name_str = obfuscate(s->name, should_obfuscate);

        if(s->fully_disabled())
        {
            name_str += " (Derelict)";
        }

        std::string detail_str;

        if(full_detail && !obfuscate_hp)
        {
            auto fully_merged = s->get_fully_merged(1.f);

            //float hp_frac = res[ship_component_element::HP] / max_res[ship_component_element::HP];
            //float percent = hp_frac * 100.f;
            //std::string hp_name = "(" + to_string_with_enforced_variable_dp(percent) + "%%)";

            std::string hp_name = "(" + to_string_with_enforced_variable_dp(fully_merged[ship_component_element::HP].cur_amount) + "/" + to_string_with_variable_prec(fully_merged[ship_component_element::HP].max_amount) + ")";

            if(obfuscate_hp)
            {
                hp_name = obfuscate(hp_name, true);
            }

            /*if(should_obfuscate)
            {
                h
            }*/

            /*std::string s1 = "(" + to_string_with_enforced_variable_dp(res[ship_component_element::HP]) + "/" + to_string_with_variable_prec(max_res[ship_component_element::HP]) + ")";
            std::string s2 = "(" + to_string_with_enforced_variable_dp(res[ship_component_element::WARP_POWER]) + "/" + to_string_with_variable_prec(max_res[ship_component_element::WARP_POWER]) + ")";
            std::string s3 = "(" + to_string_with_enforced_variable_dp(res[ship_component_element::FUEL]) + "/" + to_string_with_variable_prec(max_res[ship_component_element::FUEL]) + ")";

            name_str = name_str + " " + s1 + " " + s2 + " " + s3;*/

            name_str += " " + hp_name;
        }

        ret.push_back(name_str);
    }

    return ret;
}

void ship_manager::merge_into_me(ship_manager& other)
{
    if(this == &other)
        return;

    for(auto& i : other.ships)
    {
        i->owned_by = this;

        ships.push_back(i);
    }
}

void ship_manager::steal(ship* const s)
{
    if(s->owned_by == this)
        return;

    ship_manager* other = s->owned_by;

    for(int i=0; i<other->ships.size(); i++)
    {
        if(other->ships[i] == s)
        {
            other->ships.erase(other->ships.begin() + i);
            break;
        }
    }

    dirty = true;
    other->dirty = true;

    s->owned_by = this;
    s->dirty = true;

    ships.push_back(s);
}

void ship_manager::refill_resources(empire* from)
{
    if(parent_empire == nullptr)
        return;

    if(from == nullptr)
    {
        from = parent_empire;
    }

    int num = ships.size();

    for(ship* s : ships)
    {
        if(s->fully_disabled())
        {
            num--;
            continue;
        }

        s->refill_resources(from, num);

        num--;
    }
}

void ship_manager::resupply(empire* from, bool can_resupply_derelicts)
{
    if(parent_empire == nullptr)
        return;

    if(from == nullptr)
    {
        from = parent_empire;
    }

    int num = ships.size();

    for(ship* s : ships)
    {
        if(!can_resupply_derelicts && s->fully_disabled())
        {
            num--;
            continue;
        }

        s->resupply(from, num);

        num--;
    }
}

void ship_manager::resupply_from_nobody()
{
    int num = ships.size();

    for(ship* s : ships)
    {
        s->resupply(nullptr, num);

        num--;
    }
}

void ship_manager::repair(empire* from)
{
    if(parent_empire == nullptr)
        return;

    if(from == nullptr)
    {
        from = parent_empire;
    }

    int num = ships.size();

    for(ship* s : ships)
    {
        if(s->fully_disabled())
        {
            num--;
            continue;
        }

        s->repair(from, num);

        num--;
    }
}

bool ship_manager::should_resupply_base(const std::vector<ship_component_element>& to_test)
{
    /*std::vector<ship_component_elements::types> types =
    {
        ship_component_elements::HP,
        ship_component_elements::FUEL,
        ship_component_elements::AMMO,
        ship_component_elements::ARMOUR,
    };*/

    for(ship* s : ships)
    {
        auto fully_merged = s->get_fully_merged(1.f);

        for(auto& i : to_test)
        {
            if(fully_merged[i].max_amount < 0.0001f)
                continue;

            if(fully_merged[i].cur_amount < 0.8f * fully_merged[i].max_amount)
            {
                return true;
            }
        }
    }

    return false;
}

bool ship_manager::should_resupply()
{
    return should_resupply_base({ship_component_elements::FUEL, ship_component_elements::AMMO});
}

bool ship_manager::should_repair()
{
    return should_resupply_base({ship_component_elements::HP, ship_component_elements::ARMOUR});
}

bool ship_manager::has_access_to_repair(orbital_system* sys)
{
    if(any_derelict())
        return false;

    if(any_in_combat())
        return false;

    ///should be impossible, however parent_empire for ships
    ///is a concept that is moderately likely to change, so if I do change it later
    ///this is here for future proofing
    if(parent_empire == nullptr)
        return false;

    orbital* base = sys->get_base();

    ///should be impossible
    if(base == nullptr)
    {
        printf("warning, bad system with no base in has access to repair");
        return false;
    }

    ///if any orbital in the system is hostile to me
    ///I cannot repair
    for(orbital* o : sys->orbitals)
    {
        if(o->parent_empire == nullptr)
            continue;

        if(o->parent_empire == parent_empire)
            continue;

        if(o->type == orbital_info::FLEET)
        {
            ship_manager* sm = (ship_manager*)o->data;

            if(sm->all_derelict())
                continue;
        }

        if(o->parent_empire->is_hostile(parent_empire))
            return false;
    }

    ///my system
    if(parent_empire == base->parent_empire)
        return true;

    ///ally's system
    if(parent_empire->is_allied(base->parent_empire))
        return true;

    ///this is a system for which i have passage rights
    if(parent_empire->can_traverse_space(base->parent_empire))
        return true;

    ///otherwise false
    return false;
}

void ship_manager::tick_all(float step_s)
{
    //sf::Clock clk;

    for(ship* s : ships)
    {
        s->check_load({s->dim.x(), s->dim.y()});

        s->tick_all_components(step_s);
        s->tick_other_systems(step_s);

        if(!s->display_ui)
        {
            s->confirming_scrap = false;
        }

        if(s->is_alien)
        {
            research r = s->tick_drain_research_from_crew(step_s);

            empire* original_empire = s->original_owning_race;

            float research_currency = r.units_to_currency(false);

            if(parent_empire != nullptr)
            {
                parent_empire->add_resource(resource::RESEARCH, research_currency);
                parent_empire->culture_shift(research_info::culture_shift_distance_per_unit_research * research_currency, original_empire);
            }
        }
    }

    //printf("elapsed %f\n", clk.getElapsedTime().asMicroseconds() / 1000.f / 1000.f);
}

bool ship_manager::can_move_in_system()
{
    for(ship* s : ships)
    {
        if(!s->can_move_in_system())
            return false;
    }

    return true;
}

float ship_manager::get_move_system_speed()
{
    float min_speed = FLT_MAX;

    for(ship* s : ships)
    {
        min_speed = std::min(min_speed, s->get_move_system_speed());
    }

    return min_speed;
}

void ship_manager::draw_alerts(sf::RenderWindow& win, vec2f abs_pos)
{
    float fuel_yellow_alert = 0.2f;
    float fuel_red_alert = 0.1f;

    vec3f rcol = {1, 0.1, 0};
    //vec3f ycol = {1, 0.9, 0};

    vec3f fuel_bad_col = {1, 1, 1};
    vec3f fuel_poor_col = {0.4, 0.4, 0.4};

    std::string alert_symbol;

    vec3f alert_colour;
    bool any_alert = false;
    bool any_fully_disabled = false;
    bool any_immobile = false;

    for(ship* s : ships)
    {
        float stored = s->get_stored_resources()[ship_component_element::FUEL];
        float max_store = s->get_max_resources()[ship_component_element::FUEL];

        ///for some reason, if this evaluates to true we go slooow
        if(!s->can_move_in_system())
        {
            any_immobile = true;
        }

        if(s->fully_disabled())
        {
            any_fully_disabled = true;
        }


        if(max_store < 0.001f)
        {
            continue;
        }

        float frac = stored / max_store;

        if(frac < fuel_yellow_alert)
        {
            alert_colour = fuel_poor_col;
            any_alert = true;
        }

        if(frac < fuel_red_alert)
        {
            alert_colour = fuel_bad_col;
        }
    }

    //if(!any_alert)
    //    return;

    if(any_alert)
        alert_symbol += "!";

    //bool any_immobile = false;

    ///implement mixed colours later
    /*for(ship* s : ships)
    {
        if(!s->can_move_in_system())
        {
            any_immobile = true;

            alert_colour = rcol;
        }
    }*/

    if(any_immobile)
    {
        alert_colour = rcol;

        alert_symbol = "!";
    }


    //bool any_fully_disabled = false;

    /*for(ship* s : ships)
    {
        if(s->fully_disabled())
        {
            any_fully_disabled = true;
        }
    }*/

    if(any_fully_disabled)
    {
        alert_symbol = "!";

        alert_colour = {1, 0, 1};
    }

    if(alert_symbol == "")
        return;

    //auto transfd = win.mapCoordsToPixel({abs_pos.x(), abs_pos.y()});

    //vec2f final_pos = (vec2f){transfd.x, transfd.y} + (vec2f){10, -10};

    //printf("fpos %f %f\n", final_pos.x(), final_pos.y());

    text_manager::render(win, alert_symbol, abs_pos + (vec2f){8, -20}, alert_colour);
}

ship_type::types ship_manager::get_most_common_ship_type()
{
    std::vector<int> counts;
    counts.resize(ship_type::COUNT+1);

    for(ship* s : ships)
    {
        counts[s->estimated_type]++;
    }

    ship_type::types type = (ship_type::types)std::distance(counts.begin(), std::max_element(counts.begin(), counts.end()));

    return type;
}

sf::Texture* ship_manager::get_universe_texture()
{
    ship_type::types type = get_most_common_ship_type();

    for(ship* s : ships)
    {
        if(s->estimated_type == type)
            return s->get_world_texture();
    }

    if(ships.size() > 0)
    {
        return ships.front()->get_world_texture();
    }

    return nullptr;
}

void ship_manager::try_warp(orbital_system* fin, orbital* o)
{
    //if(!can_warp(fin, o->parent_system, o))
    //    return;

    /*for(ship* s : ships)
    {
        s->use_warp_drives();
    }

    vec2f my_pos = cur->universe_pos;
    vec2f their_pos = fin->universe_pos;

    vec2f arrive_dir = (my_pos - their_pos).norm() * 500;

    fin->steal(o, cur);

    o->transferring = false;

    o->absolute_pos = arrive_dir;
    o->set_orbit(arrive_dir);*/

    //force_warp(fin, o->parent_system, o);

    o->command_queue.try_warp(fin);
}

void ship_manager::force_warp(orbital_system* fin, orbital_system* cur, orbital* o)
{
    if(fin == cur)
        return;

    for(ship* s : ships)
    {
        s->use_warp_drives();
    }

    vec2f my_pos = cur->universe_pos;
    vec2f their_pos = fin->universe_pos;

    vec2f arrive_dir = (my_pos - their_pos).norm() * 500;

    fin->steal(o, cur);

    //o->transferring = false;
    //o->command_queue.cancel();

    o->absolute_pos = arrive_dir;
    o->set_orbit(arrive_dir);
}

bool ship_manager::can_warp(orbital_system* fin, orbital_system* cur, orbital* o)
{
    bool all_use = true;

    for(ship* s : ships)
    {
        if(!s->can_use_warp_drives())
            all_use = false;
    }

    if(any_in_combat())
        return false;

    /*vec2f base_dist = fin->universe_pos * system_manager::universe_scale;
    vec2f end_dist = cur->universe_pos * system_manager::universe_scale;

    if((base_dist - end_dist).length() > get_min_warp_distance() * system_manager::universe_scale)
    {
        return false;
    }*/

    if(!within_warp_distance(fin, o))
    {
        return false;
    }

    return all_use;
}

bool ship_manager::within_warp_distance(orbital_system* fin, orbital* o)
{
    vec2f base_dist = fin->universe_pos * system_manager::universe_scale;
    vec2f end_dist = o->parent_system->universe_pos * system_manager::universe_scale;

    if((base_dist - end_dist).length() > get_min_warp_distance() * system_manager::universe_scale)
    {
        return false;
    }

    return true;
}

float ship_manager::get_min_warp_distance()
{
    float min_dist = 0.f;
    bool init = false;

    for(ship* s : ships)
    {
        if(!init)
        {
            min_dist = s->get_warp_distance();
            init = true;
            continue;
        }

        min_dist = std::min(min_dist, s->get_warp_distance());
    }

    return min_dist;
}

bool ship_manager::can_use_warp_drives()
{
    for(ship* s : ships)
    {
        if(!s->can_use_warp_drives())
            return false;
    }

    return true;
}

float ship_manager::get_overall_warp_drive_use_frac()
{
    ///afaik this should be impossible
    int num = ships.size();

    if(num == 0)
        return 0.f;

    float accum = 1.f;

    for(ship* s : ships)
    {
        //accum +=s->get_avg_warp_use_frac();

        accum = std::min(s->get_min_warp_use_frac(), accum);
    }

    if(num == 0)
        accum = 0.f;

    return accum;
}

bool ship_manager::any_with_element(ship_component_element elem)
{
    for(ship* s : ships)
    {
        if(s->has_element[(int)elem])
            return true;
    }

    return false;
}

bool ship_manager::all_with_element(ship_component_element elem)
{
    int num_with = 0;

    for(ship* s : ships)
    {
        for(component& c : s->entity_list)
        {
            if(c.has_element(elem))
            {
                num_with++;
            }
        }
    }

    return num_with == ships.size();
}

void ship_manager::enter_combat()
{
    for(ship* s : ships)
    {
        s->enter_combat();
    }
}

void ship_manager::leave_combat()
{
    for(ship* s : ships)
    {
        s->leave_combat();
    }
}

bool ship_manager::any_in_combat()
{
    for(ship* s : ships)
    {
        if(s->in_combat())
            return true;
    }

    return false;
}

bool ship_manager::can_engage()
{
    for(ship* s : ships)
    {
        if(s->can_engage())
            return true;
    }

    return false;
}

bool ship_manager::should_be_removed_from_combat()
{
    for(ship* s : ships)
    {
        if(s->should_be_removed_from_combat())
            return true;
    }

    return false;
}

void ship_manager::random_damage_ships(float frac)
{
    for(ship* s : ships)
    {
        s->random_damage(frac);
    }
}

void ship_manager::apply_disengage_penalty()
{
    for(ship* s : ships)
    {
        s->apply_disengage_penalty();
    }
}

bool ship_manager::any_colonising()
{
    for(ship* s : ships)
    {
        if(s->colonising)
            return true;
    }

    return false;
}

bool ship_manager::any_derelict()
{
    for(ship* s : ships)
    {
        if(s->fully_disabled())
            return true;
    }

    return false;
}

bool ship_manager::all_derelict()
{
    for(ship* s : ships)
    {
        if(!s->fully_disabled())
            return false;
    }

    return true;
}

bool ship_manager::any_damaged()
{
    for(ship* s : ships)
    {
        if(s->damaged())
            return true;
    }

    return false;
}

int ship_manager::number_of_times_can_fully_dispense(const std::map<resource::types, float>& resources)
{
    int accum_num = 0;

    for(ship* s : ships)
    {
        accum_num += s->number_of_times_can_fully_dispense(resources);
    }

    return accum_num;
}

bool ship_manager::can_fully_dispense(const std::map<resource::types, float>& resources)
{
    for(ship* s : ships)
    {
        if(s->can_fully_dispense(resources))
            return true;
    }

    return false;
}

void ship_manager::fully_dispense(const std::map<resource::types, float>& resources)
{
    for(ship* s : ships)
    {
        if(s->can_fully_dispense(resources))
        {
            s->fully_dispense(resources);
            return;
        }
    }
}

float ship_manager::get_tech_adjusted_military_power()
{
    float accum = 0.f;

    for(ship* s : ships)
    {
        accum += s->get_tech_adjusted_military_power();
    }

    return accum;
}

bool ship_manager::is_military()
{
    float tech_power = get_tech_adjusted_military_power();

    if(tech_power <= FLOAT_BOUND)
        return false;

    if(any_with_element(ship_component_elements::COLONISER) ||
       any_with_element(ship_component_elements::ORE_HARVESTER))
        return false;

    return true;
}

bool ship_manager::majority_of_type(ship_type::types type)
{
    int ship_types[ship_type::COUNT + 1] = {0};

    for(ship* s : ships)
    {
        ship_types[s->ai_fleet_type]++;
    }

    int index = std::distance(std::begin(ship_types), std::max_element(std::begin(ship_types), std::end(ship_types)));

    return index == type && ship_types[index] > 0;
}

std::string ship_manager::get_engage_str()
{
    if(can_engage())
        return "(Engage)";

    bool any_derelict = false;

    float min_time_remaining = 0.f;

    for(ship* s : ships)
    {
        min_time_remaining = std::max(min_time_remaining, combat_variables::disengagement_time_s - s->disengage_clock_s);

        if(s->fully_disabled())
        {
            any_derelict = true;
        }
    }

    std::string cooldown_str = "(On Cooldown) " + to_string_with_enforced_variable_dp(min_time_remaining) + "s";

    if(any_derelict)
    {
        cooldown_str = "Ship Derelict";
    }

    return cooldown_str;
}


std::string ship_manager::get_fuel_message()
{
    float fuel_frac = get_min_fuel_frac();

    if(fuel_frac > 0.9f)
    {
        return "Full";
    }
    else if(fuel_frac > 0.7f)
    {
        return "Good";
    }
    else if(fuel_frac > 0.5f)
    {
        return "Ok";
    }
    else if(fuel_frac > 0.3f)
    {
        return "Poor";
    }
    else if(fuel_frac > 0.1f)
    {
        return "Very Poor";
    }
    else
    {
        return "Empty";
    }
}

float ship_manager::get_min_fuel_frac()
{
    float fuel_frac = FLT_MAX;

    for(ship* s : ships)
    {
        fuel_frac = std::min(s->get_fuel_frac(), fuel_frac);
    }

    return fuel_frac;
}

void ship_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(in_friendly_territory, ser);
        s.handle_serialise(can_merge, ser);
        s.handle_serialise(to_close_ui, ser);
        s.handle_serialise(toggle_fleet_ui, ser);
        s.handle_serialise(decolonising, ser);
        s.handle_serialise(accumulated_dt, ser);
        s.handle_serialise(parent_empire, ser);
        s.handle_serialise(auto_colonise, ser);
        s.handle_serialise(auto_harvest_ore, ser);
        s.handle_serialise(auto_resupply, ser);
        s.handle_serialise(ships, ser);
        //s.handle_serialise(cleanup, ser);
        //s.handle_serialise(requesting_or_in_battle, ser);
    }

    if(serialise_data_helper::send_mode == 0 || serialise_data_helper::send_mode == 2)
    {
        //s.handle_serialise(in_friendly_territory, ser);
        //s.handle_serialise(can_merge, ser);
        //s.handle_serialise(to_close_ui, ser);
        //s.handle_serialise(toggle_fleet_ui, ser);
        s.handle_serialise(decolonising, ser);
        //s.handle_serialise(accumulated_dt, ser);
        s.handle_serialise(parent_empire, ser);
        s.handle_serialise(auto_colonise, ser);
        s.handle_serialise(auto_harvest_ore, ser);
        s.handle_serialise(auto_resupply, ser);
        s.handle_serialise(ships, ser);
        //s.handle_serialise(cleanup, ser);
        //s.handle_serialise(requesting_or_in_battle, ser);

        ///when we call this fleet manage doesn't exist
        /*if(!handled_by_client)
        {
            handled_by_client = true;

            fleet_manage->fleets.push_back(this);
        }*/
    }

    //handled_by_client = true;
}

ship_manager* fleet_manager::make_new()
{
    ship_manager* ns = new ship_manager;

    fleets.push_back(ns);
    ns->make_dirty();
    //ns->fleet_manage = this;

    return ns;
}

bool fleet_manager::owns(ship_manager* sm)
{
    for(auto& i : fleets)
    {
        if(i == sm)
            return true;
    }

    return false;
}

///how do we handle dirtyness when destroying?
void fleet_manager::destroy(ship_manager* ns)
{
    for(int i=0; i < fleets.size(); i++)
    {
        if(fleets[i] == ns)
        {
            fleets.erase(fleets.begin() + i);

            delete ns;

            return;
        }
    }
}

void fleet_manager::cull_dead_deferred()
{
    for(ship_manager* sm : fleets)
    {
        int num_cleanup = 0;

        for(ship* s : sm->ships)
        {
            if(s->cleanup)
            {
                num_cleanup++;
            }
        }

        if(num_cleanup == sm->ships.size())
        {
            sm->cleanup = true;
        }
    }
}

///wait what ALERT we're just leaking memory!!!
void fleet_manager::destroy_cleanup(empire_manager& empire_manage)
{
    for(int i=0; i<fleets.size(); i++)
    {
        ship_manager* sm = fleets[i];

        for(int kk=0; kk < sm->ships.size(); kk++)
        {
            ship* s = sm->ships[kk];

            if(s->cleanup)
            {
                sm->ships.erase(sm->ships.begin() + kk);

                //delete s;
                kk--;
                continue;
            }
        }

        if(sm->cleanup)
        {
            empire_manage.notify_removal(sm);

            for(int kk=0; kk < sm->ships.size(); kk++)
            {
                ship* s = sm->ships[kk];
                //delete s;
            }

            fleets.erase(fleets.begin() + i);

            //delete sm;
            i--;
            continue;
        }
    }
}

void fleet_manager::tick_all(float step_s)
{
    int bound = 8;

    for(ship_manager* sm : fleets)
    {
        sm->accumulated_dt += step_s;
    }

    int tcount = 0;

    for(ship_manager* sm : fleets)
    {
        tcount %= bound;

        bool in_combat = sm->any_in_combat();

        if(tcount != internal_counter)// && !in_combat)
        {
            tcount++;
            continue;
        }

        sm->tick_all(sm->accumulated_dt);

        sm->accumulated_dt = 0;

        ///all ships now have ai
        /*bool is_ai = false;

        if(sm->parent_empire != nullptr && sm->parent_empire->has_ai)
        {
            is_ai = true;
        }

        if(!in_combat && !is_ai && sm->auto_resupply && sm->should_resupply() && sm->parent_empire)
        {
            sm->resupply(sm->parent_empire, false);
        }*/

        tcount++;
    }

    internal_counter += 1;
    internal_counter %= bound;

    /*int bound = 5;

    for(ship_manager* sm : fleets)
    {
        sm->accumulated_dt += step_s;
    }

    for(ship_manager* sm : fleets)
    {
        if((sm->my_id % bound) != (internal_counter % bound) && !sm->any_in_combat())
            continue;

        sm->tick_all(sm->accumulated_dt);

        sm->accumulated_dt = 0;

        if(sm->auto_resupply && sm->should_resupply() && sm->parent_empire)
        {
            sm->resupply(sm->parent_empire, false);
        }
    }


    internal_counter++;
    internal_counter %= bound;*/
}

ship* fleet_manager::nearest_free_colony_ship_of_empire(orbital* o, empire* e)
{
    orbital_system* os = o->parent_system;

    float min_dist = 999999.f;
    ship* min_ship = nullptr;

    for(orbital* test_orbital : os->orbitals)
    {
        if(test_orbital->parent_empire != e)
            continue;

        if(test_orbital->type != orbital_info::FLEET)
            continue;

        if(test_orbital->parent_system != os)
            continue;

        ship_manager* sm = (ship_manager*)test_orbital->data;

        vec2f pos = test_orbital->absolute_pos;
        vec2f base_pos = o->absolute_pos;

        float dist = (pos - base_pos).length();

        if(dist < min_dist)
        {
            ship* s = nullptr;

            for(ship* found : sm->ships)
            {
                if(found->colonising)
                    continue;

                component* fc = found->get_component_with_primary(ship_component_elements::COLONISER);

                if(fc != nullptr)
                {
                    s = found;

                    break;
                }
            }

            if(s != nullptr)
            {
                min_ship = s;
                min_dist = dist;
            }
        }
    }

    return min_ship;
}

void fleet_manager::do_serialise(serialise& s, bool ser)
{
    if(serialise_data_helper::send_mode == 1)
    {
        s.handle_serialise(internal_counter, ser);
        s.handle_serialise(fleets, ser);
    }

    if(serialise_data_helper::send_mode == 0)
    {
        s.handle_serialise(fleets, ser);
    }
}

void fleet_manager::erase_all()
{
    for(ship_manager* sm : fleets)
    {
        for(ship* s : sm->ships)
        {
            delete s;
        }

        delete sm;
    }

    fleets.clear();
}

void fleet_manager::shuffle_networked_ships()
{
    for(ship_manager* sm : fleets)
    {
        for(int i=0; i<sm->ships.size(); i++)
        {
            ship* s = sm->ships[i];

            if(s->owned_by != sm)
            {
                sm->ships.erase(sm->ships.begin() + i);
                i--;
                continue;
            }
        }
    }
}
