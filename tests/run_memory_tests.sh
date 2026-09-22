#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_build="out/test-memory"
mkdir -p "$test_build"
read -r -a sdl_cflags <<< "$(pkg-config --cflags sdl2 SDL2_ttf SDL2_image)"
read -r -a sdl_libs <<< "$(pkg-config --libs sdl2 SDL2_ttf SDL2_image)"
flags=(-std=gnu11 -g -O1 -fsanitize=address,undefined -I.)
gcc "${flags[@]}" "${sdl_cflags[@]}" -Dmain=client_program_main \
    -c client.c -o "$test_build/client.o"
gcc "${flags[@]}" -Werror=uninitialized "${sdl_cflags[@]}" tests/test_sdl_memory.c client_sdl.c \
    "$test_build/client.o" "${sdl_libs[@]}" -o "$test_build/test_sdl_memory"
"$test_build/test_sdl_memory" coordinates
"$test_build/test_sdl_memory" pipes
