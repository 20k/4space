#include "profile.hpp"
#include <chrono>
#include <imgui/imgui.h>
#include <iostream>

#ifdef ENABLE_PROFILING

std::string auto_timer::last_func;
int auto_timer::last_line;

std::map<timer_info, timer_data> auto_timer::info;

auto_timer::auto_timer(const std::string& pfunc, int pline) : func(pfunc), line(pline){}

auto_timer::~auto_timer()
{
    if(!finished)
        finish();
}

void auto_timer::start()
{
    start_s = std::chrono::high_resolution_clock::now();

    last_func = func;
    last_line = line;
}

void auto_timer::finish()
{
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - start_s);

    double time = time_span.count();

    auto_timer::info[{func, line}].time_s += time;
    //auto_timer::info[{func, line}].num ++;

    finished = true;
}

void auto_timer::increment_last_num()
{
    auto_timer::info[{last_func, last_line}].num++;
}

void auto_timer::increment_all()
{
    for(auto& i : info)
    {
        i.second.num++;
    }
}

void auto_timer::reset()
{
    info.clear();
}

void auto_timer::reduce()
{
    for(std::pair<const timer_info, timer_data>& dat : info)
    {
        const timer_info& inf = dat.first;
        timer_data& data = dat.second;

        int num = data.num;

        double time = data.time_s;

        time = time / num;

        data.num = 50;
        data.time_s = time * data.num;
    }
}

void auto_timer::dump()
{
    for(const std::pair<timer_info, timer_data>& dat : info)
    {
        const timer_info& inf = dat.first;
        const timer_data& data = dat.second;

        std::cout << inf.function_name << std::endl;
        std::cout << inf.line << std::endl;

        if(data.num <= 0)
            continue;

        double avg = (data.time_s / data.num) * 1000.f;

        std::cout << std::to_string(avg) << std::endl;
    }
}

void auto_timer::dump_imgui()
{
    if(info.size() == 0)
        return;

    ImGui::Begin("Perf");

    for(const std::pair<timer_info, timer_data>& dat : info)
    {
        const timer_info& inf = dat.first;
        const timer_data& data = dat.second;

        ImGui::Text(inf.function_name.c_str());
        ImGui::Text(std::to_string(inf.line).c_str());

        if(data.num <= 0)
            continue;

        double avg = (data.time_s / data.num) * 1000.f;

        ImGui::Text(std::to_string(avg).c_str());

        ImGui::NewLine();

        //std::cout << std::to_string(avg) << std::endl;
    }

    ImGui::End();
}
#endif
