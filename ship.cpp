#include "ship.hpp"
#include "battle_manager.hpp"
#include "empire.hpp"
#include "text.hpp"
#include "system_manager.hpp"

int ship::gid;

std::map<resource::types, float> ship_component_elements::component_storage_to_resources(types& type)
{
    std::map<resource::types, float> ret;

    if(type == AMMO)
    {
        ret[resource::IRON] = 1;
    }

    if(type == FUEL)
    {
        ret[resource::URANIUM] = 1;
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

    return ret;
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

float component_attribute::get_available_capacity()
{
    return max_amount - cur_amount;
}

float component_attribute::get_total_capacity(float step_s)
{
    return get_available_capacity() + (drained_per_s * step_s - currently_drained);
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
    cur_efficiency = currently_drained / (drained_per_s * step_s);
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

bool component::has_element(const ship_component_element& type)
{
    return components.find(type) != components.end();
}

component_attribute component::get_element(const ship_component_element& type)
{
    if(!has_element(type))
        return component_attribute();

    return components[type];
}

std::map<ship_component_element, float> merge_diffs(const std::map<ship_component_element, float>& one, const std::map<ship_component_element, float>& two)
{
    std::map<ship_component_element, float> ret = one;

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

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float net = attr.get_produced_amount(step_s);

        ret[type] = net;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_timestep_consumption_diff(float step_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float net = (attr.drained_per_s) * step_s;

        ret[type] = net;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float val = attr.cur_amount;

        ret[type] = val;
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored_max()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float val = attr.max_amount;

        ret[type] = val;
    }

    return ret;
}

///how to account for max storage? Apply_diff?
///need to keep track of time
std::map<ship_component_element, float> component::get_use_diff()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float net = 0.f;

        if(attr.can_use())
            net = (attr.produced_per_use - attr.drained_per_use);

        ret[type] = net;
    }

    return ret;
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

        auto attr = components[type];

        float extra = attr.add_amount(diff_amount);

        components[type] = attr;
        ret_extra[type] += extra;
    }

    return ret_extra;
}

void component::update_time(float step_s)
{
    for(auto& i : components)
    {
        i.second.update_time(step_s);
    }
}

std::map<ship_component_element, float> component::get_available_capacities()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        ret[i.first] = i.second.get_available_capacity();
    }

    return ret;
}

std::map<ship_component_element, float> component::get_stored_and_produced_resources(float time_s)
{
    std::map<ship_component_element, float> ret = get_timestep_production_diff(time_s);

    for(auto& i : components)
    {
        ret[i.first] += i.second.cur_amount;
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
}

float component::calculate_total_efficiency(float step_s)
{
    float min_eff = 1.f;

    for(auto& i : components)
    {
        i.second.calculate_efficiency(step_s);

        if(i.second.cur_efficiency < min_eff)
        {
            min_eff = i.second.cur_efficiency;
        }
    }

    float max_hp = components[ship_component_element::HP].max_amount;
    float cur_hp = components[ship_component_element::HP].cur_amount;

    float frac = 1.f;

    if(max_hp > 0)
    {
        frac = cur_hp / max_hp;
    }

    return min_eff * frac;
}

void component::propagate_total_efficiency(float step_s)
{
    float min_eff = calculate_total_efficiency(step_s);

    for(auto& i : components)
    {
        i.second.cur_efficiency = min_eff;
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

bool component::has_tag(component_tag::tag tag)
{
    return tag_list.find(tag) != tag_list.end();
}

float component::get_tag(component_tag::tag tag)
{
    ///preserve integrity of map?
    if(!has_tag(tag))
    {
        printf("warning: Tag fuckup in get_tag %i %s\n", tag, name.c_str());

        return 0.f;
    }

    return tag_list[tag];
}

void component::set_tag(component_tag::tag tag, float val)
{
    tag_list[tag] = val;
}

std::map<ship_component_element, float> ship::tick_all_components(float step_s)
{
    //if(step_s < 0.1)
    //    step_s = 0.1;

    /*std::map<ship_component_element, float> total_production;

    for(component& c : entity_list)
    {
        auto diff = c.get_timestep_production_diff(step_s);

        total_production = merge_diffs(total_production, diff);
    }

    //printf("%f POWER\n", total_to_apply[ship_component_element::ENERGY]);

    std::map<ship_component_element, float> available_capacities = get_available_capacities();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        std::map<ship_component_element, float> this_entity_available = c.get_available_capacities();

        for(auto& i : this_entity_available)
        {
            if(available_capacities[i.first] <= 0.001f)
                continue;

            float proportion = i.second / available_capacities[i.first];

            float applying_to_this = proportion * total_production[i.first];

            std::map<ship_component_element, float> tmap;

            tmap[i.first] = applying_to_this;

            auto r = c.apply_diff(tmap);

            ///can be none left over as we're using available capacities
            auto left_over = r;
        }
    }

    ///so amount left over is total_to_apply - available_capacities

    std::map<ship_component_element, float> left_over;

    for(auto& i : total_production)
    {
        left_over[i.first] = i.second - available_capacities[i.first];
    }




    ///so now we've put production into storage,

    return left_over;*/

    for(auto& i : entity_list)
    {
        i.update_time(step_s);
    }

    ///this covers the drain side
    std::map<ship_component_element, float> stored_and_produced = get_stored_and_produced_resources(step_s);
    std::map<ship_component_element, float> needed = get_needed_resources(step_s);
    std::map<ship_component_element, float> produced = get_produced_resources(step_s);
    std::map<ship_component_element, float> stored = get_stored_resources();

    std::map<ship_component_element, float> to_apply_prop;

    for(auto& i : needed)
    {
        if(produced[i.first] <= FLOAT_BOUND || i.second <= FLOAT_BOUND)
            continue;

        float available_to_take_now = produced[i.first];
        float total_resource = stored_and_produced[i.first];

        float valid_to_take_frac = available_to_take_now / total_resource;

        ///so this is the overall proportion of whats to be drained vs what needs to be drained
        //float frac = produced[i.first] / i.second;

        float frac = i.second / produced[i.first];

        to_apply_prop[i.first] = frac;
    }

    //printf("%f need\n", needed[ship_component_element::OXYGEN]);

    //printf("%f stap\n", produced[ship_component_elements::OXYGEN]);
    //printf("%f %f stap\n", stored_and_produced[ship_component_elements::OXYGEN], to_apply_prop[ship_component_elements::OXYGEN]);

    ///change to take first from production, then from storage instead of weird proportional
    for(auto& i : entity_list)
    {
        for(auto& c : i.components)
        {
            float my_proportion_of_total = to_apply_prop[c.first];

            component_attribute& me = c.second;

            float frac = my_proportion_of_total;

            ///dis broken
            //if(c.first == ship_component_element::ENERGY)
            //    printf("Dfrac %f\n", frac);

            if(frac > 1)
                frac = 1;

            float extra = 0;

            for(auto& k : entity_list)
            {
                for(auto& c2 : k.components)
                {
                    if(c2.first != c.first)
                        continue;

                    component_attribute& other = c2.second;

                    float take_amount = frac * other.get_produced_amount(step_s) + extra;

                    if(take_amount > me.get_total_capacity(step_s))
                        take_amount = me.get_total_capacity(step_s);

                    /*if(c.first == ship_component_element::OXYGEN)
                    {
                        printf("%f atake\n", take_amount);
                    }*/

                    ///ie the amount we actually took from other
                    float drained = me.consume_from_amount_available(other, take_amount, step_s);

                    /*if(c.first == ship_component_element::OXYGEN)
                    {
                        printf("%f ataken\n", drained);
                    }*/

                    produced[c.first] -= drained;
                    needed[c.first] -= drained;

                    extra += (take_amount - drained);
                }

            }
        }
    }

    to_apply_prop.clear();

    for(auto& i : needed)
    {
        if(stored[i.first] <= FLOAT_BOUND)
            continue;

        if(i.second <= FLOAT_BOUND)
            continue;

        ///no produced left if we're dipping into stored
        //float frac = stored[i.first] / i.second;

        ///take frac from all
        ///ok we can't just use stored because there might still be some left in the produced section
        ///because we're taking proportionally
        float frac = i.second / stored[i.first];

        to_apply_prop[i.first] = frac;
    }

    for(auto& i : entity_list)
    {
        for(auto& c : i.components)
        {
            float my_proportion_of_total = to_apply_prop[c.first];

            component_attribute& me = c.second;

            float frac = my_proportion_of_total;

            ///dis broken
            //if(c.first == ship_component_element::ENERGY)
            //    printf("Dfrac %f\n", frac);

            if(frac > 1)
                frac = 1;

            float extra = 0;

            for(auto& k : entity_list)
            {
                for(auto& c2 : k.components)
                {
                    if(c2.first != c.first)
                        continue;

                    component_attribute& other = c2.second;

                    float take_amount = frac * other.cur_amount + extra;

                    if(take_amount > me.get_total_capacity(step_s))
                        take_amount = me.get_total_capacity(step_s);

                    /*if(c2.first == ship_component_element::OXYGEN)
                    {
                        printf("%f taking from %s\n", take_amount, k.name.c_str());
                    }*/

                    ///ie the amount we actually took from other
                    float drained = me.consume_from_amount_stored(other, take_amount, step_s);

                    /*if(c2.first == ship_component_element::OXYGEN)
                    {
                        printf("%f taken\n", drained);
                    }*/

                    ///incorrect, this is taken from STORAGE not PRODUCED
                    //produced[c.first] -= drained;
                    needed[c.first] -= drained;

                    extra += (take_amount - drained);
                }
            }
        }
    }

    /*for(auto& i : needed)
    {
        if()
    }*/

    for(auto& i : produced)
    {
        if(i.second < -0.0001f)
        {
            printf("Logic error somewhere\n");

            i.second = 0;
        }
    }

    //printf("leftover %f\n", produced[ship_component_element::ENERGY]);


    std::map<ship_component_element, float> available_capacities = get_available_capacities();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        std::map<ship_component_element, float> this_entity_available = c.get_available_capacities();

        for(auto& i : this_entity_available)
        {
            if(available_capacities[i.first] <= FLOAT_BOUND)
                continue;

            float proportion = i.second / available_capacities[i.first];

            float applying_to_this = proportion * produced[i.first];

            std::map<ship_component_element, float> tmap;

            tmap[i.first] = applying_to_this;

            /*if(i.first == ship_component_elements::OXYGEN)
            {
                printf("test %f\n", applying_to_this);
            }*/

            auto r = c.apply_diff(tmap);

            ///can be none left over as we're using available capacities
            auto left_over = r;
        }
    }

    for(auto& c : entity_list)
    {
        c.propagate_total_efficiency(step_s);
    }

    ///so amount left over is total_to_apply - available_capacities

    std::map<ship_component_element, float> left_over;

    for(auto& i : produced)
    {
        left_over[i.first] = i.second - available_capacities[i.first];
    }


    ///so now we've put production into storage,

    ///this is capacites shit, ignore me
    return left_over;
}

void ship::tick_other_systems(float step_s)
{
    disengage_clock_s += step_s;
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
}

bool ship::in_combat()
{
    return currently_in_combat;
}

std::map<ship_component_element, float> ship::get_available_capacities()
{
    std::map<ship_component_element, float> ret;

    for(auto& i : entity_list)
    {
        ret = merge_diffs(ret, i.get_available_capacities());
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

bool ship::can_use(component& c)
{
    std::map<ship_component_element, float> requirements;

    ///+ve means we need input from the ship to power it
    for(auto& celem : c.components)
    {
        component_attribute& attr = celem.second;

        requirements[celem.first] = attr.drained_per_use - attr.produced_per_use;

        if(!attr.can_use())
            return false;
    }

    auto stored = get_stored_resources();

    for(auto& i : requirements)
    {
        if(stored[i.first] < requirements[i.first])
            return false;
    }

    return true;
}

void ship::use(component& c)
{
    if(!can_use(c))
        return;

    auto diff = c.get_use_diff();

    for(auto& elems : c.components)
    {
        elems.second.use();
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

void ship::distribute_resources(std::map<ship_component_element, float> res)
{
    std::map<ship_component_element, float> available_capacities = get_available_capacities();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    ///ok so this step distributes to all the individual storage
    for(component& c : entity_list)
    {
        std::map<ship_component_element, float> this_entity_available = c.get_available_capacities();

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
    entity_list.push_back(c);
}

void ship::hit(projectile* p)
{
    float shields = get_stored_resources()[ship_component_element::SHIELD_POWER];
    float armour = get_stored_resources()[ship_component_element::ARMOUR];

    float damage = p->base.get_tag(component_tag::DAMAGE);

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

    component& h = entity_list[random_component];

    std::map<ship_component_element, float> hp_diff;
    hp_diff[ship_component_element::HP] = -hp_damage;

    h.apply_diff(hp_diff);
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

void ship::generate_image(vec2i dim)
{
    float min_bound = std::min(dim.x(), dim.y());

    tex.setSmooth(true);

    intermediate_texture = new sf::RenderTexture;
    intermediate_texture->create(dim.x(), dim.y());
    intermediate_texture->setSmooth(true);

    sf::BlendMode blend(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero);

    sf::RectangleShape r1;

    r1.setFillColor(sf::Color(255, 255, 255));
    r1.setSize({dim.x(), dim.y()});
    r1.setPosition(0, 0);

    intermediate_texture->draw(r1);

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

        intermediate_texture->draw(corner, blend);
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

        intermediate_texture->draw(chunk, blend);

        exclusion.push_back(pos);
    }

    intermediate_texture->display();

    tex = intermediate_texture->getTexture();
}

ship::~ship()
{
    if(intermediate_texture)
        delete intermediate_texture;
}

void ship::resupply(empire& emp, int num)
{
    std::vector<ship_component_elements::types> types =
    {
        ship_component_elements::AMMO,
        ship_component_elements::FUEL,
        ship_component_elements::ARMOUR,
        ship_component_elements::HP
    };

    for(ship_component_elements::types& type : types)
    {
        float current_capacity = get_available_capacities()[type];

        std::map<resource::types, float> requested_resource_amounts = ship_component_elements::component_storage_to_resources(type);

        ///so we request twice as much... but we'll only take vanilla amounts
        for(auto& i : requested_resource_amounts)
            i.second *= current_capacity*num;

        float efficiency_frac;

        std::map<resource::types, float> gotten = emp.dispense_resources_proportionally(requested_resource_amounts, 1.f/num, efficiency_frac);

        std::map<ship_component_elements::types, float> to_add;
        to_add[type] = efficiency_frac * current_capacity;

        distribute_resources(to_add);
    }
}

bool ship::can_move_in_system()
{
    float threshold_working_efficiency = 0.75f;

    for(component& c : entity_list)
    {
        if(c.has_element(ship_component_elements::ENGINE_POWER))
        {
            component_attribute elem = c.get_element(ship_component_elements::ENGINE_POWER);

            if(elem.cur_efficiency >= threshold_working_efficiency)
                return true;
        }
    }

    return false;
}

void ship::apply_disengage_penalty()
{
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
}

bool ship::can_disengage()
{
    const float mandatory_combat_time_s = combat_variables::mandatory_combat_time_s;

    if(time_in_combat_s < mandatory_combat_time_s)
        return false;

    return true;
}

bool ship::can_engage()
{
    const float disengagement_timer_s = combat_variables::disengagement_time_s;

    if(is_disengaging)
    {
        if(disengage_clock_s < disengagement_timer_s)
            return false;

        return true;
    }

    return true;
}

ship* ship_manager::make_new(int team)
{
    ship* s = new ship;

    ships.push_back(s);

    s->team = team;

    s->owned_by = this;

    return s;
}

ship* ship_manager::make_new_from(int team, const ship& ns)
{
    ship* s = new ship(ns);

    ships.push_back(s);

    s->team = team;

    s->owned_by = this;

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

    for(auto& i : ships)
    {
        ret.push_back(i->name);
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

    s->owned_by = this;

    ships.push_back(s);
}

void ship_manager::resupply()
{
    if(parent_empire == nullptr)
        return;

    int num = ships.size();

    for(ship* s : ships)
    {
        s->resupply(*parent_empire, num);

        num--;
    }
}

void ship_manager::tick_all(float step_s)
{
    for(ship* s : ships)
    {
        s->tick_all_components(step_s);
        s->tick_other_systems(step_s);
    }
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

void ship_manager::draw_alerts(sf::RenderWindow& win, vec2f abs_pos)
{
    float fuel_yellow_alert = 0.2f;
    float fuel_red_alert = 0.1f;

    vec3f rcol = {1, 0.1, 0};
    vec3f ycol = {1, 0.9, 0};

    std::string alert_symbol;

    vec3f alert_colour;
    bool any_alert = false;

    for(ship* s : ships)
    {
        float stored = s->get_stored_resources()[ship_component_element::FUEL];
        float max_store = s->get_max_resources()[ship_component_element::FUEL];

        if(max_store < 0.001f)
        {
            continue;
        }

        float frac = stored / max_store;

        if(frac < fuel_yellow_alert)
        {
            alert_colour = ycol;
            any_alert = true;
        }

        if(frac < fuel_red_alert)
        {
            alert_colour = rcol;
        }
    }

    //if(!any_alert)
    //    return;

    if(any_alert)
        alert_symbol += "!";

    bool any_immobile = false;

    ///implement mixed colours later
    for(ship* s : ships)
    {
        if(!s->can_move_in_system())
        {
            any_immobile = true;

            alert_colour = rcol;
        }
    }

    if(any_immobile)
        alert_symbol = "!";

    if(alert_symbol == "")
        return;

    //auto transfd = win.mapCoordsToPixel({abs_pos.x(), abs_pos.y()});

    //vec2f final_pos = (vec2f){transfd.x, transfd.y} + (vec2f){10, -10};

    //printf("fpos %f %f\n", final_pos.x(), final_pos.y());

    text_manager::render(win, alert_symbol, abs_pos + (vec2f){8, -20}, alert_colour);
}

void ship_manager::try_warp(orbital_system* fin, orbital_system* cur, orbital* o)
{
    bool all_use = true;

    for(ship* s : ships)
    {
        if(!s->can_use_warp_drives())
            all_use = false;
    }

    if(!all_use)
        return;

    for(ship* s : ships)
    {
        s->use_warp_drives();
    }

    vec2f my_pos = cur->universe_pos;
    vec2f their_pos = fin->universe_pos;

    vec2f arrive_dir = (my_pos - their_pos).norm() * 500;

    fin->steal(o, cur);

    o->transferring = false;

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

    return all_use;
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

void ship_manager::apply_disengage_penalty()
{
    for(ship* s : ships)
    {
        s->apply_disengage_penalty();
    }
}

ship_manager* fleet_manager::make_new()
{
    ship_manager* ns = new ship_manager;

    fleets.push_back(ns);

    return ns;
}

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

void fleet_manager::cull_dead(empire_manager& empire_manage)
{
    for(int i=0; i < fleets.size(); i++)
    {
        if(fleets[i]->ships.size() == 0)
        {
            auto m = fleets[i];

            empire_manage.notify_removal(m);

            fleets.erase(fleets.begin() + i);

            delete m;

            i--;

            continue;
        }
    }
}

void fleet_manager::tick_all(float step_s)
{
    for(auto& i : fleets)
    {
        i->tick_all(step_s);
    }
}
