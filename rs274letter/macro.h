#pragma once

#include <string.h>
#include <assert.h>
#include <iostream>

#include "util.h"

#define RS274LETTER_ASSERT(x) \
    if (!(x)) { \
        std::cout << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << rs274letter::util::BacktraceToString(100, 2, " -- ") << std::endl; \
        assert(x); \
    }

#define RS274LETTER_ASSERT2(x, w) \
    if (!(x)) { \
        std::cout << "ASSERTION: " #x \
            << "\ninfo: " << w \
            << "\nbacktrace:\n" \
            << rs274letter::util::BacktraceToString(100, 2, " -- ") << std::endl; \
        assert(x); \
    }

