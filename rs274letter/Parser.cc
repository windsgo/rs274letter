// Parser.cc
#include "Parser.h"

#include "Exception.h"
#include "util.h"
#include "macro.h"

#include <iostream>

namespace rs274letter
{

// public static method for parsing 
AstObject Parser::parse(const std::string& string) {
    return Parser(string.cbegin(), string.cend()).parse();
}

// construtor
Parser::Parser(const std::string::const_iterator& cbegin, const std::string::const_iterator& cend)
    : _tokenizer(std::make_unique<Tokenizer>(cbegin, cend)) {
    // get the first token as lookahead
    this->_lookahead = this->_tokenizer->getNextToken();
}

// private member method
AstObject Parser::parse()
{
    return this->program();
}

AstObject Parser::program()
{
    return AstObject{
        {"type", "program"},
        {"body", this->statementList()}
    };
}

AstArray Parser::statementList(const std::optional<TokenType>& stop_lookahead_tokentype /*= std::nullopt*/)
{
    AstArray statement_list;

    statement_list.emplace_back(this->statement());
    while(!this->_lookahead.empty() && Tokenizer::GetTokenType(this->_lookahead) != stop_lookahead_tokentype.value_or("__no_reach__")) {
        // stop_lookahead_tokentype 是指结束查找语句的标识，如块语句从'{'查找到下一个'}'为止
        statement_list.emplace_back(this->statement());
    }

    return statement_list;
}

AstObject Parser::statement()
{
    auto&& lookahead_type = Tokenizer::GetTokenType(this->_lookahead);
    if (lookahead_type == "RTN") {
        return this->emptyStatement();
    } else if (lookahead_type == "LETTER") {
        return this->commandStatement();
    } else { // TODO
        return this->expressionStatement();
    }
}

AstObject Parser::emptyStatement()
{   
    this->eat("RTN");

    return  AstObject{
        {"type", "emptyStatement"}
    };
}

AstObject Parser::commandStatement()
{
    auto&& command_number_group_list = this->commandNumberGroupList();
    this->eat("RTN");

    return AstObject{
        {"type", "commandStatement"},
        {"body", command_number_group_list} // body is an array
    };
}

AstObject Parser::expressionStatement()
{
    auto&& expression = this->expression();
    this->eat("RTN");

    return AstObject{
        {"type", "expressionStatement"},
        {"body", expression}
    };
}

AstArray Parser::commandNumberGroupList()
{
    AstArray command_number_group_list;

    command_number_group_list.emplace_back(this->commandNumberGroup());
    while(Tokenizer::GetTokenType(this->_lookahead) != "RTN") {
        command_number_group_list.emplace_back(this->commandNumberGroup());
    }

    return command_number_group_list;
}

AstObject Parser::commandNumberGroup()
{
    // eat a letter
    auto&& letter = this->eat("LETTER");

    // transform letter to lower case
    std::transform(letter.begin(), letter.end(), letter.begin(), ::tolower);
    
    // eat the following number to construct a command-number group
    auto&& number = this->expression();

    return AstObject{
        {"type", "commandNumberGroup"},
        {"letter", letter},
        {"number", number}
    };
}

AstObject Parser::expression()
{
    return this->assignmentExpression();
}

AstObject Parser::assignmentExpression()
{
    auto&& left = this->additiveExpression();

    if (!this->IsAssignmentOperator(this->_lookahead)) {
        // if next token is not an assign operator
        // then this `assignmentExpression` is only an `additiveExpression`
        // May need to throw here, because this makes no sense, but now debug. // TODO
        return left;
    }

    // this `assignmentExpression` is EXACTLY an assignmentExpression
    return AstObject{
        {"type", "assignmentExpression"},
        {"operator", this->assignmentOperator()},
        {"left", this->IsValidAssignmentTarget(left)}, // The additiveExpression may not be a valid `leftHandSideExpresion`
        {"right", this->assignmentExpression()}
    };
}

TokenValue Parser::assignmentOperator()
{
    return this->eat("ASSIGN_OPERATOR");
}

AstObject Parser::additiveExpression()
{   
    AstObject left;
    if (Tokenizer::GetTokenType(this->_lookahead) == "ADDITIVE_OPERATOR") {
        // the lookahead is "+" or "-"
        // which means this additive expression omitted the first number 0
        // such as "-5" is parsed as an additive expression as (0 - 5)
        left = AstObject{
            {"type", "integerNumericLiteral"},
            {"value", 0}
        };
    } else {
        // an `additiveExpression` may just be a `multiplicativeExpression`
        left = this->multiplicativeExpression();
    }
    // or have an ADDITIVE_OPERATOR with right-hand-side-multiplicativeExpression
    // while loop to expand nested additive expression
    // e.g. 1 + 2 + 3 would be -> left:1, op:+, right:2 -> left:(1 + 2), op:+, right:3, to make this expansion
    // e.g. 1 + 2 * 3 would be -> left:1, op:+, right:(2 * 3), to make this expansion
    // e.g. 1 * 4 + 2 + 3 would be -> left:(1 * 4), op:+, right:2 -> left:(1 * 4) + 2, op:+, right:3 , to make this expansion
    while (Tokenizer::GetTokenType(this->_lookahead) == "ADDITIVE_OPERATOR") {
        auto&& op = this->eat("ADDITIVE_OPERATOR"); // eat the operator
        auto&& right = this->multiplicativeExpression(); // get the right hand side expression

        // make the original left, be the "left-hand-side" of new left, 
        // since the original left found a right-hand-side expression
        left = AstObject{
            {"type", "binaryExpression"},
            {"operator", op},
            {"left", left},
            {"right", right}
        };
    }

    return left;
}

AstObject Parser::multiplicativeExpression()
{
    // an `multiplicativeExpression` may just be a `primaryExpression`
    auto&& left = this->primaryExpression();

    // or have an MULTIPLICATIVE_OPERATOR with right-hand-side-primaryExpression
    // while loop to expand nested multiplicativeExpression
    while (Tokenizer::GetTokenType(this->_lookahead) == "MULTIPLICATIVE_OPERATOR") {
        auto&& op = this->eat("MULTIPLICATIVE_OPERATOR"); // eat the operator
        auto&& right = this->primaryExpression(); // get the right hand side expression

        // make the original left, be the "left-hand-side" of new left, 
        // since the original left found a right-hand-side expression
        left = AstObject{
            {"type", "binaryExpression"},
            {"operator", op},
            {"left", left},
            {"right", right}
        };
    }

    return left;
}

AstObject Parser::primaryExpression()
{
    auto&& type = Tokenizer::GetTokenType(this->_lookahead);
    
    if( type == "INTEGER" || type == "DOUBLE" ) {
        return this->numericLiteral();
    } else if (type == "[") {
        return this->parenthesizedExpression();
    } else {
        return this->leftHandSideExpression();
    }
}

AstObject Parser::parenthesizedExpression()
{
    this->eat("[");

    auto&& expression = this->expression(); 
    RS274LETTER_ASSERT(!expression.empty()); // inside the [ ] should be an expression which should be empty
         
    this->eat("]");

    return expression;
}

AstObject Parser::leftHandSideExpression()
{
    return this->variable();
}

AstObject Parser::variable()
{
    this->eat("#");

    auto&& type = Tokenizer::GetTokenType(this->_lookahead);

    if (type == "INTEGER") {
        return AstObject{
            {"type", "numberIndexVariable"},
            {"index", std::stoi(this->eat("INTEGER"))}
        };       
    } else if (type == "VAR_NAME") {
        return AstObject{
            {"type", "nameIndexVariable"},
            {"index", this->eat("VAR_NAME")}
        };       
    } else {
        throw Exception("Unexpected variable index type: " + type);
    }
}

// AstObject Parser::literal()
// {
//     return this->numericLiteral();
// }

AstObject Parser::numericLiteral()
{
    if (Tokenizer::GetTokenType(this->_lookahead) == "INTEGER") {
        return this->integerNumericLiteral();
    } else if (Tokenizer::GetTokenType(this->_lookahead) == "DOUBLE") {
        return this->doubleNumericLiteral();
    } else {
        throw Exception("Unknown numeric literal.");
    }
}

AstObject Parser::doubleNumericLiteral()
{
    auto&& token_value = this->eat("DOUBLE");

    return AstObject{
        {"type", "doubleNumericLiteral"},
        {"value", std::stod(token_value)}
    };
}

AstObject Parser::integerNumericLiteral()
{
    auto&& token_value = this->eat("INTEGER");

    return AstObject{
        {"type", "integerNumericLiteral"},
        {"value", std::stoi(token_value)}
    };
}

TokenValue Parser::eat(const TokenType &token_type)
{
    auto&& token = this->_lookahead;

    // std::cout << "  [DEBUG]: " << "eat:" << token.to_string() << std::endl;

    // lookahead is empty
    if (token.empty()) {
        throw Exception("Unexpected end of input, expected: type = " + token_type);
    }

    // lookahead does not have the key: type
    auto&& token_type_opt = token.find("type");
    if (!token_type_opt || !token_type_opt.value().is_string()) {
        throw Exception("Internal Error, token has no key 'type', or type is not string");
    }

    // lookahead does not have the key: value
    auto&& token_value_opt = token.find("value");
    if (!token_value_opt || !token_value_opt.value().is_string()) {
        throw Exception("Internal Error, token has no key 'value', or value is not string");
    }

    // lookahead's type != the given eating token_type
    if (token_type_opt.value() != token_type) {
        std::cout << rs274letter::util::BacktraceToString(100) << std::endl;
        throw Exception("Unexpected token:\n"
              "type: " + token_type_opt.value().as_string()
            + ", value: " + token_value_opt.value().as_string()
            + "\nexpected:\n"
              "type: " + token_type);
    }

    // lookahead the next token
    this->_lookahead = _tokenizer->getNextToken();

    // return the token's value (which is a std::string)
    return token_value_opt.value().as_string(); // Note: use as_string, not to_string
}

bool Parser::IsAssignmentOperator(const Token &token)
{
    return Tokenizer::GetTokenType(token) == "ASSIGN_OPERATOR";
}


const Token& Parser::IsValidAssignmentTarget(const Token &token)
{
    return token; // TODO
}

} // namespace rs274letter
