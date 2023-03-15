#pragma once

#include "Tokenizer.h"
#include "Parser.h"
#include "Exception.h"
#include "macro.h"

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace rs274letter
{

using CommandNumberGroup = std::pair<char, double>;

class CommandStatement {
public:
    CommandStatement() noexcept = default;
    ~CommandStatement() noexcept = default;

    inline void pushBack(const CommandNumberGroup& cng) {
        auto it = command_map.find(cng.first);
        if (it == command_map.end()) {
            command_map.insert({cng.first, {cng.second}});
        } else {
            it->second.push_back(cng.second);
        }
    }

    inline unsigned int appearTimesOfLetter(char letter) const { 
        auto it = command_map.find(letter);
        return it == command_map.end() ? 0 : it->second.size();
    }

    inline bool has_letter(char letter) const 
    { return this->appearTimesOfLetter(letter) != 0 ; }

    inline std::optional<std::vector<double>> getNumbersOfLetter(char letter) const {
        auto it = command_map.find(letter);
        if (it == command_map.end()) 
            return std::nullopt;
        else 
            return it->second;
    }

    inline std::string toString() const {
        std::stringstream ss;
        for (const auto& group : command_map) {
            ss << group.first << ": ";
            for (const auto& number : group.second) {
                ss << number << ", ";
            }
            ss << "\n";
        }
        return ss.str();
    }
private:
    std::unordered_map<char, std::vector<double>> command_map;
};

inline std::ostream& operator<<(std::ostream& os, const CommandStatement& cs) {
    os << cs.toString();
    return os;
}

class SerializerError : public Exception {
public:
    SerializerError(const std::string& str) : Exception(str) {}
    virtual const char* what() const noexcept override {
        _str = "RS274Exception: SerializerError:\n" + _str;
        return _str.c_str();
    }
};

/**
 * This class will 
 *  - calculate the variables in the parsed result
 *  - serialize the commands to a list
 * Note:
 *  - Each while loop layer should be smaller than 1000 times by default, 
 *    otherwise throw exception.
 * 
 * Error:
 *  - Catch the exception 
*/
class Serializer {
public:
    using ptr = std::shared_ptr<Serializer>;
    ~Serializer() noexcept = default;

    Serializer() noexcept = default;
    
    // Construct or reset
    template <typename T>
    Serializer(T&& parse_result) noexcept {
        this->reset(std::forward<T>(parse_result));
    }

    template <typename T>
    void reset(T&& parse_result) noexcept {
        static_assert(std::is_constructible<AstObject, T>::value, "Parameter can't be used to construct a (AstObject)parse_result");
        this->clear();
        this->_parse_result = std::forward<T>(parse_result);


        // TODO 目前这些先搁置，等完成了正常的流程控制解析，再回来做命令检查、环境处理的时候做
        /**
         * setup the global environment
         * 1. 因为在解析变量值时可能会有global命名变量,在此处需要初始化一些,
         *    并在处理commandStatement时修改对应变量
         * 2. 初始化全局环境,如坐标系,运动模式,进给速度,等
        */

        this->storeVariable("_test_global", 1001, true);
    }

    inline void clear() {
        this->_parse_result.clear();
        this->_command_statement_list.clear();

        this->_nameindex_oword_set.clear();
        this->_numberindex_oword_set.clear();

        this->_nameindex_variable_value_map.clear();
        this->_numberindex_variable_value_map.clear();
        this->_sub_nameindex_variable_value_map.clear();
        this->_sub_numberindex_variable_value_map.clear();
        this->_global_nameindex_variable_value_map.clear();

        this->_is_in_sub = false;
    }

public:
    // Interfaces

    /**
     * @brief get the value (double) of the variable with the given key
     * @param key key could be either int or std::string or any type that 
     * can construct them. 
     * @details 
     *  - int: get `#1` like variable
     *  - std::string: get `#<var_name>`
     * @note
     *  This only returns the non-sub local variables and global variables
    */
    template <typename T>
    double getVariableValue(const T& key) const {
        if constexpr (std::is_constructible_v<std::string, T>) {
            return this->existsAndGetVariable(key).value();
        } else if constexpr (std::is_constructible_v<int, T>) {
            return this->existsAndGetVariable(key).value();
        } else {
            static_assert(std::is_constructible_v<std::string, T> 
                || std::is_constructible_v<int, T>, 
                "Not a valid key to getVariable()");
        }
    }

    inline std::list<CommandStatement> getCommandList() const {
        return this->_command_statement_list;
    }

    /**
     * @brief process the whole parse_result
    */
    void processProgram();

    template <typename T>
    std::optional<double> hasVariable(const T& key) const {
        if constexpr (std::is_constructible_v<std::string, T>) {
            return this->existsAndGetVariable(key);
        } else if constexpr (std::is_constructible_v<int, T>) {
            return this->existsAndGetVariable(key);
        } else {
            static_assert(std::is_constructible_v<std::string, T> 
                || std::is_constructible_v<int, T>, 
                "Not a valid key to hasVariable()");
        }
    }

    std::string getAllVariablesPrinted() const {
        std::stringstream ss;

        ss << "normal number indexed:\n";
        for (const auto& i : this->_numberindex_variable_value_map) {
            ss << i.first << ":\t" << i.second << "\n";
        }

        ss << "normal name indexed:\n";
        for (const auto& i : this->_nameindex_variable_value_map) {
            ss << i.first << ":\t" << i.second << "\n";
        }

        ss << "global:\n";
        for (const auto& i : this->_global_nameindex_variable_value_map) {
            ss << i.first << ":\t" << i.second.second << ", "
            << (i.second.first == GlobalVariableType::Internal ? "Internal" : "Normal") 
            << "\n";
        }

        return ss.str();
    }

private:
    /**************************/
    /***  variable related  ***/
    /**************************/

    /**
     * These functions are used when the statements are trying to
     * store and search variables. These functions will automatically
     * figure out whether it is a `sub environment` and whether the
     * given `key`/`index` exists or is a global index
     * @throw If anything wrong, will throw exception, since it is a statement error 
    */

    /**
     * @brief Store value to a name-indexed variable. Determine whether it is
     * sub environment or not inside automatically.
     * @param index name_index string
     * @param value stored value
     * @param assign_internal set true if it is an internal-assign, default false
     * @throw if assign to an internal value without assign_internal=true
    */
    void storeVariable(const std::string& index, double value, bool assign_internal = false);

    /**
     * @brief Store value to a number-indexed variable. Determine whether it is
     * sub environment or not inside automatically.
     * @param index number_index int
     * @param value stored value
    */
    void storeVariable(int index, double value);

    /**
     * @brief try to get the value of a number-indexed variable. Determine whether
     * it is sub environment or not inside automatically.
     * @param index number_index int
     * @return std::nullopt if not exist, otherwise the std::optional will hold
     * the found value
    */
    std::optional<double> existsAndGetVariable(int index) const;

    /**
     * @brief try to get the value of a name-indexed variable. Determine whether
     * it is sub environment or not inside automatically.
     * @param index name_index string
     * @return std::nullopt if not exist, otherwise the std::optional will hold
     * the found value
    */
    std::optional<double> existsAndGetVariable(const std::string& index) const;

    /**
     * @brief returns true if the index is a global index (starts with '_')
     * `AND` the index exists in the _global_var_map
     * `AND` the found exist variable is marked as `Internal`.
     * Otherwise returns false;
     * @param index name_index string
    */
    bool isInternalNameIndex(const std::string& index) const;

    
    /**************************/
    /*** process statements ***/
    /**************************/

    /**
     * @brief produce a flow jump control
     * Used to record the current flow_jump_statement waiting wo be consumed
    */
    void produceFlowControl(const AstObject& flow_jump_statement);

    /**
     * @brief try to consume the flow control produced
     * It should be called after the while statement receive a RETURN_STATE_NEED_CONTINUE
     * or RETURN_STATE_NEED_BREAK or FLOW_STATE_NEED_RETURN
     * @throw throw if flow_jump type is matched but o-words do not match 
     * @note:
     *  - if currently flow state is normal, this does nothing
     *  - if currently flow state isn't normal, and is not while-break or while-continue
     *    or sub-return, this does nothing
     *  - if currently flow state isn't normal, and match while-break or while-continue
     *    or sub-return, but o-words doesn't match, throw
     *  - if currently flow state isn't normal, and match while-break or while-continue
     *    or sub-return, and o-words all match, will set current_flow state to normal,
     *    and clear the current flow_jump_statement
    */
    void consumeFlowControl(const AstObject& this_current_statement);

    /**
     * @brief process a statement list
     * This will process each statement in the statement list
    */
    void processStatementList(const AstArray& statement_list);

    /**
     * @brief process a statement
     * @param statement Any statement that is allowed
     * @throw meet any error
    */
    void processStatement(const AstObject& statement);

    /**
     * @brief process the command statement. 
     * This will calculate every number after each command-letter,
     * and then push back the CommandStatement struct into the
     * internal command statement list
    */
    void processCommandStatement(const AstObject& command_statement);
    
    /**
     * @brief process the expression statement.
     * This will do all the assignments in the expression statement,
     * store the assigned target value into the internal variable maps
    */
    void processExpressionStatement(const AstObject& expression_statement);
    
    /**
     * @brief process the o-if statement.
     * This will calculate the `test`(condition)s of the if statement,
     * go into the right branch and process the statement list inside.
     * @throw If `test` can not be calculated as a value
    */
    void processOIfStatement(const AstObject& o_if_statement);

    /**
     * @brief process the o-while statement.
     * @throw any error or the single while loop times is larger than allowed.
    */
    void processOWhileStatement(const AstObject& o_while_statement);

    /**
     * 
    */
    void processOContinueStatement(const AstObject& o_continue_statement);
    void processOBreakStatement(const AstObject& o_break_statement);

    /**************************/
    /*** astnode value get  ***/
    /**************************/

    /**
     * These functions are used when calculating the value of expressions
    */

    /**
     * @brief get the calculated value of any type of expression
     * @param expression Any expression that could return a value. 
     * Throw exception if cannot get a value
    */
    double getValue(const AstObject& expression);

    /**
     * @brief get the value of a numeric literal
     * @param v the numeric literal
    */
    double getValueOfNumericLiteral(const AstObject& v);
    double getValueOfDoubleNumericLiteral(const AstObject& v);
    int getValueOfIntegerNumericLiteral(const AstObject& v);

    /**
     * @brief get the calculated index value of a number-indexed variable
     * @param variable the number-indexed variable
    */
    int getNumberIndexOfNumberIndexVariable(const AstObject& variable);

    /**
     * @brief get the calculated value of a number-indexed variable
     * @param v the number-indexed variable
    */
    double getValueOfNumberIndexVariable(const AstObject& v);
    
    /**
     * @brief get the name-index string of a name-indexed variable
     * @param variable the name-indexed variable
    */
    std::string getNameIndexOfNameIndexVariable(const AstObject& variable);

    /**
     * @brief get the calculated value of a name-indexed variable
     * @param v the name-indexed variable
    */
    double getValueOfNameIndexVariable(const AstObject& v);

    /**
     * @brief get the calculated value of a binaryExpression
    */
    double getValueOfBinaryExpression(const AstObject& expression);

    /**
     * @brief get the calculated value of an insideFunctionExpression
    */
    double getValueOfInsideFunctionExpression(const AstObject& expression);

    /**
     * @brief get the calculated value of an assignmentExpression
    */
    double getValueOfAssignmentExpression(const AstObject& expression);

    /**
     * @brief assign value to target, target should be a variable currently,
     * this is called by the getValueOfAssignmentExpression() while processing
    */
    void assignVariable(const AstObject& target, double value);

private: // private status function, just easier for further revise
    /**
     * @brief returns if is in the sub_environment
    */
    inline bool _isInSubEnvironment() const { return _is_in_sub; }

private: // static 
    // helper functions
    /**
     * @brief returns if `d` is within the tolerance to the nearest integer
    */
    static bool _is_within_tolerance(const double& d);

    /**
     * @brief convert a double to integer
     * @param not_negative if should be non-negative (>=0), set it to true, default false
     * @return std::nullopt if cannot convert, either because not within tolerance or 
     * because of the flag `not_negative`. Otherwise the std::optional holds the converted
     * integer.
    */
    static std::optional<int> _convert_to_integer(const double& d, bool not_negative = true);

    // these functions simply help to tell whether a variable `LOOKS LIKE` a global variable
    static bool _start_with_underline(const std::string& str);
    static bool _is_global_variable(const AstObject& variable);
    static bool _is_global_variable_name_index(const std::string& variable_index);

private:
    // normal variables
    std::unordered_map<int, double> _numberindex_variable_value_map;
    std::unordered_map<std::string, double> _nameindex_variable_value_map;

    // sub environment variables
    std::unordered_map<int, double> _sub_numberindex_variable_value_map;
    std::unordered_map<std::string, double> _sub_nameindex_variable_value_map;

    // global environment name-indexed variable
    enum GlobalVariableType { Internal = 0, Normal = 1 };
    std::unordered_map<std::string, std::pair<GlobalVariableType, double>> _global_nameindex_variable_value_map;

    // o-word set
    std::unordered_set<int> _numberindex_oword_set;
    std::unordered_set<std::string> _nameindex_oword_set;

    AstObject _parse_result;

private:
    // environment and status
    bool _is_in_sub = false; // if is in a sub environment

    enum FlowState {
        FLOW_STATE_NORMAL = 0,
        FLOW_STATE_NEED_CONTINUE, // tell the caller need to consume a continue
        FLOW_STATE_NEED_BREAK, // tell the caller need to consume a break
        FLOW_STATE_NEED_RETURN // tell the caller need to consume a return
    };

    FlowState _current_flow_state = FlowState::FLOW_STATE_NORMAL;
    AstObject _current_flow_control_statement;

private:
    std::list<CommandStatement> _command_statement_list;

private:
    inline static std::size_t _s_max_loop_times = 1000;
    inline static double _s_double_to_integer_tolerance = 1e-6;
};

} // namespace rs274letter
