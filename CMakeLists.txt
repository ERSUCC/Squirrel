cmake_minimum_required(VERSION 3.21)

project(squirrel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SQUIRREL_SOURCES src/base64.cpp
                     src/files.cpp
                     src/gui.cpp
                     src/json.cpp
                     src/main.cpp
                     src/network.cpp)

if(APPLE)
    set(SQUIRREL_SOURCES ${SQUIRREL_SOURCES} src/files_mac.mm)
endif()

add_executable(squirrel ${SQUIRREL_SOURCES})

target_include_directories(squirrel PRIVATE include)

if(WIN32)
    set_target_properties(squirrel PROPERTIES WIN32_EXECUTABLE True)

    target_include_directories(squirrel PRIVATE "C:/Program Files/SDL2/include"
                                                "C:/Program Files/SDL2_ttf/include")

    target_link_directories(squirrel PRIVATE "C:/Program Files/SDL2/lib/x64"
                                             "C:/Program Files/SDL2_ttf/lib/x64")

    target_link_libraries(squirrel kernel32 user32 ws2_32 iphlpapi sdl2 sdl2_ttf)
elseif(APPLE)
    target_include_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl2/2.32.4_1/include/SDL2"
                                                "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/include/SDL2")

    target_link_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl2/2.32.4_1/lib"
                                             "/opt/homebrew/Cellar/sdl2_ttf/2.24.0/lib")

    target_link_libraries(squirrel "-framework CoreFoundation" "-framework AppKit" sdl2 sdl2_ttf)
else()
    target_include_directories(squirrel PRIVATE "/usr/include/SDL2"
                                                "/usr/include/SDL2_ttf")

    target_link_libraries(squirrel SDL2 SDL2_ttf)
endif()
