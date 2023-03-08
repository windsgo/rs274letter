// Parser.h
#pragma once

#include <string>
#include <memory>

#include "json.hpp"

#include "Tokenizer.h"

namespace rs274letter
{

/**
 * AstObject
 * used to store each AST node, and construct the whole AST
*/
using AstValue = json::value;
using AstObject = json::object;
using AstArray = json::array;

class Parser {
public:
    ~Parser() noexcept = default;

    /**
     * parse()
     * parse the whole program with a code string.
    */
    static AstObject parse(const std::string& string);

private:
    Parser(const std::string::const_iterator& cbegin, const std::string::const_iterator& cend);

    AstObject parse();

    /**
     * A program may be:
     *  : statementList
    */
    AstObject program();
    
    /**
     * A statementList is an array of statement:
     *  : statementList statement -> statement ... statement statement
    */
    AstArray statementList(const std::optional<TokenType>& stop_lookahead_tokentype = std::nullopt);

    /**************************/
    /*****    Statement    ****/
    /**************************/

    /**
     * A statement may be a:
     *  : emptyStatement
     *  | commandStatement
     *  | expressionStatement
     *  ;
    */
    AstObject statement();


    /**
     * A emptyStatement may be:
     *  : "RTN"
     *  ;
    */
    AstObject emptyStatement();

    /**
     * A commandStatement may be:
     *  : commandNumberGroupList "RTN"
     *  ;
    */
    AstObject commandStatement();

    /**
     * An expressionStatement may be:
     *  : Expression "RTN"
     *  ;
    */
    AstObject expressionStatement();

    /**
     * A commandNumberGroupList is an array of commandNumberGroup:
     *  : commandNumberGroupList commandNumberGroup -> commandNumberGroup ... commandNumberGroup commandNumberGroup
     *  ;
    */
    AstArray commandNumberGroupList();

    /**
     * A command-number group consists of:
     *  : LETTER assignmentExpression
     *  ;
    */
    AstObject commandNumberGroup();


    /**************************/
    /*****   Expression    ****/
    /**************************/

    /**
     * Expression Priority: (low to high)
     *  assignmentExpression
     *  additiveExpression
     *  multiplicativeExpression
     *  primaryExpression (which could be literal / parenthesizedExpression / leftHandSideExpression )
     *  ;
    */

    /**
     * An expression may be:
     *  : assignmentExpression
     *  ;
     * Since `assignmentExpression` is the lowest priority expression
    */
    AstObject expression();

    /**
     * An assignmentExpression may be:
     *  : additiveExpression,
     *  | leftHandSideExpression assignmentOperator additiveExpression
     *  ;
     * Note: the expanded assignmentExpression must have a leftHandSideExpresion on its left,
     * However, we can only first parse an `additiveExpression`, then examine if it is an `leftHandSideExpresion`
    */
    AstObject assignmentExpression();

    /**
     * assignmentOperator is:
     *  : ASSIGN_OPERATOR
     *  ;
     * This returns the operator as a string, currently must be "=", or throw exception.
    */
    TokenValue assignmentOperator();
    
    /**
     * An additiveExpression maybe:
     *  : multiplicativeExpression
     *  | additiveExpression ADDITIVE_OPERATOR additiveExpression
     *  ;
    */
    AstObject additiveExpression();

    /**
     * An multiplicativeExpression maybe:
     *  : primaryExpression
     *  | multiplicativeExpression ADDITIVE_OPERATOR primaryExpression
     *  ;
    */
    AstObject multiplicativeExpression();

    /**
     * An primaryExpression maybe:
     *  : literal
     *  | parenthesizedExpression
     *  | leftHandSideExpression
     *  ;
    */
    AstObject primaryExpression();

    /**
     * A parenthesizedExpression is:
     *  : "[" expression "]"
     *  ;
     * Note: this just return the included expression itself, no "type": "parenthesizedExpression" ...
     * Here may need to be only an additiveExpression, an assignmentExpression is not gramatically correct in rs274 // TODO
    */
    AstObject parenthesizedExpression();

    /**
     * a leftHandSideExpression may be:
     *  : variable
     *  ;
    */
    AstObject leftHandSideExpression();

    /**
     * an variable is:
     *  : "#" INTEGER (e.g. #02) (numberIndexVariable)
     *  | "#" VAR_NAME (e.g. #<myvar>) (nameIndexVariable)
     *  ;
    */
    AstObject variable();

    /**************************/
    /*****     literal     ****/
    /**************************/

    // /**
    //  * a literal is:
    //  *  : numericLiteral
    //  *  ;
    //  * Currently just a numeric Literal
    // */
    // AstObject literal();
    /**
     * a numericLiteral may be:
     *  : doubleNumericLiteral
     *  | integerNumericLiteral
     *  ;
    */
    AstObject numericLiteral();
    AstObject doubleNumericLiteral();
    AstObject integerNumericLiteral();

    TokenValue eat(const TokenType& token_type);
private:
    // helper internal static functions
    static bool IsAssignmentOperator(const Token& token);
    static const Token& IsValidAssignmentTarget(const Token& token);
private:
    std::unique_ptr<Tokenizer> _tokenizer;

    Token _lookahead;
};

} // namespace rs274letter
