add_library(rs274letter SHARED
    Tokenizer.cc
    Parser.cc
    util.cc
    Serializer.cc
    InsideFunction.cc
)

find_package(Boost)
if (Boost_FOUND) 
    message(STATUS Boost has found)
    target_compile_definitions(rs274letter PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)
endif(Boost_FOUND)


# 不允许单行出现非赋值表达式
target_compile_definitions(rs274letter PRIVATE NO_SINGLE_NON_ASSIGN_EXPRESSION)

# 不允许赋值表达式右侧非primary expression, 如#1 = #2 + 1为非法，需写为#1 = [#2 + 1]
target_compile_definitions(rs274letter PRIVATE MUST_PRIMARY_RIGHT_HANDSIDE_OF_ASSIGN)

# 在o和#后面跟的直接指定为primaryExpression，即允许跟正负号+/-，允许跟字面浮点数(检查时再计算其是否
# 符合tolerance，符合退化为整数)
target_compile_definitions(rs274letter PRIVATE NAMEINDEX_JUST_PRIMARYEXPRESSION)

# 引用到number_index variable时如果未定义就抛出异常，否则返回0
target_compile_definitions(rs274letter PRIVATE NUMBERINDEX_VARIABLE_UNDEFINED_ERROR)
# 引用到name_index variable时如果未定义就抛出异常，否则返回0
target_compile_definitions(rs274letter PRIVATE NAMEINDEX_VARIABLE_UNDEFINED_ERROR)

target_include_directories(rs274letter PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/meojson/include>
)

if (UNIX)
target_link_libraries(rs274letter PUBLIC dl backtrace)
elseif(WIN32)

endif (UNIX)
