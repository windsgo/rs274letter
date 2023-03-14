#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <type_traits>
#include <iostream>

namespace rs274letter { namespace util
{

void Backtrace(std::vector<std::string> &bt, [[maybe_unused]] int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "    ");

template <typename _ChronoUnit = std::chrono::microseconds>
class ElapsedTimer {
private:
    std::chrono::high_resolution_clock::time_point _p;
    std::string _name;
    std::string _unit;
    bool _print;
public:
    ElapsedTimer(const std::string& name="default time counter", bool print_at_destruction = true) 
        : _name(name), _print(print_at_destruction) {
#define __lXX(T) \
    constexpr (std::is_same<_ChronoUnit, std::chrono::T>::value) { _unit = #T; }
      
        if __lXX(seconds)
        else if __lXX(milliseconds)
        else if __lXX(microseconds)
        else if __lXX(nanoseconds)
        else { _unit = "unknown unit"; }
#undef __lXX
        _p = std::chrono::high_resolution_clock::now();
    }
  
    ~ElapsedTimer() {
        if (_print) {
        auto n = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<_ChronoUnit>(n - _p).count();
        std::cout << _name << ": " << time << "(" << _unit << ")" << std::endl;
        }
    }

    template<typename T = _ChronoUnit>
    uint64_t elapsed() const {
        auto n = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<T>(n - _p).count();
    }

    void reset() {
        _p = std::chrono::high_resolution_clock::now();
    }
};

} // namespace util
} // namespace rs274letter 
