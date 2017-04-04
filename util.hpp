#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED

#include <iomanip>
#include <math.h>

template <typename T>
inline
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

template<typename T>
inline
std::string to_string_with_variable_prec(const T a_value)
{
    int n = ceil(log10(a_value)) + 1;

    if(a_value <= 0.0001f)
        n = 2;

    if(n < 2)
        n = 2;

    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

template<typename T>
inline
std::string to_string_with_enforced_variable_dp(const T a_value, int forced_dp = 1)
{
    /*std::ostringstream out;
    out << std::setprecision(n) << a_value;
    std::string fstr = out.str();*/

    std::string fstr = std::to_string(a_value);

    auto found = fstr.find('.');

    if(found == std::string::npos)
    {
        return fstr + ".0";
    }

    if(fabs(a_value) <= 0.0999999 && fabs(a_value) >= 0.0001)
        forced_dp++;

    found += forced_dp + 1;

    if(found >= fstr.size())
        return fstr;

    fstr.resize(found);

    return fstr;
}

inline
std::string format(std::string to_format, const std::vector<std::string>& all_strings)
{
    int len = 0;

    for(auto& i : all_strings)
    {
        if(i.length() > len)
            len = i.length();
    }

    for(int i=to_format.length(); i<len; i++)
    {
        to_format = to_format + " ";
    }

    return to_format;
}

#endif // UTIL_HPP_INCLUDED
