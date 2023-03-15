#include "Serializer.h"

#include <cmath>
#include <iomanip>
#include <iostream>

#define VARIABLE_DEBUG_OUPUT

namespace rs274letter
{

static double _rad2deg(double reg) {
    static const double cvt = 180 / M_PI;
    return reg * cvt;
}

static double _deg2rad(double reg) {
    static const double cvt = M_PI / 180;
    return reg * cvt;
}

#define RS274LETTER_ASSERT_TYPE(v, type) \
    RS274LETTER_ASSERT(v.at("type").as_string() == type)

#define RS274LETTER_ASSERT_TYPE2(v, type1, type2) \
    RS274LETTER_ASSERT(v.at("type").as_string() == type1 || v.at("type").as_string() == type2)

void Serializer::storeVariable(const std::string &index, double value, bool assign_internal/* = false*/)
{
#ifdef VARIABLE_DEBUG_OUPUT
    const char* env = this->_isInSubEnvironment() ? "[Sub]" : "[Normal]";
    if (auto old = this->existsAndGetVariable(index)) {
        std::cout << env
            <<"Override nameIndexVariable, index: " << index
            << ", old_value: " << old.value()
            << ", new_value: " << value << std::endl;
    } else {
        std::cout << env
            << "Define new nameIndexVariable, index: " << index
            << ", value: " << value << std::endl;
    }
#endif

    std::stringstream ss;
    // examine whether the index stands for a global index
    if (_is_global_variable_name_index(index)) {
        // global index
        if (this->isInternalNameIndex(index) && !assign_internal) {
            ss << "Cannot assign to a internal variable"
               << ", index: " << index;
            throw SerializerError(ss.str());
        } else if (assign_internal) {
            // internal assign
            this->_global_nameindex_variable_value_map[index] = {
                GlobalVariableType::Internal, value
            };
        } else {
            // normal global assign
            this->_global_nameindex_variable_value_map[index] = {
                GlobalVariableType::Normal, value
            };
        }

        // end store
        return;
    }

    if (this->_isInSubEnvironment()) {
        // In a sub-environment
        this->_sub_nameindex_variable_value_map[index] = value;
    } else {   
        // Not in a sub-environment
        this->_nameindex_variable_value_map[index] = value;
    }
}

void Serializer::storeVariable(int index, double value)
{
#ifdef VARIABLE_DEBUG_OUPUT
    const char* env = this->_isInSubEnvironment() ? "[Sub]" : "[Normal]";
    if (auto old = this->existsAndGetVariable(index)) {
        std::cout << env
            <<"Override numberIndexVariable, index: " << index
            << ", old_value: " << old.value()
            << ", new_value: " << value << std::endl;
    } else {
        std::cout << env
            << "Define new numberIndexVariable, index: " << index
            << ", value: " << value << std::endl;
    }
#endif
    if (this->_isInSubEnvironment()) {
        // In a sub-environment
        this->_sub_numberindex_variable_value_map[index] = value;
    } else {   
        // Not in a sub-environment
        this->_numberindex_variable_value_map[index] = value;
    }
}

std::optional<double> Serializer::existsAndGetVariable(int index) const
{
    if (this->_isInSubEnvironment()) {
        auto it = this->_sub_numberindex_variable_value_map.find(index);
        if (it == this->_sub_numberindex_variable_value_map.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    } else {
        auto it = this->_numberindex_variable_value_map.find(index);
        if (it == this->_numberindex_variable_value_map.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    }
}

std::optional<double> Serializer::existsAndGetVariable(const std::string &index) const
{
    if (_is_global_variable_name_index(index)) {
        auto it = this->_global_nameindex_variable_value_map.find(index);
        if (it == this->_global_nameindex_variable_value_map.end()) {
            return std::nullopt;
        } else {
            return it->second.second;
        }
    }

    if (this->_isInSubEnvironment()) {
        auto it = this->_sub_nameindex_variable_value_map.find(index);
        if (it == this->_sub_nameindex_variable_value_map.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    } else {
        auto it = this->_nameindex_variable_value_map.find(index);
        if (it == this->_nameindex_variable_value_map.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    }
}

bool Serializer::isInternalNameIndex(const std::string &index) const
{
    // whether starts with _, if not, return false
    if (!_is_global_variable_name_index(index)) {
        return false;
    }

    // whether already exists in global, if not exists, return false
    auto it = this->_global_nameindex_variable_value_map.find(index);
    if (it == this->_global_nameindex_variable_value_map.end()) {
        return false;
    }

    // whether is marked as internal, if not internal, return false
    if (it->second.first == GlobalVariableType::Internal) {
        return true;
    } else {
        return false;
    }

}

void Serializer::processStatementList(const AstArray &statement_list)
{
    for (const auto& statement : statement_list) {
        this->processStatement(statement.as_object());
    }
}

void Serializer::processStatement(const AstObject& statement)
{
    auto&& statement_type = statement.at("type").as_string();

    if (statement_type == "expressionStatement") {
        this->processExpressionStatement(statement);
    } else if (statement_type == "commandStatement") {
        this->processCommandStatement(statement);
    } else if (statement_type == "oIfStatement") {
        this->processOIfStatement(statement);
    } else { // TODO
        std::stringstream ss;
        ss << "Unknown type of statement:" 
           << "\nstatement:" << statement.to_string();
        throw SerializerError(ss.str());
    }
}

void Serializer::processCommandStatement(const AstObject &command_statement)
{
    RS274LETTER_ASSERT_TYPE(command_statement, "commandStatement");
    CommandStatement cs;
    
    RS274LETTER_ASSERT(command_statement.at("commands").is_array());
    auto&& commands = command_statement.at("commands").as_array();
    for (const auto& command : commands) {
        RS274LETTER_ASSERT_TYPE(command, "commandNumberGroup");
        auto&& letter_str = command.at("letter").as_string();
        RS274LETTER_ASSERT(letter_str.size() == 1);
        
        auto number_value = this->getValue(command.at("number").as_object());

        cs.pushBack({letter_str[0], number_value});
    }

    // TODO IMPORTANT
    /**
     * 在这里需要对该行命令进行：
     *  - 检查，可能包括（命令分组、参数是否有误、是否有不兼容的同行命令等）
     *  - 根据命令，修改需要修改的环境变量（参照linuxcnc手册），如 #<_motion_mode>
     *  - 可能后续不会使用CommandStatement进行命令的最终输出，因为如果完成了检查，就应该可以更明确地进行命令分组
     * */

    this->_command_statement_list.emplace_back(std::move(cs));
}

double Serializer::processExpressionStatement(const AstObject &expression_statement)
{
    RS274LETTER_ASSERT_TYPE(expression_statement, "expressionStatement");
    return this->getValue(expression_statement.at("expression").as_object());
}

void Serializer::processOIfStatement(const AstObject &o_if_statement)
{
    RS274LETTER_ASSERT_TYPE(o_if_statement, "oIfStatement");
    
    double test_value = this->getValue(o_if_statement.at("test").as_object());

    // examine o-words
    // TODO

    if (!!test_value) {
        this->processStatementList(o_if_statement.at("consequent").as_array());
    } else {
        this->processStatementList(o_if_statement.at("alternate").as_array());
    }
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
    } else if (expression_type == "nameIndexVariable") {
        return this->getValueOfNameIndexVariable(expression);
    } else if (expression_type == "insideFunctionExpression") {
        return this->getValueOfInsideFunctionExpression(expression);
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
           << "\nindex:" << std::setprecision(10) << variable_index_d;
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

    if (auto variable_value = this->existsAndGetVariable(index_int)) {
        return variable_value.value();
    } else {
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

    if (auto variable_value = this->existsAndGetVariable(index_name)) {
        return variable_value.value();
    } else {
#ifdef NAMEINDEX_VARIABLE_UNDEFINED_ERROR
        std::stringstream ss;
        ss << "Use undefined nameIndexVariable:"
           << "\nindex:" << index_name
           << "\nvariable:" << v.to_string();
        throw SerializerError(ss.str());
#else // NOT NAMEINDEX_VARIABLE_UNDEFINED_ERROR
        return 0;
#endif // NAMEINDEX_VARIABLE_UNDEFINED_ERROR
    }
}

double Serializer::getValueOfBinaryExpression(const AstObject &expression)
{
    RS274LETTER_ASSERT_TYPE(expression, "binaryExpression");
    auto left_value = this->getValue(expression.at("left").as_object());
    auto right_value = this->getValue(expression.at("right").as_object());

    auto&& op = expression.at("operator").as_string();

#define _XX(x) \
    if (op == #x) {\
        return left_value x right_value;\
    }

#define _XX2(str, x) \
    if (op == #str) {\
        return left_value x right_value;\
    }

    _XX(+)
    else _XX(-)
    else _XX(*)
    else _XX(/)
    else if (op == "**") {
        return std::pow((double)left_value, (double)right_value);
    }
    else _XX(>)
    else _XX(<)
    else _XX(>=)
    else _XX(<=)
    else _XX(==)
    else _XX(!=)
    else _XX2(gt, >)
    else _XX2(lt, <)
    else _XX2(ge, >=)
    else _XX2(le, <=)
    else _XX2(eq, ==)
    else _XX2(ne, !=)
    else if (op == "and") {
        return left_value && right_value;
    }
    else if (op == "or") {
        return left_value || right_value;
    }
    else if (op == "xor") {
        return (bool)left_value ^ (bool)right_value;
    }
    
#undef _XX2
#undef _XX

    else {
        std::stringstream ss;
        ss << "Unexpected operator of binaryExpression:"
        << op << ", expression:\n" << expression.to_string();
        throw SerializerError(ss.str());
    }
}

double Serializer::getValueOfInsideFunctionExpression(const AstObject &expression)
{
    RS274LETTER_ASSERT_TYPE(expression, "insideFunctionExpression");

    auto&& function_name = expression.at("functionName").as_string();

    auto&& param = expression.at("param");
    double param_value;

    if (function_name == "atan") {
        // special function atan, return in this cpp if-branch
        RS274LETTER_ASSERT(param.is_array());
        RS274LETTER_ASSERT(param.as_array().size() == 2);

        // first param of atan2
        param_value = this->getValue(param.as_array()[0].as_object());
        
        // second param of atan2
        double param_value2 = this->getValue(param.as_array()[1].as_object());
        
        // reg2deg
        return _rad2deg(std::atan2(param_value, param_value2));
    } else if (function_name == "exists") {
        // special function exists, return in this cpp if-branch
        RS274LETTER_ASSERT_TYPE2(param, "nameIndexVariable", "numberIndexVariable");
        if (param.at("type").as_string() == "nameIndexVariable") {
            auto&& param_name_index = this->getNameIndexOfNameIndexVariable(param.as_object());
            return this->existsAndGetVariable(param_name_index).has_value();
        } else {
            auto&& param_number_index = this->getNumberIndexOfNumberIndexVariable(param.as_object());
            return this->existsAndGetVariable(param_number_index).has_value();
        }
    } else {
        RS274LETTER_ASSERT(param.is_object());
        param_value = this->getValue(param.as_object());
    }

    // normal inside functions
    if (function_name == "abs") {
        return std::abs(param_value);
    } else if (function_name == "acos") {
        return _rad2deg(std::acos(param_value));
    } else if (function_name == "asin") {
        return _rad2deg(std::asin(param_value));
    } else if (function_name == "cos") {
        return std::cos(_deg2rad(param_value));
    } else if (function_name == "exp") {
        return std::exp(param_value);
    } else if (function_name == "fix") {
        return std::floor(param_value); // Round down to integer
    } else if (function_name == "fup") {
        return std::ceil(param_value); // Round up to integer
    } else if (function_name == "round") {
        return std::round(param_value); // Round to nearest integer
    } else if (function_name == "ln") {
        return std::log(param_value);
    } else if (function_name == "sin") {
        return std::sin(_deg2rad(param_value));
    } else if (function_name == "sqrt") {
        return std::sqrt(param_value);
    } else if (function_name == "tan") {
        return std::tan(_deg2rad(param_value));
    }
    else {
        RS274LETTER_ASSERT(false); // TODO
        return -1;
    } 
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
        // numberIndexVariable
        int target_index = this->getNumberIndexOfNumberIndexVariable(target);

        this->storeVariable(target_index, value);
    } else {
        // nameIndexVariable
        auto&& target_index = this->getNameIndexOfNameIndexVariable(target);

        this->storeVariable(target_index, value);
    }
}

bool Serializer::_is_within_tolerance(const double &d)
{
    return (std::abs(d - std::round(d)) <= _s_double_to_integer_tolerance);
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

bool Serializer::_start_with_underline(const std::string &str)
{
    if (str.empty()) return false;
    if (str[0] == '_') return true; else return false;
}

bool Serializer::_is_global_variable(const AstObject &variable)
{
    RS274LETTER_ASSERT_TYPE2(variable, "numberIndexVariable", "nameIndexVariable");
    auto&& type = variable.at("type").as_string();

    if (type == "numberIndexVariable") {
        return false;
    } else {
        auto&& name_index = variable.at("index").as_string();
        return _start_with_underline(name_index);
    }
}

bool Serializer::_is_global_variable_name_index(const std::string &variable_index)
{
    return _start_with_underline(variable_index);
}

} // namespace rs274letter




