#include "ship.hpp"

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

void component_attribute::update_time(float step_s)
{
    current_time_s += step_s;
}

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
std::map<ship_component_element, float> component::get_timestep_diff(float step_s)
{
    std::map<ship_component_element, float> ret;

    for(auto& i : components)
    {
        auto type = i.first;
        auto attr = i.second;

        float net = (attr.produced_per_s - attr.drained_per_s) * step_s;

        ret[type] = net;
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

std::pair<component, std::map<ship_component_element, float>> component::apply_diff(const std::map<ship_component_element, float>& diff)
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

void component::add(ship_component_element element, const component_attribute& attr)
{
    if(has_element(element))
    {
        printf("DUPLICATE ELEMENT ADDED RUH ROH");
        throw;
    }

    components[element] = attr;
}


std::map<ship_component_element, float> ship::tick_all_components(float step_s)
{
    std::map<ship_component_element, float> total_to_apply;

    for(component& c : entity_list)
    {
        auto diff = c.get_timestep_diff(step_s);

        total_to_apply = merge_diffs(total_to_apply, diff);
    }

    //printf("%f POWER\n", total_to_apply[ship_component_element::ENERGY]);

    std::map<ship_component_element, float> available_capacities = get_available_capacities();

    ///how to apply the output to systems fairly? Try and distribute evenly? Proportionally?
    ///proportional seems reasonable atm
    for(component& c : entity_list)
    {
        std::map<ship_component_element, float> this_entity_available = c.get_available_capacities();

        for(auto& i : this_entity_available)
        {
            if(available_capacities[i.first] <= 0.001f)
                continue;

            float proportion = i.second / available_capacities[i.first];

            float applying_to_this = proportion * total_to_apply[i.first];

            std::map<ship_component_element, float> tmap;

            tmap[i.first] = applying_to_this;

            auto r = c.apply_diff(tmap);

            c = r.first;

            ///can be none left over as we're using available capacities
            auto left_over = r.second;
        }
    }

    ///so amount left over is total_to_apply - available_capacities

    std::map<ship_component_element, float> left_over;

    for(auto& i : total_to_apply)
    {
        left_over[i.first] = i.second - available_capacities[i.first];
    }

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

void ship::add(const component& c)
{
    entity_list.push_back(c);
}
