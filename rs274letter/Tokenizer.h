// Tokenizer.h
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
    /**
     * Constructor, disable default construct
     * We DO NOT store string in this class, to avoid the copy
     * We use iterator to visit each char in the string
     * ** The original string should always be valid until you don't use this instance anymore
     * (Tokenizer is usually only used in Parser, which only provides static method for parsing
     * which means the string passed to the Parser will always be valid with this Tokenizer
     * )
    */
    Tokenizer(const std::string::const_iterator& beg, const std::string::const_iterator& end) noexcept;

    ~Tokenizer() noexcept = default;

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

    /**
     * getCurLine()
     * Return current line number.
     * line number starts at 1.
     * line number will increase if a '\n' is read
    */
    inline std::size_t getCurLine() const { return _cur_line; }

    /**
     * IsTokenType()
     * Return True if token's type equals to token_type
     * If token does not have `type` key, or the key's value is not a string, throw Exception
    */
    static bool IsTokenType(const Token& token, const TokenType& token_type);

    /**
     * GetTokenType()
     * Return the token's type as a string
    */
    static TokenType GetTokenType(const Token& token);

private:
    std::string::const_iterator _cur;
    std::string::const_iterator _end;
    std::size_t _cur_line{0};
};

} // namespace rs274letter

