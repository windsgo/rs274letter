#include "Tokenizer.h"

#include "Exception.h" // Define Exception

#include <regex> // c++ regex library
#include <optional> // optional value support
#include <vector>

#include <iostream> // debug output

namespace rs274letter
{
    
Tokenizer::Tokenizer(std::string string) 
    : _string(std::move(string))
    , _cursor(0)
    , _cur_line(0) 
{

}

Tokenizer::~Tokenizer() {

}

void Tokenizer::init(std::string string) {
    this->_string = std::move(string);
    this->_cursor = 0;
    this->_cur_line = 0;
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
    // NULL Tokens
    XX(R"(^\s+)", "NULL"),
    XX(R"(^\(.*\))", "COMMENT()"),
    XX(R"(^;.*)", "COMMENT;"),
    XX(R"(^\n)", "RETURN"),

    XX(R"(^[gG])", "G"),
    XX(R"(^[mM])", "M"),
    XX(R"(^[eE])", "E"),
    // XX(R"(^[wW])", "W"),

    XX(R"(^[xXyYzZaAbBcC])", "COORDINATE"),

    XX(R"(^\d+\.\d*|^\d*\.\d+)", "DOUBLE"), // DOUBLE should be higher than INTEGER, support 01.00 / 01. / .01
    XX(R"(^\d+)", "INTEGER"),

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
                                              const std::string &str)
{
    std::smatch m;
    if (std::regex_search(str.begin(), str.end(), m, pattern)) {
        return m[0].str();
    } else {
        return std::nullopt;
    }
}

static bool _should_skip_token_type(const TokenType& type) {
    return type == "NULL" 
        || type == "RETURN"
        || type == "COMMENT;"
        || type == "COMMENT()";
}

Token Tokenizer::getNextToken() {
    if (!this->hasMoreTokens()) {
        return {};
    }

    const std::string& str = this->_string.substr(this->_cursor);

    for (auto&& [raw_pattern_str, pattern, token_type] : s_spec_vec) {
        auto&& token_value_opt = _match_regex(pattern, str);
        if (!token_value_opt) {
            continue; // current pattern match failed, continue match next pattern defined in the `s_spec_vec`
        }

        // matched, token_value_opt != std::nullopt

        auto&& token_value = token_value_opt.value();
        
        std::cout << "DEBUG: Match regex: " << "\033[33m" << raw_pattern_str  << "\033[0m"
            << ",\ttype: " << "\033[32m" << token_type << "\033[0m"
            << ",\tvalue: " << "[" << "\033[7m" << token_value << "\033[0m" << "]" 
            << std::endl;

        this->_cursor += token_value.size(); // set the cursor to the begin of the next possible token

        if (_should_skip_token_type(token_type)) {
            return this->getNextToken(); // skip "null" token, give the next none-"null" token, recursively
        }

        return Token { // json::object
            {"type", token_type},
            {"value", token_value}
        };
    }

    throw Exception("Unexpected token(char): \"" + str.substr(0, 1) + "\"");
}   

bool Tokenizer::hasMoreTokens() const
{
    return (this->_cursor < this->_string.size());
}

} // namespace rs274letter
