#pragma once

#include "json.hpp"

#include <string>

namespace rs274letter
{

/**
 * Token: json::object
 *  {
 *      "type": TokenType("SOME_TOKEN_TYPE"),
 *      "value": TokenValue("some value")
 *  }
*/
using Token = json::object;

using TokenType = std::string; // type of a `Token`: "NULL", etc.
using TokenValue = std::string; // value of a `Token`: "123", "abc", etc.

class Tokenizer {
public:
    Tokenizer() = default;
    Tokenizer(std::string string);

    ~Tokenizer();

    /**
     * init()
     * Initialize the tokenizer with a code string.
     */
    void init(std::string string);

    /**
     * getNextToken()
     * Get the next token.
     * Side Effects: _cursor and _cur_line may change.
     */
    Token getNextToken();

    /**
     * hasMoreTokens()
     * Return true if this code has more tokens (based on _cursor).
     */
    bool hasMoreTokens() const;

private:
    std::string _string;
    std::size_t _cursor{0};
    std::size_t _cur_line{0};
};

} // namespace rs274letter

