// Parser.h
#pragma once

#include <string>
#include <memory>

#include "json.hpp"

#include "Tokenizer.h"
#include "Exception.h"

namespace rs274letter
{

class SyntaxError : public Exception {
public:
    SyntaxError(const std::string& str) : Exception(str) { }
    virtual const char* what() const noexcept override {
        static std::string str
            = "RS274Exception: SyntaxError:\n" + _str;
        return str.c_str();
    }
};

/**
 * AstObject
 * used to store each AST node, and construct the whole AST
*/
using AstValue = json::value;
using AstObject = json::object;
using AstArray = json::array;

class Parser {
    // friend class _BackupParserState;
public:
    ~Parser() noexcept = default;

    /**
     * parse()
     * parse the whole program with a code string.
     * You can catch the rs274letter::Exception to get error message
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
    AstArray statementList(const std::optional<std::vector<TokenType>>& stop_lookahead_tokentypes = std::nullopt);

    /**************************/
    /*****    Statement    ****/
    /**************************/

    /**
     * A statement may be a:
     *  : emptyStatement
     *  | commandStatement
     *  | expressionStatement
     *  | oCommandStatement (may be deleted) // TODO
     *  | ifStatement
     *  ;
    */
    AstObject statement();


    /**
     * A emptyStatement may be:
     *  : "RTN"
     *  ;
    */
    // AstObject emptyStatement();

    /**
     * A commandStatement may be:
     *  : commandNumberGroupList "RTN"
     *  ;
    */
    AstObject commandStatement();

    /**
     * An expressionStatement may be:
     *  : expression "RTN"
     *  ;
    */
    AstObject expressionStatement();

    /**
     * An oCommandStatement may be:
     *  : oCallStatement
     *  | oIfStatement 
     *  ;
    */
    AstObject oCommandStatement();

    /**
     * an oCallStatement is:
     *  : (o_command_start) call optParenthesizedExpressionList "RTN"
     *  ;
    */
    AstObject oCallStatement(const AstObject& o_command_start);
    
    /**
     * an ifStatement is:
     *  : if parenthesizedExpression "RTN" opt-statementList endif
     *  | if parenthesizedExpression "RTN" opt-statementList else statementList endif
     *  ;
    */
    AstObject ifStatement();

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

    /**
     * an oCommand is:
     *  : O < nameIndex >
     *  | O numberIndex
     *  ;
    */
    AstObject oCommand();


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
     * @param: must_be_assignment
    */
    AstObject expression(bool must_be_assignment = false);

    /**
     * An assignmentExpression may be:
     *  : additiveExpression,
     *  | leftHandSideExpression assignmentOperator additiveExpression
     *  ;
     * Note: the expanded assignmentExpression must have a leftHandSideExpresion on its left,
     * However, we can only first parse an `additiveExpression`, then examine if it is an `leftHandSideExpresion`
     * @param bool must_be_assignment
    */
    AstObject assignmentExpression(bool must_be_assignment = false);

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
     *  : optADDITIVE_OPERATOR numericLiteral
     *  | optADDITIVE_OPERATOR parenthesizedExpression
     *  | optADDITIVE_OPERATOR leftHandSideExpression
     *  ;
     * @param bool can_have_forward_additive_op
     * use the param because we don't want like `--#1` appears
    */
    AstObject primaryExpression(bool can_have_forward_additive_op = true);

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
     *  | "#" parenthesizedExpression (e.g. #[1+#4]) (numberIndexVariable)
     *  | "#" variable (e.g. ##[3+#7]) (numberIndexVariable)
     *  | "#" VAR_NAME (e.g. #<myvar>) (nameIndexVariable)
     *  ;
    */
    AstObject variable();

    /**
     * a nameIndex is an std::string with angle brackets removed
     *  : ("<") name_str (">")
     *  ;
    */
    std::string nameIndex();

    /**
     * a numberIndex is:
     *  : INTEGER(should be positive)
     *  | parenthesizedExpression
     *  | variable
     *  ;
    */
    AstValue numberIndex();

    /**************************/
    /*****     literal     ****/
    /**************************/

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

    /**
     * isNextLineOEndif()
     * check if the next line is o... endif
     * this will not modify _tokenizer and _lookahead
    */
    // bool isNextLineOEndif() noexcept;

private:
    std::unique_ptr<Tokenizer> _tokenizer;

    Token _lookahead;
};

} // namespace rs274letter
