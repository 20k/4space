#include "ship.hpp"
#include "battle_manager.hpp"

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

    sf::RectangleShape r1;

    r1.setFillColor(sf::Color(255, 255, 255));
    r1.setSize({dim.x(), dim.y()});
    r1.setPosition(0, 0);

    intermediate_texture->draw(r1);

    int granularity = min_bound/4;

    sf::RectangleShape corner;
    corner.setSize({granularity, granularity});
    corner.setFillColor(sf::Color(0,0,0));
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

        intermediate_texture->draw(corner);
    }

    sf::RectangleShape chunk = corner;


    //chunk.setRotation(0.f);

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

        intermediate_texture->draw(chunk);

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
