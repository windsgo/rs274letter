#pragma once

#include <exception>
#include <string>

namespace rs274letter {

class Exception : std::exception {
protected:
    std::string _str;

public:
    Exception(std::string str) : _str(str) {}
    virtual const char* what() const noexcept override {
        return _str.c_str();
    }
};

}