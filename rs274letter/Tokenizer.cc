// Tokenizer.cc
#include "Tokenizer.h"

#include "Exception.h" // Define Exception
#include "macro.h"

#include <regex> // c++ regex library
#include <optional> // optional value support
#include <vector>

#include <iostream> // debug output
// #define TOKEN_DEBUG_OUTPUT
#define COMMENT_STDOUT_OUTPUT

namespace rs274letter
{

Tokenizer::Tokenizer(const std::string::const_iterator& beg, const std::string::const_iterator& end) noexcept {
    this->_cur = beg;
    this->_end = end;
    this->_cur_line = 1;
}

/**
 * s_spec_vec
 *  A vector of the `regex` to `token_type` map,
 *  used to find the `regex` pattern in the code `_string`
 * 
 * tuple: <pattern_string(easier for regex debug), regex, TokenType>
 */
#define XX(ptn, type) std::make_tuple( ptn, std::regex{ptn}, type )

static const std::vector<std::tuple<std::string, std::regex, TokenType>> s_spec_vec = {
    // Return
    XX(R"(^\n)", "RTN"),         // return, designed to use for line number calculation

    // NULL Tokens
    XX(R"(^[ \t]+)", "NULL"),          // spaces and tabs, excluding \n
    XX(R"(^\(.*\))", "CMT()"),  // comment like : ( xxx )
    XX(R"(^;.*)", "CMT;"),      // comment like : ; abc 
    
    // Control Flow
    // should be above LETTER
    XX(R"(^\bcall\b|^\bCALL\b)", "call"), // o... call

    // if related keywords
    XX(R"(^\bif\b|^\bIF\b)", "if"),  // if 
    XX(R"(^\bendif\b|^\bENDIF\b)", "endif"),  // endif 
    XX(R"(^\belseif\b|^\bELSEIF\b)", "elseif"),  // elseif 
    XX(R"(^\belse\b|^\bELSE\b)", "else"),  // else 

    // sub related keywords
    XX(R"(^\bsub\b|^\bSUB\b)", "sub"),
    XX(R"(^\breturn\b|^\bRETURN\b)", "return"),
    XX(R"(^\bendsub\b|^\bENDSUB\b)", "endsub"),

    // while related keywords
    XX(R"(^\bwhile\b|^\bWHILE\b)", "while"),
    XX(R"(^\bendwhile\b|^\bENDWHILE\b)", "endwhile"),
    // XX(R"(^\bdo\b|^\bDO\b)", "do"),      // Won't support do-while structure
    XX(R"(^\bbreak\b|^\bBREAK\b)", "break"),
    XX(R"(^\bcontinue\b|^\bCONTINUE\b)", "continue"),

    // repeat related keywords
    XX(R"(^\brepeat\b|^\bREPEAT\b)", "repeat"),
    XX(R"(^\bendrepeat\b|^\bENDREPEAT\b)", "endrepeat"),
    

    XX(R"(^[oO](?![a-zA-Z_]))", "O"), // control command letter "o" / "O", above all letters

    /*
    // Commands
    XX(R"(^[gG])", "G"),            // G Code Identifier
    XX(R"(^[mM])", "M"),            // M Code Identifier
    XX(R"(^[eE])", "E"),            // E Code Identifier

    // Parameters
    XX(R"(^[x-zX-Za-cA-Cu-wU-W])", "COORDINATE"), // Coordinate Identifier (XYZABCUVW)
    // XX(R"(^[pP])", "P"),            // P Parameter Identifier
    // XX(R"(^[qQ])", "Q"),            // Q Parameter Identifier
    */

    // Only identifies LETTER-DOUBLE group (we use double for all)
    //   Normal CommandStatement will contains a vector of LETTER-DOUBLE pair
    //   In Tokenizer and Parser we only examine if this is Syntactically correct
    //   We will examine whether a normal CommandStatement is Environmentally correct
    // in the Executer when we examine the result which is provided by the Parser

    // use (?![a-zA-Z]) to specify that the next char after `LETTER` should not be
    // a letter or _ 
    XX(R"(^[a-zA-Z](?![a-zA-Z_]))", "LETTER"),

    // Variable Operations
    XX(R"(^#)", "#"),               // #, variable pre Identifier
    XX(R"(^=(?!=))", "ASSIGN_OPERATOR"),          // =, assignment operator =
    XX(R"(^<\w+>)", "VAR_NAME"),    // variable name <_var_name_>

    // Numerical Literals
    XX(R"(^\d+\.\d*|^\d*\.\d+)", "DOUBLE"), // DOUBLE should be higher than INTEGER, support 01.00 / 01. / .01
    XX(R"(^\d+)", "INTEGER"),

    // Relational Operations
    XX(R"(^[><]=?|^==|^!=|^[gl][te]|^[GL][TE]|^EQ|^eq|^NE|^ne)", "RELATIONAL_OPERATOR"),

    // Binary Expression
    XX(R"(^\*\*(?![\*\/]))", "POW_OPERATOR"), // "**",
    XX(R"(^\+(?!\+)|^\-(?!\-))", "ADDITIVE_OPERATOR"), // "+" or "-", donot allow "--" or "++", but allow "+-", "-+"
    XX(R"(^[\*\/](?![\*]))", "MULTIPLICATIVE_OPERATOR"), // "*" or "/"
    XX(R"(^and|^AND|^or|^OR|^xor|^XOR)", "LOGICAL_OPERATOR"), // logic: AND OR XOR

    XX(R"(^\[)", "["),
    XX(R"(^\])", "]"),


    // String Literals: only used for call syntax: call "myfile.ngc" (not rs274 standard) // TODO
    XX(R"(^"\w*")", "STRING"),

    XX(R"(^\w+)", "IDENTIFIER") // used for internal function
};

#undef XX

/**
 * _match_regex()
 *  Match the string `str` using the `pattern`,
 *  return std::nullopt if not matched,
 *  return a `TokenValue` (a substr of the `str`) if matched
 */
static std::optional<TokenValue> _match_regex(const std::regex &pattern,
                                              const std::string::const_iterator &beg,
                                              const std::string::const_iterator &end)
{
    std::smatch m;
    if (std::regex_search(beg, end, m, pattern)) {
        return m[0].str();
    } else {
        return std::nullopt;
    }
}

static bool _should_skip_token_type(const TokenType& type) {
    return type == "NULL" 
        // || type == "RTN" // RETURN should be used to seperate normal statement line
        || type == "CMT;"
        || type == "CMT()";
}

Token Tokenizer::getNextToken() {

    // auto&& next_token = _lookAheadOneTokenNoRecurive(this->_cur);

    // if (next_token.empty()) return {};

    // this->_cur += next_token.at("value").as_string().size();

    // auto&& type = Tokenizer::GetTokenType(next_token);

    // if (type == "RTN") {
    //     ++(this->_cur_line);
    // }

    // if (_should_skip_token_type(type)) {
    //     return this->getNextToken();
    // }

    // return next_token;

    if (!this->hasMoreTokens()) {
        return {};
    }

    for (auto&& [raw_pattern_str, pattern, token_type] : s_spec_vec) {
        auto&& token_value_opt = _match_regex(pattern, _cur, _end);
        if (!token_value_opt) {
            continue; // current pattern match failed, continue match next pattern defined in the `s_spec_vec`
        }

        // matched, token_value_opt != std::nullopt

        auto&& token_value = token_value_opt.value();
        
#ifdef TOKEN_DEBUG_OUTPUT
        if (token_type != "NULL")
            std::cout << "\t[DEBUG]: " 
                // << "Match regex: " << "\033[33m" << raw_pattern_str  << "\033[0m"
                << "\ttype: " << "\033[32m" << token_type << "\033[0m"
                << ",\tvalue: " << "[" << "\033[7m" << (token_type == "RTN" ? "\\n" : token_value) << "\033[0m" << "]" 
                << std::endl;
#endif // TOKEN_DEBUG_OUTPUT

        this->_cur += token_value.size(); // set the cursor to the begin of the next possible token
        this->_cur_col += token_value.size();

        if (token_type == "RTN") {
            ++(this->_cur_line);
            this->_cur_col = 1;
        }

#ifdef COMMENT_STDOUT_OUTPUT

        if (token_type == "CMT;" || token_type == "CMT()") {
            std::cout << this->_cur_line << "\t[COMMENT]: " << token_value << std::endl;
        }

#endif // COMMENT_STDOUT_OUTPUT

        if (_should_skip_token_type(token_type)) {
            return this->getNextToken(); // skip "null" token, give the next none-"null" token, recursively
        }

        return Token { // json::object
            {"type", token_type},
            {"value", token_value}
        };
    }

    std::stringstream ss;
    ss << "Unexpected token(char): \"" << (*this->_cur) << "\"\n"
       << this->getLineColumnShowString();
    throw Exception(ss.str());
}

// std::vector<Token> Tokenizer::lookAheadTokens(size_t token_num) const {
//     std::vector<Token> token_vec;
//     token_vec.reserve(token_num);

//     auto tmp_cursor = _cur;
//     std::size_t token_got_num = 0;

//     while(token_got_num < token_num && tmp_cursor != _end) {
//         auto&& next_token = _lookAheadOneTokenNoRecurive(tmp_cursor);
//         auto&& type = Tokenizer::GetTokenType(next_token);
//         tmp_cursor += next_token.at("value").as_string().size();
//         if (_should_skip_token_type(type)) {
//             continue;
//         }

//         token_vec.emplace_back(next_token);
//         ++token_got_num;
//     }
    
//     RS274LETTER_ASSERT(token_vec.size() == token_num | tmp_cursor == this->_end);
//     return token_vec;
// }

bool Tokenizer::hasMoreTokens() const
{
    return (this->_cur != this->_end);
}

bool Tokenizer::IsTokenType(const Token &token, const TokenType &token_type)
{
    auto&& t = token.find<TokenType>("type");
    if (!t) {
        throw Exception("Token does not have `type` key. Token: " + token.to_string());
    }

    return (t.value() == token_type);
}

TokenType Tokenizer::GetTokenType(const Token& token) {
    return token.at("type").as_string();
}

std::string Tokenizer::GetTokenTypeValueShowString(const Token &token)
{
    std::stringstream ss;
#define _XX(x) \
    if (token.empty()) { \
        ss << "_empty_"; \
    } else { \
        auto&& x = token.find(#x).value_or("_no_"#x"_"); \
        if (!x.is_string()) { \
            ss << "_none_str_"#x"_"; \
        } else { \
            ss << x.as_string(); \
        } \
    }

    ss << "type: ";
    _XX(type)
    ss << ", value: ";
    _XX(value)

#undef _XX
    
    return ss.str();
}

// Token Tokenizer::_lookAheadOneTokenNoRecurive(std::string::const_iterator start) const {
//     if (start == this->_end) {
//         return {};
//     }

//     for (auto&& [raw_pattern_str, pattern, token_type] : s_spec_vec) {
//         auto&& token_value_opt = _match_regex(pattern, start, this->_end);
//         if (!token_value_opt) {
//             continue; // current pattern match failed, continue match next pattern defined inthe `s_spec_vec`
//         }

//         // matched, token_value_opt != std::nullopt
//         auto&& token_value = token_value_opt.value();
        
//         if (token_type != "NULL")
//         std::cout << "\t[DEBUG]: " 
//             // << "Match regex: " << "\033[33m" << raw_pattern_str  << "\033[0m"
//             << "\ttype: " << "\033[32m" << token_type << "\033[0m"
//             << ",\tvalue: " << "[" << "\033[7m" << (token_type == "RTN" ? "\\n" : token_value)
//             << "\033[0m" << "]" 
//             << std::endl;

//         return Token { // json::object
//             {"type", token_type},
//             {"value", token_value}
//         };
//     }

//     std::stringstream ss;
//     ss << "Unexpected token(char): \"" << (*start) << "\"";
//     throw Exception(ss.str());
// }

} // namespace rs274letter
