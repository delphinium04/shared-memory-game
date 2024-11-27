cmake_minimum_required(VERSION 3.10)
project(finalServer)

set(CMAKE_C_STANDARD 11)

# 'manager' 실행 파일을 manager.c에서 생성
add_executable(manager manager.c)

# 'client' 실행 파일을 client.c에서 생성
add_executable(client client.c )