#ifndef PROFILE_HPP_INCLUDED
#define PROFILE_HPP_INCLUDED

#include <chrono>
#include <map>
#include <string>

struct timer_info
{
    std::string function_name;
    int line;
};

struct timer_data
{
    int num = 0;
    double time_s = 0.f;
};

inline
bool operator<(const timer_info& t1, const timer_info& t2)
{
    if(t1.function_name != t2.function_name)
    {
        return t1.function_name < t2.function_name;
    }

    return t1.line < t2.line;
}

struct auto_timer
{
    std::chrono::high_resolution_clock::time_point start_s;

    std::string func;
    int line;

    static std::string last_func;
    static int last_line;

    auto_timer(const std::string& func, int line);

    void start();
    void finish();

    static void increment_last_num();
    static void increment_all();
    static void reduce();

    static std::map<timer_info, timer_data> info;

    static void reset();
    static void dump();
    static void dump_imgui();
};

#define MAKE_AUTO_TIMER() auto_timer(__PRETTY_FUNCTION__, __LINE__)

#endif // PROFILE_HPP_INCLUDED
