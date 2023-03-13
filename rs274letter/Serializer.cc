#include "Serializer.h"

#include <cmath>

namespace rs274letter
{

#define RS274LETTER_ASSERT_TYPE(v, type) \
    RS274LETTER_ASSERT(v.at("type").as_string() == type)

#define RS274LETTER_ASSERT_TYPE2(v, type1, type2) \
    RS274LETTER_ASSERT(v.at("type").as_string() == type1 || v.at("type").as_string() == type2)

void Serializer::calcVariable()
{
    
}

double Serializer::getValue(const AstObject &expression)
{
    auto&& expression_type = expression.at("type").as_string();

    if (expression_type == "doubleNumericLiteral" || expression_type == "integerNumericLiteral") {
        return this->getValueOfNumericLiteral(expression);
    } else if (expression_type == "binaryExpression") {
        return this->getValueOfBinaryExpression(expression);
    } else if (expression_type == "assignmentExpression") {
        return this->getValueOfAssignmentExpression(expression);
    } else if (expression_type == "numberIndexVariable") {
        return this->getValueOfNumberIndexVariable(expression);
    } else { // TODO
        std::stringstream ss;
        ss << "Unknown type of expression:" 
           << "\nexpression:" << expression.to_string();
        throw SerializerError(ss.str());
    }
}

double Serializer::getValueOfNumericLiteral(const AstObject &v)
{
    RS274LETTER_ASSERT_TYPE2(v, "doubleNumericLiteral", "integerNumericLiteral");
    return v.at("value").as_double();
}

double Serializer::getValueOfDoubleNumericLiteral(const AstObject &v)
{
    RS274LETTER_ASSERT_TYPE(v, "doubleNumericLiteral");
    return v.at("value").as_double();
}

int Serializer::getValueOfIntegerNumericLiteral(const AstObject &v)
{
    RS274LETTER_ASSERT_TYPE(v, "integerNumericLiteral");
    return v.at("value").as_integer();
}

int Serializer::getNumberIndexOfNumberIndexVariable(const AstObject &variable)
{
    RS274LETTER_ASSERT_TYPE(variable, "numberIndexVariable");
    auto&& index = variable.at("index").as_object(); // the index node
    double variable_index_d = this->getValue(index);

    std::stringstream ss;
    auto&& index_int_opt = _convert_to_integer(variable_index_d);
    if (!index_int_opt) {
        ss << "Invalid number index larger than tolerance or is negetive:"
           << "\ntolerance:" << this->_s_double_to_integer_tolerance
           << "\nindex:" << variable_index_d;
        throw SerializerError(ss.str());
    }

    RS274LETTER_ASSERT(index_int_opt.value() >= 0);
    return index_int_opt.value();
}

double Serializer::getValueOfNumberIndexVariable(const AstObject &v)
{
    RS274LETTER_ASSERT_TYPE(v, "numberIndexVariable");
    RS274LETTER_ASSERT(v.at("index").is_object());
    
    auto index_int = this->getNumberIndexOfNumberIndexVariable(v);

    auto it = this->_numberindex_variable_value_map.find(index_int);
    if (it == this->_numberindex_variable_value_map.end()) {
        // variable not defined
#ifdef NUMBERINDEX_VARIABLE_UNDEFINED_ERROR
        std::stringstream ss;
        ss << "Use undefined numberIndexVariable:"
           << "\nindex:" << index_int
           << "\nvariable:" << v.to_string();
        throw SerializerError(ss.str());
#else // NOT NUMBERINDEX_VARIABLE_UNDEFINED_ERROR
        return 0;
#endif // NUMBERINDEX_VARIABLE_UNDEFINED_ERROR
    } else {
        return it->second;
    }
}

std::string Serializer::getNameIndexOfNameIndexVariable(const AstObject &variable)
{
    RS274LETTER_ASSERT_TYPE(variable, "nameIndexVariable");
    RS274LETTER_ASSERT(variable.at("index").is_string());

    return variable.at("index").as_string();
}

double Serializer::getValueOfNameIndexVariable(const AstObject &v)
{
    RS274LETTER_ASSERT_TYPE(v, "nameIndexVariable");
    RS274LETTER_ASSERT(v.at("index").is_string());

    auto&& index_name = this->getNameIndexOfNameIndexVariable(v);
    auto it = this->_nameindex_variable_value_map.find(index_name);
    if (it == this->_nameindex_variable_value_map.end()) {
#ifdef NAMEINDEX_VARIABLE_UNDEFINED_ERROR
        std::stringstream ss;
        ss << "Use undefined nameIndexVariable:"
           << "\nindex:" << index_name
           << "\nvariable:" << v.to_string();
        throw SerializerError(ss.str());
#else // NOT NAMEINDEX_VARIABLE_UNDEFINED_ERROR
        return 0;
#endif // NAMEINDEX_VARIABLE_UNDEFINED_ERROR
    } else {
        return it->second;
    }
}

double Serializer::getValueOfBinaryExpression(const AstObject &expression)
{
    return 0.0;
}

double Serializer::getValueOfAssignmentExpression(const AstObject &expression)
{
    RS274LETTER_ASSERT_TYPE(expression, "assignmentExpression");

    // process the assign process
    auto&& left = expression.at("left").as_object();
    auto&& right = expression.at("right").as_object();
    RS274LETTER_ASSERT_TYPE2(left, "numberIndexVariable", "nameIndexVariable");

    double right_value = this->getValue(right);

    this->assignVariable(left, right_value);

    return right_value; // return the assigned value as the value of assignmentExpression
}

void Serializer::assignVariable(const AstObject &target, double value)
{
    RS274LETTER_ASSERT_TYPE2(target, "numberIndexVariable", "nameIndexVariable");
    if (target.at("type").as_string() == "numberIndexVariable") {
        int target_index = this->getNumberIndexOfNumberIndexVariable(target);

        // DEBUG
        auto it = this->_numberindex_variable_value_map.find(target_index);
        if (it != this->_numberindex_variable_value_map.end()) {
            std::cout << "Override numberIndexVariable, index: " << target_index
                << ", value:" << value << std::endl;
        } else {
            std::cout << "Define new numberIndexVariable, index: " << target_index
                << ", value:" << value << std::endl;
        }

        this->_numberindex_variable_value_map[target_index] = value;
    } else {
        // nameIndexVariable
        auto&& target_index = this->getNameIndexOfNameIndexVariable(target);

        // DEBUG
        auto it = this->_nameindex_variable_value_map.find(target_index);
        if (it != this->_nameindex_variable_value_map.end()) {
            std::cout << "Override nameIndexVariable, index: " << target_index
                << ", value:" << value << std::endl;
        } else {
            std::cout << "Define new nameIndexVariable, index: " << target_index
                << ", value:" << value << std::endl;
        }

        this->_nameindex_variable_value_map[target_index] = value;
    }
}

bool Serializer::_is_within_tolerance(const double &d)
{
    return (std::abs(d - std::round(d)) < _s_double_to_integer_tolerance);
}

std::optional<int> Serializer::_convert_to_integer(const double &d, bool not_negative /* = true*/)
{
    if (not_negative && d < 0.0) return std::nullopt;
    if (_is_within_tolerance(d)) {
        return static_cast<int>(d);
    } else {
        return std::nullopt;
    }
}

} // namespace rs274letter




