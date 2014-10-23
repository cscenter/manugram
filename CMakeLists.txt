cmake_minimum_required(VERSION 2.8)

project(manugram)

set(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")

include_directories(src)

add_executable(main src/main.cpp)
