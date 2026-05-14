cmake_minimum_required(VERSION 3.16)
project(ifc_parser_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ifc-parse
    main.cpp
    io/ifc_file_reader.cpp
    geometry_normalization/geometry_myimpl.cpp
    geometry_normalization/geometry_libimpl.cpp
)

target_include_directories(ifc-parse PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

enable_testing()
add_test(
    NAME smoke_readme_commands
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_smoke_tests.sh" "$<TARGET_FILE:ifc-parse>"
)
set_tests_properties(smoke_readme_commands PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
