// Parser.cc
#include "Parser.h"

#include "Exception.h"
#include "util.h"
#include "macro.h"

#include <iostream>
#include <sstream>

namespace rs274letter
{

// class _BackupParserState {
//     friend class Parser;
// public:
//     _BackupParserState(Parser* parser) 
//         : _parser(parser)
//         , _it_bak(_parser->_tokenizer->getCurrentIterator())
//         , _line_bak(_parser->_tokenizer->getCurLine())
//         , _lookahead_bak(_parser->_lookahead)
//     {    
//         std::cout << "bak beg" << std::endl;
//     }

//     ~_BackupParserState() {
//         _parser->_tokenizer->setCurrentIterator(_it_bak);
//         _parser->_tokenizer->setCurLine(_line_bak);
//         _parser->_lookahead = std::move(_lookahead_bak);
//         std::cout << "bak end" << std::endl;
//     }

// private:
//     Parser* _parser;

//     std::string::const_iterator _it_bak;
//     std::size_t _line_bak;
//     Token _lookahead_bak;
// };

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

static bool _is_lookahead_stoptokentypes(const Token& lookahead, std::vector<TokenType> vec) {
    auto && next_type = Tokenizer::GetTokenType(lookahead);

    for (auto&& type : vec) {
        if (next_type == type) {
            return true;
        }
    }
    return false;
}

AstArray Parser::statementList(const std::optional<std::vector<TokenType>>& stop_lookahead_tokentypes_after_o /*= std::nullopt*/) {
    AstArray statement_list;

    while(!this->_lookahead.empty()) {
        if (Tokenizer::GetTokenType(this->_lookahead) != "O") {
            // Not a statement(line) start with 'O'
            auto&& statement = this->statement();

            // may return an emptyStatement which is an empty AstObject()
            if (!statement.empty()) {
                statement_list.emplace_back(statement);
            }

            continue; // next statement
        }

        // encounter a statement which starts with an O

        // get the o-word
        auto&& o_word = this->oCommand();

        if (stop_lookahead_tokentypes_after_o 
            && _is_lookahead_stoptokentypes(this->_lookahead, 
            stop_lookahead_tokentypes_after_o.value_or(std::vector<TokenType>()))) {
            // meet the tokentype specified in `stop_lookahead_tokentypes_after_o`

            // record the eaten o-word here
            _last_o_word = std::move(o_word);

            // and break, to stop generating statementList and return 
            break;
        } else {
            // do not have stop generating flag
            // or haven't met the stop generating flags specified
            
            // emplace back a oCommandStatement with the given pre-o-word(which is eaten)
            statement_list.emplace_back(this->oCommandStatement(o_word));
        }
    }

    return statement_list;
}

AstObject Parser::statement()
{
    auto&& lookahead_type = Tokenizer::GetTokenType(this->_lookahead);
    if (lookahead_type == "RTN") {
        // return this->emptyStatement();
        // we see emptyStatement as a real `empty`
        // don't record anything
        this->eat("RTN");
        return {};
    } else if (lookahead_type == "LETTER") {
        return this->commandStatement();
    } else if (lookahead_type == "O") {
        return this->oCommandStatement();
    } 
    else { // TODO
        return this->expressionStatement(); 
    }
}

// AstObject Parser::emptyStatement()
// {   
//     this->eat("RTN");

//     return  AstObject{
//         {"type", "emptyStatement"}
//     };
// }

AstObject Parser::commandStatement()
{
    auto&& command_number_group_list = this->commandNumberGroupList();
    
    if (!this->_lookahead.empty()) this->eat("RTN");

    return AstObject{
        {"type", "commandStatement"},
        {"commands", command_number_group_list} // body is an array
    };
}

AstObject Parser::expressionStatement()
{
#ifdef NO_SINGLE_NON_ASSIGN_EXPRESSION
    auto&& expression = this->expression(true); // must be assignment
#else // NOT NO_SINGLE_NON_ASSIGN_EXPRESSION
    auto&& expression = this->expression(); // must be assignment
#endif // NO_SINGLE_NON_ASSIGN_EXPRESSION
    
    if (!this->_lookahead.empty()) this->eat("RTN");

    return AstObject{
        {"type", "expressionStatement"},
        {"expression", expression}
    };
}

AstObject Parser::oCommandStatement(const std::optional<AstObject> pre_o_word/* = std::nullopt*/)
{
    // given the pre_o_word or not ?
    auto&& o_command_start = pre_o_word ? pre_o_word.value() : this->oCommand();
    RS274LETTER_ASSERT(
        o_command_start.at("type").as_string() == "nameIndexOCommand" || 
        o_command_start.at("type").as_string() == "numberIndexOCommand");

    auto&& next_type_after_o = Tokenizer::GetTokenType(this->_lookahead);

    if (next_type_after_o == "call") {
        return this->oCallStatement(o_command_start);
    } else if (next_type_after_o == "if") {
        return this->oIfStatement(o_command_start);
    } else if (next_type_after_o == "sub") {
        return this->oSubStatement(o_command_start);
    } else if (next_type_after_o == "return") {
        return this->oReturnStatement(o_command_start);
    } else if (next_type_after_o == "while") {
        return this->oWhileStatement(o_command_start);
    } else if (next_type_after_o == "continue") {
        return this->oContinueStatement(o_command_start);
    } else if (next_type_after_o == "break") {
        return this->oBreakStatement(o_command_start);
    } else if (next_type_after_o == "repeat") {
        return this->oRepeatStatement(o_command_start);
    } else {
        std::cout << util::BacktraceToString(100) << std::endl;
        throw SyntaxError("Unexpected token type after oCommand: " + next_type_after_o);
    }
}

AstObject Parser::oCallStatement(const AstObject &o_command_start)
{
    this->eat("call");
    auto call_o_command = o_command_start;

    auto&& param_list = this->oCallParamList();

    if (!this->_lookahead.empty()) this->eat("RTN");
    
    return AstObject{
        {"type", "oCallStatement"},
        {"callOCommand", std::move(call_o_command)},
        {"paramList", param_list}
    };
}

AstObject Parser::oReturnStatement(const AstObject &o_command_start)
{
    if (this->_parsing_o_sub == false) {
        std::stringstream ss;
        ss << "o-return statement should be in a sub-statement\n"
           << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
    }

    this->eat("return");
    auto return_o_command = o_command_start;

    AstObject return_expression;
    if (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) == "[") {
        return_expression = this->parenthesizedExpression();
    }
    
    return AstObject{
        {"type", "oReturnStatement"},
        {"returnOCommand", std::move(return_o_command)},
        {"returnRtnExpr", std::move(return_expression)}
    };
}

AstArray Parser::oCallParamList()
{
    AstArray param_list;

    while (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) != "RTN") {
        param_list.emplace_back(this->parenthesizedExpression());
    }

    return param_list;
}

AstObject Parser::oIfStatement(const AstObject &o_command_start, bool should_eat_if/* = true*/)
{
    // should_eat_if is false when want to get a sub `elseif` statement
    if (should_eat_if) this->eat("if");

    // The `const reference` to o_command_start may refer to
    // `this->_last_o_word` which may change during recursively
    // calling `this->statementList()`. 
    // So I make a copy here, and finally use std::move() to construct
    // it into the AstObject's { "oIfCommand", std::move(if_o_command) }
    // For example, if we don't copy here, when this oIfStatement is called
    // by an oIfStatement() function itself, that is, in an `elseif`,
    // we get a `o_command_start` which is bound to this->_last_o_word,
    // when we skip out and eat an `endif`, this->_last_o_word will change
    // to the o-word in front of `endif`, this time, `o_command_start` also
    // changes !! 
    auto if_o_command = o_command_start;

    auto&& test = this->parenthesizedExpression();
    this->eat("RTN");

    // `o_word_list` is used to collect the o-words written for this whole 
    // `oIfStatement`, we will examine whether these o-words are the same 
    // value as each other in the future, to meet rs274 standard
    AstArray o_word_list; 
    // o_word_list.emplace_back(o_command_start);

    // tell the statementList
    // when encounter an oStatement and the next is else, elseif or endif
    // stop generating list, with an pre-o-word eaten now,
    // so we need to collect this eaten o-word later from this->_last_o_word
    auto&& consequent = this->statementList({{"else", "elseif", "endif"}});

    /**
     * About how to deal with `elseif`:
     * 
     * o1 if [#1]
     * o1 elseif [#2]
     * o1 elseif [#3]
     * o1 else
     * o1 endif
     * 
     * I expand the `elseif` in the above oIfStatement 
     * as a special `alternate`, which is also an oIfStatement
     * ==>
     * 
     * o1 if [#1]
     * o1 else
     *      o1 if [#2]
     *          o1 if [#3]
     *          o1 else
     *          o1 endif
     */

    // elseif
    if (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) == "elseif") {
        // if next is elseif, eat an `elseif`
        this->eat("elseif");
        
        // _last_o_word here is the o-word in front of this `elseif`
        o_word_list.emplace_back(this->_last_o_word);
 
        // without eating an if, get an ifStatement recursively
        // pass the false, means not to eat if at the start of the function
        auto&& sub_o_if_statement = this->oIfStatement(this->_last_o_word, false);

        // directly returns an ifStatement here,
        // because there will only be an `endif` in the whole if-block,
        // and we will deal with the maybe-incoming `else` in the sub_o_if_statement,
        // so we don't have to eat another `endif` or `else` after 
        // getting this sub_o_if_statement .
        return AstObject{
            {"type", "oIfStatement"},
            {"ifOCommand", std::move(if_o_command)},
            {"test", test},
            {"consequent", consequent},
            {"alternate", sub_o_if_statement},
            {"otherWordsList", std::move(o_word_list)}
        };
    }

    // else
    AstArray alternate; // alternate is the `else` 's statementList
    if (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) == "else") {
        // if next is else, eat an `else`
        this->eat("else");

        // _last_o_word here is the o-word in front of this `else`
        o_word_list.emplace_back(this->_last_o_word);
        alternate = this->statementList({{"endif"}});
    }

    // _last_o_word here is the o-word in front of this `endif`
    o_word_list.emplace_back(this->_last_o_word);
    this->eat("endif");
    
    if (!this->_lookahead.empty()) this->eat("RTN");

    return AstObject{
        {"type", "oIfStatement"},
        {"ifOCommand", std::move(if_o_command)},
        {"test", test},
        {"consequent", consequent},
        {"alternate", std::move(alternate)},
        {"otherWordsList", std::move(o_word_list)}
    };
}

AstObject Parser::oSubStatement(const AstObject &o_command_start)
{   
    RS274LETTER_ASSERT(this->_parsing_o_sub == false);
    if (this->_parsing_o_sub == true) {
        std::stringstream ss;
        ss << "Internal Error, _parsing_o_sub should be false"
           << _last_o_word.to_string() << "\n"
           << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
    }
    this->_parsing_o_sub = true; // marked as parsing sub start

    this->eat("sub");
    auto sub_o_command = o_command_start;

    auto&& body = this->statementList({{"sub", "endsub"}});

    // Do not allow nested o-sub !
    if (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) == "sub") {
        std::stringstream ss;
        ss << "Nested sub statement definition is not allowed:"
           << _last_o_word.to_string() << "\n"
           << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
    }

    this->eat("endsub");

    AstObject return_expression;
    if (!this->_lookahead.empty() 
        && Tokenizer::GetTokenType(this->_lookahead) == "[") {
        return_expression = this->parenthesizedExpression();
    }

    if (!this->_lookahead.empty()) this->eat("RTN");

    this->_parsing_o_sub = false; // marked as parsing sub end

    return AstObject{
        {"type", "oSubStatement"},
        {"subOCommand", std::move(sub_o_command)},
        {"endsubOCommand", this->_last_o_word},
        {"body", body},
        {"endsubRtnExpr", std::move(return_expression)}
    };
}

AstObject Parser::oWhileStatement(const AstObject &o_command_start)
{
    ++this->_parsing_o_while_layers;
    auto while_o_command = o_command_start;

    this->eat("while");

    // condition(test)
    auto&& test = this->parenthesizedExpression();
    this->eat("RTN");

    // body
    auto&& body = this->statementList({{"endwhile"}});

    // endwhile
    auto endwhile_o_command = this->_last_o_word;
    this->eat("endwhile");
    if (!this->_lookahead.empty()) this->eat("RTN");

    --this->_parsing_o_while_layers;
    return AstObject{
        {"type", "oWhileStatement"},
        {"whileOCommand", std::move(while_o_command)},
        {"endwhileOCommand", std::move(endwhile_o_command)},
        {"test", test},
        {"body", body},
        {"nestedLayer", this->_parsing_o_while_layers + 1} // layers, used for debug
    };
}

AstObject Parser::oContinueStatement(const AstObject &o_command_start)
{
    if (this->_parsing_o_while_layers < 1) {
        std::stringstream ss;
        ss << "o-continue statement should be in a while-statement\n"
           << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
    }

    auto continue_o_command = this->_last_o_word;
    this->eat("continue");
    
    return AstObject{
        {"type", "oContinueStatement"},
        {"continueOCommand", std::move(continue_o_command)},
        {"nestedLayer", this->_parsing_o_while_layers}
    };
}

AstObject Parser::oBreakStatement(const AstObject & o_command_start)
{
    if (this->_parsing_o_while_layers < 1) {
        std::stringstream ss;
        ss << "o-break statement should be in a while-statement\n"
           << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
    }

    auto break_o_command = this->_last_o_word;
    this->eat("break");
    
    return AstObject{
        {"type", "oBreakStatement"},
        {"breakOCommand", std::move(break_o_command)},
        {"nestedLayer", this->_parsing_o_while_layers}
    };
}

AstObject Parser::oRepeatStatement(const AstObject &o_command_start)
{
    auto repeat_o_command = o_command_start;

    this->eat("repeat");

    // repeat times
    auto&& times = this->parenthesizedExpression();
    this->eat("RTN");

    // body
    auto&& body = this->statementList({{"endrepeat"}});

    // endrepeat
    auto endrepeat_o_command = this->_last_o_word;
    this->eat("endrepeat");
    if (!this->_lookahead.empty()) this->eat("RTN");

    return AstObject{
        {"type", "oRepeatStatement"},
        {"repeatOCommand", std::move(repeat_o_command)},
        {"endrepeatOCommand", std::move(endrepeat_o_command)},
        {"times", times},
        {"body", body}
    };
}

AstArray Parser::commandNumberGroupList()
{
    AstArray command_number_group_list;

    command_number_group_list.emplace_back(this->commandNumberGroup());
    while(!this->_lookahead.empty() && Tokenizer::GetTokenType(this->_lookahead) != "RTN") {
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
    
    // number or sth (which can be calced as a number)
    auto&& number = this->primaryExpression();

    return AstObject{
        {"type", "commandNumberGroup"},
        {"letter", letter},
        {"number", std::move(number)}
    };
}

AstObject Parser::oCommand()
{
    this->eat("O");
    auto&& type = Tokenizer::GetTokenType(this->_lookahead);

    if (type == "VAR_NAME") {
        // #<_var_name_>
        return AstObject{
            {"type", "nameIndexOCommand"},
            {"index", this->nameIndex()}
        };
    } else {
        return AstObject{
            {"type", "numberIndexOCommand"},
            {"index", this->numberIndex()}
        };
    }
}

AstObject Parser::expression(bool must_be_assignment/* = false*/)
{
    return this->assignmentExpression(must_be_assignment);
}

AstObject Parser::assignmentExpression(bool must_be_assignment/* = false*/)
{
    // auto&& left = this->additiveExpression();
    auto&& left = this->relationalExpression();

    if (!this->IsAssignmentOperator(this->_lookahead)) {
        // if must_be_assignment, throw SyntaxError here
        if (must_be_assignment) {
            std::cout << util::BacktraceToString(100) << std::endl;
            std::stringstream ss;
            ss << this->_tokenizer->getLineColumnShowString()
               << "\nMust be assignment expression here, but no assignment operator,"
               << " next token is:\n"
               << Tokenizer::GetTokenTypeValueShowString(this->_lookahead);
            throw SyntaxError(ss.str());
        }
        
        // if next token is not an assign operator
        // then this `assignmentExpression` is only an `additiveExpression`
        return left;
    } 

    // this `assignmentExpression` is EXACTLY an assignmentExpression
    return AstObject{
        {"type", "assignmentExpression"},
        {"operator", this->assignmentOperator()},
        {"left", this->IsValidAssignmentTarget(left)}, // The additiveExpression may not be a valid `leftHandSideExpresion`
#ifdef MUST_PRIMARY_RIGHT_HANDSIDE_OF_ASSIGN
        {"right", this->primaryExpression()} // need to be a primary expression in rs274
#else // NOT MUST_PRIMARY_RIGHT_HANDSIDE_OF_ASSIGN
        {"right", this->assignmentExpression()} 
#endif // MUST_PRIMARY_RIGHT_HANDSIDE_OF_ASSIGN
    };
}

TokenValue Parser::assignmentOperator()
{
    return this->eat("ASSIGN_OPERATOR");
}

AstObject Parser::relationalExpression()
{
    auto&& left = this->additiveExpression();

    while (Tokenizer::GetTokenType(this->_lookahead) == "RELATIONAL_OPERATOR") {
        auto&& op = this->eat("RELATIONAL_OPERATOR"); // eat the operator
        auto&& right = this->additiveExpression(); // get the right hand side expression

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

AstObject Parser::additiveExpression()
{   
    // an `additiveExpression` may just be a `multiplicativeExpression`
    auto&& left = this->multiplicativeExpression();

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

AstObject Parser::primaryExpression(bool can_have_forward_additive_op/* = true*/)
{
    auto&& type = Tokenizer::GetTokenType(this->_lookahead);
    
    if (type == "ADDITIVE_OPERATOR" && can_have_forward_additive_op) {
        // 一元运算符，正、负，识别为additive，包装为一个省略0的加减法表达式
        auto zero = AstObject{
            {"type", "integerNumericLiteral"},
            {"value", 0}
        };
        auto&& op = this->eat("ADDITIVE_OPERATOR");

        return AstObject{
            {"type", "binaryExpression"},
            {"operator", op},
            {"left", std::move(zero)},
            {"right", this->primaryExpression(false)}
        };
    } else if ( type == "INTEGER" || type == "DOUBLE" ) {
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

    if (type == "VAR_NAME") {
        // #<_var_name_>
        return AstObject{
            {"type", "nameIndexVariable"},
            {"index", this->nameIndex()}
        };
    } else {
        return AstObject{
            {"type", "numberIndexVariable"},
            {"index", this->numberIndex()}
        };
    }
}

std::string Parser::nameIndex()
{
    auto&& var_with_angle_brackets = this->eat("VAR_NAME");

    return var_with_angle_brackets.substr(1, var_with_angle_brackets.size() - 2);
}

AstValue Parser::numberIndex()
{   
#ifdef NAMEINDEX_JUST_PRIMARYEXPRESSION
    return this->primaryExpression();
#else // NOT NAMEINDEX_JUST_PRIMARYEXPRESSION
    auto&& type = Tokenizer::GetTokenType(this->_lookahead);

    if (type == "INTEGER") {
        return std::stoi(this->eat("INTEGER")); // literal won't be negative here
    } else if (type == "DOUBLE") {
        throw SyntaxError("Cannot have a Double Literal after #");
    } else if (type == "[") {
        return this->parenthesizedExpression();
    } else if (type == "#") {
        return this->variable();
    } else {
        throw SyntaxError("Unexpected variable index type: " + type);
    }
#endif // NAMEINDEX_JUST_PRIMARYEXPRESSION
}

AstObject Parser::numericLiteral()
{
    if (Tokenizer::GetTokenType(this->_lookahead) == "INTEGER") {
        return this->integerNumericLiteral();
    } else if (Tokenizer::GetTokenType(this->_lookahead) == "DOUBLE") {
        return this->doubleNumericLiteral();
    } else {
        throw SyntaxError("Unknown numeric literal.");
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
        throw SyntaxError("Unexpected end of input, expected: type = " + token_type);
    }

    // lookahead does not have the key: type
    auto&& token_type_opt = token.find("type");
    if (!token_type_opt || !token_type_opt.value().is_string()) {
        throw SyntaxError("Internal Error, token has no key 'type', or type is not string");
    }

    // lookahead does not have the key: value
    auto&& token_value_opt = token.find("value");
    if (!token_value_opt || !token_value_opt.value().is_string()) {
        throw SyntaxError("Internal Error, token has no key 'value', or value is not string");
    }

    // lookahead's type != the given eating token_type
    if (token_type_opt.value() != token_type) {
        std::cout << rs274letter::util::BacktraceToString(100) << std::endl;
        std::stringstream ss;
        ss << "Unexpected token:\n"
           << Tokenizer::GetTokenTypeValueShowString(token)
           << "\nexpected:\n" 
           << "type: " << token_type
           << "\n" << this->_tokenizer->getLineColumnShowString();
        throw SyntaxError(ss.str());
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


const AstValue& Parser::IsValidAssignmentTarget(const AstValue &ast_value)
{
    auto&& type = ast_value.at("type").as_string();
    if (type == "numberIndexVariable" || type == "nameIndexVariable") {
        return ast_value;
    } else {
        std::stringstream ss;
        ss << "Invalid assignment target on the lhs, target: " << ast_value.to_string();
        throw SyntaxError(ss.str());
    }
}

// bool Parser::isNextLineOEndif() noexcept
// {   
//     _BackupParserState _b(this);

//     try {
//         auto&& type = Tokenizer::GetTokenType(this->_lookahead);

//         if (type != "O") {
//             return false;
//         }

//         this->oCommand();

//         type = Tokenizer::GetTokenType(this->_lookahead);

//         if (type != "endif") {
//             return false;
//         }

//         this->eat("endif");
//         this->eat("RTN");
//     } catch (...) {
//         return false;
//     }
//     return true;
// }

} // namespace rs274letter
