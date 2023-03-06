add_executable(general_test general_test.cc)
add_dependencies(general_test rs274letter)

target_include_directories(general_test PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/meojson/include>
)

target_link_libraries(general_test PRIVATE
    rs274letter
)