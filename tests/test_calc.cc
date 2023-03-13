#include <iostream>

#include "rs274letter/Serializer.h"

static void test_assign(rs274letter::Serializer& s,
            const rs274letter::AstObject& program, 
            int statement_index) 
{
    auto program_statement_list = program.at("body").as_array(); // make a copy

    auto&& assign_expression = program_statement_list[statement_index]["expression"].as_object();
    auto&& var = assign_expression["left"].as_object();

    std::cout << s.getValue(assign_expression) << std::endl;
}

int main(int argc, char** argv) {

    std::string code = R"(
        #5 = 123
        #<_abc_> = 12.34
    )";

    auto&& program = rs274letter::Parser::parse(code);

    std::cout << program.format(true) << std::endl;
    
    try {
        rs274letter::Serializer s(program);   
        test_assign(s, program, 0);
        test_assign(s, program, 1);
        test_assign(s, program, 1);
        std::cout << s.getVariableValue(5) << std::endl;
        std::cout << s.getVariableValue("_abc_") << std::endl;
    } catch (rs274letter::Exception& e) {
        std::cout << "rs274:\n" << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}