cmake_minimum_required(VERSION 3.10)
project(sdltest LANGUAGES C)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "This game requires Linux (System V shared memory, fork, and POSIX signals). "
        "On Windows, build inside WSL/Ubuntu or select a Linux/WSL target in Visual Studio. "
        "See README.md for setup and build commands.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})

add_executable(manager manager.c)
add_executable(client client.c client_sdl.c)

add_custom_target(client_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/src"
    VERBATIM)
add_dependencies(client client_assets)

target_link_libraries(manager ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
target_link_libraries(client ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

option(GAME_AUTO_START "Start one manager and two clients after building" ON)
if(GAME_AUTO_START)
    add_custom_target(start_game ALL
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/start_game.sh"
            "$<TARGET_FILE:manager>" "$<TARGET_FILE:client>"
        DEPENDS manager client
        COMMENT "Starting one manager and two clients"
        VERBATIM)
endif()
