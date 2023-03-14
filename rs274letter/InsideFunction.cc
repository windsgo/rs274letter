#include "InsideFunction.h"

#include <unordered_set>

namespace rs274letter
{

/**
 * @brief inside function names in the rs274 linuxcnc manual
*/
static std::unordered_set<std::string> s_insidefunction_set = {
    "atan",
    "abs",
    "acos",
    "asin",
    "cos",
    "exp",
    "fix",
    "fup",
    "round",
    "ln",
    "sin",
    "sqrt",
    "tan",
    "exists"
};

bool IsInsideFunction(const std::string & function_name)
{
    return s_insidefunction_set.find(function_name) != s_insidefunction_set.end();
}

    
} // namespace rs274letter
