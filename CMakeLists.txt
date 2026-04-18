cmake_minimum_required(VERSION 3.10)
project(app)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -fsanitize=address -fsanitize=undefined -g -Wall -Wextra")
    message(STATUS "Selected ${CMAKE_BUILD_TYPE} build type. Building with sanitizers.")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O3 -static")
endif ()

add_executable(${PROJECT_NAME} main.cpp)