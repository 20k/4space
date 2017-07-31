#ifndef TOOLTIP_HANDLER_HPP_INCLUDED
#define TOOLTIP_HANDLER_HPP_INCLUDED

#include <string>

struct tooltip
{
    static std::string tool;

    static void add(const std::string& str);

    static void set_clear_tooltip();
};

#endif // TOOLTIP_HANDLER_HPP_INCLUDED
