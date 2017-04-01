#include "ship.hpp"
#include <math.h>
#include <vec/vec.hpp>

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
    return current_time_s >= time_last_used_s + time_between_uses_s;
}

float component_attribute::get_available_capacity()
{
    return max_amount - cur_amount;
}

float component_attribute::get_net()
{
    return produced_per_s - drained_per_s;
}

void component_attribute::update_time(float step_s)
{
    current_time_s += step_s;

    currently_drained = 0;

    available_for_consumption = produced_per_s * step_s;
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

float component_attribute::consume_from(component_attribute& other, float max_proportion, float step_s)
{
    if(drained_per_s <= 0.001f)
        return 0.f;

    float max_drain_amount = drained_per_s * max_proportion * step_s;

    float needed_drain = drained_per_s * step_s - currently_drained;

    needed_drain = std::max(needed_drain, 0.f);

    float to_drain = std::min(max_drain_amount, needed_drain);

    float amount_drained = other.consume_max(to_drain);

    currently_drained += amount_drained;
    cur_efficiency = currently_drained / drained_per_s;

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

        float net = (attr.produced_per_s) * step_s * attr.cur_efficiency;

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

float component::calculate_total_efficiency()
{
    float min_eff = 1.f;

    for(auto& i : components)
    {
        if(i.second.cur_efficiency < min_eff)
        {
            min_eff = i.second.cur_efficiency;
        }
    }

    return min_eff;
}

void component::propagate_total_efficiency()
{
    float min_eff = calculate_total_efficiency();

    for(auto& i : components)
    {
        i.second.cur_efficiency = min_eff;
    }
}

std::map<ship_component_element, float> ship::tick_all_components(float step_s)
{
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

    std::map<ship_component_element, float> to_apply_prop;

    for(auto& i : needed)
    {
        if(stored_and_produced[i.first] <= 0.001f || i.second <= 0.001f)
            continue;

        ///so this is the overall proportion of whats to be drained vs what needs to be drained
        float frac = stored_and_produced[i.first] / i.second;

        to_apply_prop[i.first] = frac;
    }

    //printf("%f %f stap\n", stored_and_produced[ship_component_elements::OXYGEN], to_apply_prop[ship_component_elements::OXYGEN]);

    for(auto& i : entity_list)
    {
        for(auto& c : i.components)
        {
            float my_proportion_of_total = to_apply_prop[c.first];

            component_attribute& me = c.second;

            /*float amount_to_drain = me.drained_per_s;

            float available_amount_to_try = stored_and_produced[c.first] * my_proportion_of_total;

            float to_drain = std::min(amount_to_drain, available_amount_to_try);

            float frac = 0.f;

            if(amount_to_drain > 0.001f)
                frac = to_drain / amount_to_drain;*/

            float frac = my_proportion_of_total;

            if(frac > 1)
                frac = 1;

            /*if(c.first == ship_component_elements::OXYGEN)
            {
                printf("%f frac\n", frac);
            }*/

            for(auto& k : entity_list)
            {
                for(auto& c2 : k.components)
                {
                    if(c2.first != c.first)
                        continue;

                    component_attribute& other = c2.second;

                    ///ie the amount we actually took from other
                    float drained = me.consume_from(other, frac, step_s);

                    produced[c.first] -= drained;
                }

            }
        }
    }

    for(auto& i : produced)
    {
        if(i.second < -0.0001f)
        {
            printf("Logic error somewhere\n");
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
            if(available_capacities[i.first] <= 0.001f)
                continue;

            float proportion = i.second / available_capacities[i.first];

            float applying_to_this = proportion * produced[i.first];

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

    for(auto& c : entity_list)
    {
        c.propagate_total_efficiency();
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

    distribute_resources(diff);
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
            if(available_capacities[i.first] <= 0.001f)
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
