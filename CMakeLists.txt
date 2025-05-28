cmake_minimum_required(VERSION 3.21)

project(squirrel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(squirrel src/base64.cpp
                        src/files.cpp
                        src/gui.cpp
                        src/json.cpp
                        src/main.cpp
                        src/network.cpp)

if(WIN32)
    set_target_properties(squirrel PROPERTIES WIN32_EXECUTABLE True)

    target_include_directories(squirrel PRIVATE include "C:/Program Files/SDL2/include")
    target_include_directories(squirrel PRIVATE include "C:/Program Files/SDL2_ttf/include")

    target_link_directories(squirrel PRIVATE "C:/Program Files/SDL2/lib/x64")
    target_link_directories(squirrel PRIVATE "C:/Program Files/SDL2_ttf/lib/x64")

    target_link_libraries(squirrel kernel32 user32 ws2_32 iphlpapi)
elseif(APPLE)
    target_include_directories(squirrel PRIVATE include "/opt/homebrew/Cellar/sdl2/2.32.4_1/include/SDL2")
    target_include_directories(squirrel PRIVATE include "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/include/SDL2")

    target_link_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl2/2.32.4_1/lib")
    target_link_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/lib")
elseif(LINUX)
    # not sure yet
endif()

target_link_libraries(squirrel sdl2 sdl2_ttf)
