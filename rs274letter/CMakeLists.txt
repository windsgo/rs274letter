add_library(rs274letter SHARED
    Tokenizer.cc
)

target_include_directories(rs274letter PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/meojson/include>
)