#ifndef STACKTRACE_HELPER_HPP_INCLUDED
#define STACKTRACE_HELPER_HPP_INCLUDED

#define BOOST_STACKTRACE_LINK
#define BOOST_STACKTRACE_USE_WINDBG

#include <signal.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <fstream>

inline
void my_signal_handler(int signum)
{
    ::signal(signum, SIG_DFL);
    boost::stacktrace::safe_dump_to("./backtrace.dump");
    ::raise(SIGABRT);
}

inline
void register_signal_handlers()
{
    ::signal(SIGSEGV, &my_signal_handler);
    ::signal(SIGABRT, &my_signal_handler);
}

inline
bool file_exists(const std::string& name)
{
    std::ifstream infile(name);
    return infile.good();
}

inline
void check_stacktrace()
{
    if(file_exists("./backtrace.dump"))
    {
        // there is a backtrace
        std::ifstream ifs("./backtrace.dump");

        boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
        std::cout << "Previous run crashed:\n" << st << std::endl;

        // cleaning up
        ifs.close();
        std::remove("./backtrace.dump");
        //boost::filesystem::remove("./backtrace.dump");
    }
}

#endif // STACKTRACE_HELPER_HPP_INCLUDED
