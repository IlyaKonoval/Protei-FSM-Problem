cmake_minimum_required(VERSION 3.10)
project(app CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)

if (_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak,address,undefined -g -Wall -Wextra")
    message(STATUS "Build type: Debug (sanitizers enabled)")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
    message(STATUS "Build type: Release")
endif ()

add_executable(${PROJECT_NAME} main.cpp)
