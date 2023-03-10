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
     * Side Effects: _cur and _cur_line may change.
     */
    Token getNextToken();

    /**
     * lookAheadTokens()
     * look ahead the next few tokens
     * This is const method, will not change _cur and _cur_line
    */
    // std::vector<Token> lookAheadTokens(size_t token_num) const;

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
    inline std::size_t getCurColumn() const { return _cur_col; }

    inline std::string getLineColumnShowString() const {
        std::stringstream ss;
        ss << "line: " << _cur_line << ", column: " << _cur_col;
        return ss.str();
    } 

    // inline void setCurLine(std::size_t line) { _cur_line = line; } 

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

    /**
     * GetTokenTypeValueShowString()
     * Return a string to show the type and value of a token
    */
    static std::string GetTokenTypeValueShowString(const Token& token);

    // inline std::string::const_iterator getCurrentIterator() const { return _cur; }

    // inline void setCurrentIterator(const std::string::const_iterator& cit) { _cur = cit; }

private:
    /**
     * _lookAheadOneToken()
     * match the next token, return a token anyway, even if it's null or comments
    */
    // Token _lookAheadOneTokenNoRecurive(std::string::const_iterator start) const;

private:
    std::string::const_iterator _cur;
    std::string::const_iterator _end;
    std::size_t _cur_line{1};
    std::size_t _cur_col{1};
};

} // namespace rs274letter

