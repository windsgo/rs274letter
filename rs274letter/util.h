#pragma once

#include <string>
#include <vector>

namespace rs274letter { namespace util
{

void Backtrace(std::vector<std::string> &bt, [[maybe_unused]] int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "    ");

} // namespace util
} // namespace rs274letter 
