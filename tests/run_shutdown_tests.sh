#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source_dir=$PWD
build_dir=$(realpath "${1:-build-linux}")
test_dir=$(mktemp -d)
cleanup_test() {
    test_status=$?
    if (( test_status != 0 )); then
        echo 'FAIL: shutdown regression; session logs follow' >&2
        cat "$test_dir/game-session/"*.log >&2 || true
    fi
    if [[ -f "$test_dir/game-session/session.pid" ]]; then
        kill "$(cat "$test_dir/game-session/session.pid")" 2>/dev/null || true
        flock -w 10 "$test_dir/game-session/launch.lock" true || true
    fi
    rm -rf -- "$test_dir"
}
trap cleanup_test EXIT
cp "$build_dir/manager" "$build_dir/client" "$test_dir/"
cp -r "$build_dir/src" "$test_dir/src"

cat >"$test_dir/close_window.c" <<'EOF'
#include <SDL.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
int __real_SDL_PollEvent(SDL_Event *event);
int __wrap_SDL_PollEvent(SDL_Event *event) {
    int fd = open(getenv("TEST_CLOSE_MARKER"), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd == -1) return __real_SDL_PollEvent(event);
    close(fd);
    event->type = SDL_QUIT;
    return 1;
}
EOF
gcc -std=gnu11 $(pkg-config --cflags sdl2 SDL2_ttf SDL2_image) \
    client.c client_sdl.c "$test_dir/close_window.c" \
    -Wl,--wrap=SDL_PollEvent $(pkg-config --libs sdl2 SDL2_ttf SDL2_image) \
    -o "$test_dir/client"

check_shutdown() {
    if ! flock -w 20 "$test_dir/game-session/launch.lock" true; then
        echo 'FAIL: session lock remained held after window closure' >&2
        return 1
    fi
    test ! -f "$test_dir/game-session/session.pid"
    for ((poll = 0; poll < 100; poll++)); do
        if ! pgrep -f "^$test_dir/(manager|client)$" > /dev/null; then
            return 0
        fi
        sleep 0.1
    done
    echo 'FAIL: game processes survived window closure' >&2
    return 1
}

for attempt in 1 2; do
    rm -f "$test_dir/closed"
    env DISPLAY=:0 SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
        TEST_CLOSE_MARKER="$test_dir/closed" \
        bash "$source_dir/scripts/start_game.sh" "$test_dir/manager" "$test_dir/client"
    check_shutdown
    test -f "$test_dir/closed"
    echo "PASS: window closure cleans up session; launch $attempt"
done

env DISPLAY=:0 SDL_VIDEODRIVER=invalid-test-driver \
    bash "$source_dir/scripts/start_game.sh" "$test_dir/manager" "$test_dir/client" || true
check_shutdown
grep -q 'SDL 초기화 오류' "$test_dir/game-session/"client-*.log
echo 'PASS: SDL initialization failure cleans up session'
