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

class SerializerError : public Exception {
public:
    SerializerError(const std::string& str) : Exception(str) {}
    virtual const char* what() const noexcept override {
        _str
            = "RS274Exception: SerializerError:\n" + _str;
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
        this->_parse_result = std::forward<T>(parse_result);
    }

public:
    // Interfaces

    template <typename T>
    double getVariableValue(const T& key) {
        if constexpr (std::is_constructible_v<std::string, T>) {
            return this->_nameindex_variable_value_map.at(key);
        } else if constexpr (std::is_constructible_v<int, T>) {
            return this->_numberindex_variable_value_map.at(key);
        } else {
            static_assert(std::is_constructible_v<std::string, T> 
                || std::is_constructible_v<int, T>, 
                "Not a valid key to getVariable()");
        }
    }

// private:
    /***********************/
    /*** private methods ***/
    /***********************/
    void calcVariable();

    // void processStatementList();
    // void processExpressionStatement();

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

    // get the calced index value of a number-indexed variable
    int getNumberIndexOfNumberIndexVariable(const AstObject& variable);
    double getValueOfNumberIndexVariable(const AstObject& v);
 
    std::string getNameIndexOfNameIndexVariable(const AstObject& variable);
    double getValueOfNameIndexVariable(const AstObject& v);

    double getValueOfBinaryExpression(const AstObject& expression);

    double getValueOfAssignmentExpression(const AstObject& expression);
    void assignVariable(const AstObject& target, double value);

private:
    // helper functions
    static bool _is_within_tolerance(const double& d);
    static std::optional<int> _convert_to_integer(const double& d, bool not_negative = true);

private:
    std::unordered_map<int, double> _numberindex_variable_value_map;
    std::unordered_map<std::string, double> _nameindex_variable_value_map;
    std::unordered_set<int> _numberindex_oword_set;
    std::unordered_set<std::string> _nameindex_oword_set;

    AstObject _parse_result;

    inline static std::size_t _s_max_loop_times = 1000;
    inline static double _s_double_to_integer_tolerance = 1e-6;
};

} // namespace rs274letter
