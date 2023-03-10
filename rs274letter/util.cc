#include "util.h"

#if (defined (__unix__)) && (!defined (_WIN32))
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <execinfo.h>

#include <iostream>
#include <sstream>

#ifdef BOOST_STACKTRACE_USE_BACKTRACE
#include <boost/stacktrace.hpp>
#else // BOOST_STACKTRACE_USE_BACKTRACE
#include <backtrace.h>
#endif // BOOST_STACKTRACE_USE_BACKTRACE
#endif // (defined (__unix__)) && (!defined (_WIN32))


namespace rs274letter { namespace util 
{

void Backtrace([[maybe_unused]] std::vector<std::string> &bt, [[maybe_unused]] int size, [[maybe_unused]] int skip) {
#if (defined (__unix__)) && (!defined (_WIN32))
#ifdef BOOST_STACKTRACE_USE_BACKTRACE
    auto vec = boost::stacktrace::stacktrace().as_vector();
    bt.reserve(vec.size() - skip);
    for (size_t i = skip; i < vec.size(); ++i) {
        std::stringstream ss;
        ss << vec[i];
        bt.push_back(ss.str());
    }
#else // BOOST_STACKTRACE_USE_BACKTRACE
    void **array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if (strings == NULL) {
        std::cout << "backtrace_symbols error" << std::endl;
        free(strings);
        free(array);
        return;
    }

    bt.reserve(s - skip);
    for (size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
#endif // BOOST_STACKTRACE_USE_BACKTRACE
#else // (defined (__unix__)) && (!defined (_WIN32))
    return;
#endif // (defined (__unix__)) && (!defined (_WIN32))
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::stringstream ss;
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << i << "# " << bt[i] << std::endl;
    }
    return ss.str();
}

} // namespace util
} // namespace rs274letter
