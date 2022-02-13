cmake_minimum_required(VERSION 3.21)
project(test)

set(CMAKE_CXX_STANDARD 23)

add_executable(test main.cpp)

target_link_libraries(test ${CMAKE_DL_LIBS})