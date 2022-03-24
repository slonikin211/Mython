cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(Mython)

set(SOURCES 
    src/main.cpp
    src/lexer_test_open.cpp
)
set(HEADERS
    src/test_runner_p.h
)

set(PAIRS
    src/lexer.cpp src/lexer.h
)

add_executable(mython ${SOURCES} ${HEADERS} ${PAIRS})

set(CXX_COVERAGE_COMPILE_FLAGS "-std=c++17 -Wall -Werror -g")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_COMPILE_FLAGS}")

set_target_properties(
    mython PROPERTIES
    CXX_STANDART 17
    CXX_STANDART_REQUIRED ON
)