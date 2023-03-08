add_library(rs274letter SHARED
    Tokenizer.cc
    Parser.cc
    util.cc
)

target_compile_definitions(rs274letter PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)

target_include_directories(rs274letter PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/meojson/include>
)

target_link_libraries(rs274letter PUBLIC backtrace)