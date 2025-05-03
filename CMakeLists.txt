cmake_minimum_required(VERSION 3.21)

project(squirrel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(squirrel WIN32 src/gui.cpp
                              src/main.cpp)

target_include_directories(squirrel PRIVATE include "C:/Program Files/SDL2/include")
target_link_directories(squirrel PRIVATE "C:/Program Files/SDL2/lib/x64")

target_link_libraries(squirrel kernel32 user32 ws2_32 sdl2)
