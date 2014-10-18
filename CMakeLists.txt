cmake_minimum_required(VERSION 2.8)

project(manugram)

set(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")

find_package(Boost)

include_directories(src)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(main src/main.cpp)
