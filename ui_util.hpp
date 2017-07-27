#ifndef UI_UTIL_HPP_INCLUDED
#define UI_UTIL_HPP_INCLUDED

#include <imgui/imgui.h>

void imgui_hp_bar(float fraction, vec3f col, vec2f dim)
{
    constexpr int num_divisions = 100;

    float num_filled = floor(fraction * num_divisions);

    float extra = fraction * num_divisions - num_filled;

    float vals[num_divisions];

    for(int i=0; i<num_divisions; i++)
    {
        if(i < num_filled)
        {
            vals[i] = 1.f;
        }
        else if(i == num_filled)
        {
            vals[i] = extra;
        }
        else
        {
            vals[i] = 0;
        }
    }

    ImGui::PlotHistogram("", vals, num_divisions, 0, nullptr, 0, 1, ImVec2(dim.x(), dim.y()));
}

#endif // UI_UTIL_HPP_INCLUDED
