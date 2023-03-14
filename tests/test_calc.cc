#include <iostream>

#include "rs274letter/Serializer.h"
#include "rs274letter/util.h"

// static void test_assign(rs274letter::Serializer& s,
//             const rs274letter::AstObject& program, 
//             int statement_index) 
// {
//     auto program_statement_list = program.at("body").as_array(); // make a copy

//     auto&& assign_expression = program_statement_list[statement_index]["expression"].as_object();
//     auto&& var = assign_expression["left"].as_object();

//     std::cout << s.getValue(assign_expression) << std::endl;
// }

int main(int argc, char** argv) {
    rs274letter::util::ElapsedTimer timer("main");
    std::string code = R"(
        #5 = [#6 = 123]
        #<_abc_> = [#5 GT 12.34]
        G55G90G01X#5Y[-#6]A-#<_abc_>B.65C121.95F100
        o1 if [#5 EQ123.0]
            #6 = [9**0.5]
        o1 else
            #6 = 66
        o1 endif
        #10 = atan[1]/[-1]
        #11 = abs[-1.23]
        #12 = acos[1/2]
        #13 = sin[asin[1/2]]
        #14 = cos[60]
        #15 = exp[2]
        #16 = fix[-1.6]
        #17 = fup[-1.6]
        #18 = round[-1.6]
        #19 = ln[exp[3.5]]
        #20 = sin[30]
        #21 = sqrt[3]
        #22 = tan[-60]
        #23 = exists[#<_abc_>]
        #24 = exists[#10]
        #25 = exists[#<_abc>]
        #26 = exists[#100]
        E[sin[30]]
    )";

    rs274letter::AstObject program;
    
    // std::cout << program.format(true) << std::endl;
    
    try {
        {
            rs274letter::util::ElapsedTimer timer("parse");
            program = rs274letter::Parser::parse(code);
        }
        rs274letter::Serializer s(std::move(program));   
        {
            rs274letter::util::ElapsedTimer timer("processProgram");
            s.processProgram();
        }
        // test_assign(s, program, 0);
        // test_assign(s, program, 1);
        // s.processStatement(program.at("body").as_array()[2].as_object());
        std::cout << "#5:" << s.getVariableValue(5) << std::endl;
        std::cout << "#6:" << s.getVariableValue(6) << std::endl;
        for (unsigned int i = 0; i < 100; ++i) {
            if (s.hasVariable(i))
                std::cout << "#" << i << " = " << s.getVariableValue(i) << std::endl;
        }

        std::cout << "#<_abc_>:" << s.getVariableValue("_abc_") << std::endl;

        for (const auto& i : s.getCommandList()) {
            std::cout << i << std::endl;
        }
        
    } 
    catch (rs274letter::Exception& e) {
        std::cout << "rs274:\n" << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    } 
    catch (int i) {}

    // double x = 0.00f;
    // if (x == 0)
    // std::cout << !!x << std::endl;

    return 0;
}