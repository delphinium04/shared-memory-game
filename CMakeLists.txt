cmake_minimum_required(VERSION 3.10)
project(sdltest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Use pkg-config to find SDL2, SDL2_ttf, and SDL2_image
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

# Link directories (This line added - Make sure to link directories)
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})

add_executable(sdltest main.c client_sdl.c)
add_executable(manager manager.c)
add_executable(client client.c client_sdl.c)

# Link SDL2, SDL2_ttf, and SDL2_image libraries
target_link_libraries(sdltest ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
target_link_libraries(manager ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
target_link_libraries(client ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
