cmake_minimum_required(VERSION 3.21)

project(squirrel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SQUIRREL_SOURCES include/safe_queue.hpp
                     src/base64.cpp
                     src/errors.cpp
                     src/files.cpp
                     src/gui.cpp
                     src/json.cpp
                     src/main.cpp
                     src/network.cpp
                     src/renderer.cpp)

if(APPLE)
    set(SQUIRREL_SOURCES ${SQUIRREL_SOURCES} src/files_mac.mm)
endif()

add_executable(squirrel ${SQUIRREL_SOURCES})

target_include_directories(squirrel PRIVATE include)

if(WIN32)
    set_target_properties(squirrel PROPERTIES WIN32_EXECUTABLE True)

    target_include_directories(squirrel PRIVATE "C:/Program Files/SDL3/include"
                                                "C:/Program Files/SDL3/include/SDL3"
                                                "C:/Program Files/SDL3_ttf/include/SDL3_ttf")

    target_link_directories(squirrel PRIVATE "C:/Program Files/SDL3/lib/x64"
                                             "C:/Program Files/SDL3_ttf/lib/x64")

    target_link_libraries(squirrel kernel32 user32 ws2_32 iphlpapi sdl3 sdl3_ttf)
elseif(APPLE)
    target_include_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl3/3.2.22/include"
                                                "/opt/homebrew/Cellar/sdl3/3.2.22/include/SDL3"
                                                "/opt/homebrew/Cellar/sdl3_ttf/3.2.2/include/SDL3_ttf")

    target_link_directories(squirrel PRIVATE "/opt/homebrew/Cellar/sdl3/3.2.22/lib"
                                             "/opt/homebrew/Cellar/sdl3_ttf/3.2.2/lib")

    target_link_libraries(squirrel "-framework CoreFoundation" "-framework AppKit" sdl3 sdl3_ttf)
else()
    target_include_directories(squirrel PRIVATE "/usr/include/SDL3"
                                                "/usr/include/SDL3_ttf")

    target_link_libraries(squirrel SDL3 SDL3_ttf)
endif()
