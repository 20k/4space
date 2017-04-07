#include "research.hpp"
#include "util.hpp"
#include "empire.hpp"

#include "../../render_projects/imgui/imgui.h"

float research_info::get_research_level_cost(int level, float cost)
{
    #define TESTING
    #ifdef TESTING
    return 2;
    #endif

    if(level == 0)
        return cost;

    return get_research_level_cost(level - 1) * 4.f * cost;
}

float research_info::get_cost_scaling(float level, float base_cost)
{
    return get_research_level_cost(level, base_cost);
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
