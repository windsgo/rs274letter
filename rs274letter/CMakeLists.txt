add_library(rs274letter SHARED
    Tokenizer.cc
    Parser.cc
    util.cc
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

target_include_directories(rs274letter PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/meojson/include>
)

if (UNIX)
target_link_libraries(rs274letter PUBLIC dl backtrace)
elseif(WIN32)

endif (UNIX)
