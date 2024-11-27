cmake_minimum_required(VERSION 3.10)
project(finalServer)

set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_PREFIX_PATH "/usr/include/SDL2" "/usr/local/include/SDL2")

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# 'manager' 실행 파일을 manager.c에서 생성
add_executable(manager manager.c)

# 'client' 실행 파일을 client.c에서 생성
add_executable(client client.c client_sdl.c )
target_link_libraries(client SDL2::SDL2 SDL2::SDL2_ttf)
