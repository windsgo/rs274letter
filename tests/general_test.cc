#include <iostream>
#include <fstream>

#include "rs274letter/Tokenizer.h"
#include "rs274letter/Exception.h"
#include "rs274letter/util.h"
#include "rs274letter/macro.h"
#include "rs274letter/Parser.h"

using namespace rs274letter;

int main(int argc, char** argv) {

    std::string code = 
    R"( o1 if[2]
o222 elseif [#2 = 1]
o<aa> endif
    )";

    if (argc > 1) {
        auto file_name = argv[1];

        std::ifstream ifs(file_name, std::ios_base::in);
        
        if (ifs.is_open()) {
            std::stringstream ss;
            ss << ifs.rdbuf();
            code = ss.str();
        }

        ifs.close();
    }

    auto t = std::make_unique<Tokenizer>(code.cbegin(), code.cend());

    // auto vec = t->lookAheadTokens(1000);
    // for (auto&& i : vec) {
    //     std::cout << i.to_string() << std::endl;
    // }

    try {
        while(t->hasMoreTokens()) {
            auto&& token = t->getNextToken();
            if (!token.empty())
                std::cout << "line:" << t->getCurLine() << token.to_string() << std::endl;
        }


        auto v = Parser::parse(code);
        std::cout << v.format(true, "  ", 0) << std::endl;

        std::ofstream ofs("output.json");
        ofs << v.format(true, "  ", 0) << std::endl;

        // RS274LETTER_ASSERT2(1==0, "hi");
        // std::cout << rs274letter::util::BacktraceToString(100) << std::endl;
    } catch (Exception& e) {
        std::cout << e.what() << std::endl;
    }

    // std::cout << std::stod("+.05") << std::endl;

    return 0;
}