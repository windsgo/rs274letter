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
    AstArray statementList(const std::optional<std::vector<TokenType>>& stop_lookahead_tokentypes_after_o = std::nullopt);
    
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
     *  | oSubStatement
     *  | oWhileStatement
     *  | oRepeatStatement
     *  ;
     * if given pre_o_word, it will not eat an oCommand inside the function
    */
    AstObject oCommandStatement(const std::optional<AstObject> pre_o_word = std::nullopt);

    /**
     * an oCallStatement is:
     *  : (pre-oCommand) call oCallParamList "RTN"
     *  ;
    */
    AstObject oCallStatement(const AstObject& o_command_start);

    /**
     * an oReturnStatement is:
     *  : (pre-oCommand) return opt-parenthesizedExpression "RTN"
     *  ;
     * Should only appear in a subStatement block
    */
    AstObject oReturnStatement(const AstObject& o_command_start);

    /**
     * an oCallParamList is:
     *  : parenthesizedExpression parenthesizedExpression ... parenthesizedExpression
     *  ;
    */
    AstArray oCallParamList();
    
    /**
     * an oIfStatement is:
     *  : (pre-oCommand) if parenthesizedExpression "RTN" opt-statementList endif "RTN"
     *  | (pre-oCommand) if parenthesizedExpression "RTN" opt-statementList oCommand endif "RTN"
     *  ;
     */
    AstObject oIfStatement(const AstObject& o_command_start,
        bool should_eat_if = true);

    /**
     * an oSubStatement is:
     *  : (pre-oCommand) sub "RTN" opt-statementList endsub "RTN"
     *  ;
     * Do not allow nested oSubStatement
    */
    AstObject oSubStatement(const AstObject& o_command_start);

    /**
     * an oWhileStatement is:
     *  : (pre-oCommand) while parenthesizedExpression "RTN" opt-statementList endwhile "RTN"
    */
    AstObject oWhileStatement(const AstObject& o_command_start);

    /**
     * an oContinueStatement is:
     *  : (pre-oCommand) continue "RTN"
     *  ;
     * Should only appears in a while loop
    */
    AstObject oContinueStatement(const AstObject& o_command_start);

    /**
     * an oBreakStatement is:
     *  : (pre-oCommand) break "RTN"
     *  ;
     * Should only appears in a while loop
    */
    AstObject oBreakStatement(const AstObject& o_command_start);

    /**
     * an oRepeatStatement is:
     *  : (pre-oCommand) repeat parenthesizedExpression "RTN"
     *  ;
     */
    AstObject oRepeatStatement(const AstObject& o_command_start);

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
     *  relationalExpression
     *  additiveExpression
     *  multiplicativeExpression
     *  primaryExpression
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
     *  : relationalExpression,
     *  | leftHandSideExpression assignmentOperator relationalExpression
     *  ;
     * Note: the expanded assignmentExpression must have a leftHandSideExpresion on its left,
     * However, we can only first parse an `relationalExpression`, then examine if it is an `leftHandSideExpresion`
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
     * a relationalExpression is:
     *  : additiveExpression
     *  | relationalExpression RELATIONAL_OPERATOR relationalExpression
     *  ;
    */
    AstObject relationalExpression();
    
    /**
     * an additiveExpression maybe:
     *  : multiplicativeExpression
     *  | additiveExpression ADDITIVE_OPERATOR additiveExpression
     *  ;
    */
    AstObject additiveExpression();

    /**
     * a multiplicativeExpression maybe:
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
    static const AstValue& IsValidAssignmentTarget(const AstValue& ast_value);

    /**
     * isNextLineOEndif()
     * check if the next line is o... endif
     * this will not modify _tokenizer and _lookahead
    */
    // bool isNextLineOEndif() noexcept;

private:
    std::unique_ptr<Tokenizer> _tokenizer;

    Token _lookahead;
    
    // store the last oCommand AstObject
    // for sub flow control to save them

    // Note: any reference bound to this `_last_o_word` needs to pay attention
    // that `_last_o_word` may change during the call of this->statementList()
    // the reference bound to it may potentially change !
    AstObject _last_o_word;

    // used to examine if `o... return` is used in an `o... sub`
    bool _parsing_o_sub = false;

    // used to examine if `o... continue/break` is used in an `o... while`
    int _parsing_o_while_layers = 0; // layer stands for the loop nested layers, 0 is no loop
};

} // namespace rs274letter
