// Tokenizer.cc
#include "Tokenizer.h"

#include "Exception.h" // Define Exception

#include <regex> // c++ regex library
#include <optional> // optional value support
#include <vector>

#include <iostream> // debug output

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
    XX(R"(^\s+)", "NULL"),          // spaces
    XX(R"(^\(.*\))", "CMT()"),  // comment like : ( xxx )
    XX(R"(^;.*)", "CMT;"),      // comment like : ( ; abc )
    

    // Control Flow
    XX(R"(^if\b|^IF\b)", "IF"),  // should be above LETTER

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

    XX(R"(^[a-zA-Z])", "LETTER"),


    // Variable Operations
    XX(R"(^#)", "#"),               // #, variable pre Identifier
    XX(R"(^=)", "ASSIGN_OPERATOR"),          // =, assignment operator =
    XX(R"(^<\w+>)", "VAR_NAME"),    // variable name <_var_name_>

    // Numerical Literals
    // XX(R"(^[+-]?\d+\.\d*|^[+-]?\d*\.\d+)", "DOUBLE"), // DOUBLE should be higher than INTEGER, support 01.00 / 01. / .01
    // XX(R"(^[+-]?\d+)", "INTEGER"),
    XX(R"(^\d+\.\d*|^\d*\.\d+)", "DOUBLE"), // DOUBLE should be higher than INTEGER, support 01.00 / 01. / .01
    XX(R"(^\d+)", "INTEGER"),

    // Binary Expression
    XX(R"(^[\+\-])", "ADDITIVE_OPERATOR"), // "+" or "-"
    XX(R"(^[\*\/])", "MULTIPLICATIVE_OPERATOR"), // "*" or "/"

    XX(R"(^\[)", "["),
    XX(R"(^\])", "]"),


    // String Literals: only used for call syntax: call "myfile.ngc" (not rs274 standard) // TODO
    XX(R"(^"\w*")", "STRING"),

    XX(R"(^\w+)", "STR")
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
        
        if (token_type != "NULL")
        std::cout << "\t[DEBUG]: " 
            // << "Match regex: " << "\033[33m" << raw_pattern_str  << "\033[0m"
            << "\ttype: " << "\033[32m" << token_type << "\033[0m"
            << ",\tvalue: " << "[" << "\033[7m" << (token_type == "RTN" ? "\\n" : token_value) << "\033[0m" << "]" 
            << std::endl;

        this->_cur += token_value.size(); // set the cursor to the begin of the next possible token

        if (token_type == "RTN") {
            ++_cur_line;
        }

        if (_should_skip_token_type(token_type)) {
            return this->getNextToken(); // skip "null" token, give the next none-"null" token, recursively
        }

        return Token { // json::object
            {"type", token_type},
            {"value", token_value}
        };
    }

    std::stringstream ss;
    ss << "Unexpected token(char): \"" << (*_cur) << "\"";
    throw Exception(ss.str());
}   

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

} // namespace rs274letter
