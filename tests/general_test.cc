#include <iostream>

#include "rs274letter/Tokenizer.h"
#include "rs274letter/Exception.h"

using namespace rs274letter;

int main(int argc, char** argv) {

    Tokenizer t{R"(
        g01X21.2y300.Z.05A30(12 34); ab
    )"};

    try {
        while(t.hasMoreTokens()) {
            auto&& token = t.getNextToken();
            if (!token.empty())
                std::cout << token.to_string() << std::endl;
        }
    } catch (Exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}