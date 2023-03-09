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
    R"( G1 x-30 Y-[2.9+#2] Z+#<var> A123
        ; 1+1 ; not valid
        #1=[#2-2]
        #2=[2+#3*[1.2+#<aa>]]
        ##2=[#[##3*2]+#<_a>*#7]
        #<_abc_> = -1
        #2 = +##2

        o1 if [1]
            #1 = 2
        o1 endif
    )";

    auto t = std::make_unique<Tokenizer>(code.cbegin(), code.cend());


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