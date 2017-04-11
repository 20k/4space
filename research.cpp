#include "research.hpp"
#include "util.hpp"
#include "empire.hpp"

#include "../../render_projects/imgui/imgui.h"

float research_info::get_research_level_cost(int level, float cost, bool testing, float scaling)
{
    if(testing)
        return 2;

    if(level == 0)
        return cost;

    float fv = get_research_level_cost(level - 1, cost, testing, scaling) * scaling;

    return fv;
}

float research_info::get_cost_scaling(float level, float base_cost)
{
    float val = get_research_level_cost(level, base_cost, false, 3.f);

    return val;
}

float research_info::tech_unit_to_research_currency(float tech_unit, bool has_minimum_value)
{
    ///tech UNITS not currency, 1 is quite a long way
    if(tech_unit < 1/20.f && has_minimum_value)
        tech_unit = 1/20.f;

    if(tech_unit < 0)
        tech_unit = 0;

    return tech_unit * 20.f;
}

research::research()
{
    for(int i=0; i<(int)research_info::COUNT; i++)
    {
        categories.push_back({(research_info::types)i, 0.f});
    }
}

void research::draw_ui(empire* emp)
{
    std::vector<std::string> fnames = research_info::names;
    std::vector<std::string> amounts;

    for(auto& i : categories)
    {
        amounts.push_back(to_string_with_variable_prec(i.amount));
    }

    ImGui::Begin("Research");

    for(int i=0; i<fnames.size(); i++)
    {
        std::string display = format(fnames[i], fnames) + " | " + format(amounts[i], amounts);

        ImGui::Text(display.c_str());


        int research_level = categories[i].amount;

        float research_cost = research_info::get_research_level_cost(research_level);

        float current_research_resource = emp->resources.resources[resource::RESEARCH].amount;

        //if(research_cost > current_research_resource)
        //    continue;

        std::string purchase_string = "| (Purchase for " + std::to_string((int)research_cost) + ")";

        ImGui::SameLine();

        ImGui::Text(purchase_string.c_str());

        ///we need an are you sure dialogue, but that will come with popup notifications
        if(ImGui::IsItemClicked() && research_cost < current_research_resource)
        {
            emp->resources.resources[resource::RESEARCH].amount -= research_cost;

            categories[i].amount += 1;
        }
    }

    ImGui::End();
}

void research::tick(float step_s)
{

}

void research::add_amount(const research_category& category)
{
    categories[category.type].amount += category.amount;
}

void research::add_amount(research_info::types type, float amount)
{
    categories[type].amount += amount;

    categories[type].amount = std::max(categories[type].amount, 0.f);
}

float research::units_to_currency(bool has_minimum_value)
{
    float accum = 0;

    for(auto& i : categories)
    {
        accum += research_info::tech_unit_to_research_currency(i.amount, has_minimum_value);
    }

    return accum;
}

float research::level_to_currency()
{
    float val = 0.f;

    for(int i=0; i<categories.size(); i++)
    {
        val += research_info::get_research_level_cost(categories[i].amount);
    }

    return val;
}

research research::div(float amount)
{
    for(auto& i : categories)
    {
        i.amount /= amount;
    }

    return *this;
}
