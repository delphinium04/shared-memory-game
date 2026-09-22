#!/usr/bin/env bash
set -euo pipefail

if [[ ${1:-} != --session ]]; then
    manager=$(realpath "$1")
    client=$(realpath "$2")
    runtime="$(dirname "$manager")/game-session"
    mkdir -p "$runtime"
    exec 9>"$runtime/launch.lock"
    if ! flock -n 9; then
        echo "Game already running. Logs: $runtime"
        exit 0
    fi
    if [[ -z ${DISPLAY:-} && -z ${WAYLAND_DISPLAY:-} ]]; then
        echo "No GUI display. Skipping game launch (use Linux desktop or WSLg)."
        exit 0
    fi
    rm -f "$runtime/started" "$runtime/failed"
    nohup setsid bash "$0" --session "$manager" "$client" "$runtime" \
        >"$runtime/launcher.log" 2>&1 < /dev/null &
    exec 9>&-
    for ((attempt = 0; attempt < 150; attempt++)); do
        if [[ -f "$runtime/started" ]]; then
            cat "$runtime/started"
            exit 0
        fi
        if [[ -f "$runtime/failed" ]]; then
            cat "$runtime/launcher.log" >&2
            exit 1
        fi
        sleep 0.1
    done
    echo "Game launch timed out. See $runtime/launcher.log" >&2
    exit 1
fi

manager=$2
client=$3
runtime=$4
room=$(( (RANDOM << 15 | RANDOM) + 1 ))
pids=()
shmid=
cleanup() {
    trap - EXIT TERM INT
    for pid in "${pids[@]}"; do
        kill -TERM -- "-$pid" 2>/dev/null || true
    done
    wait || true
    if [[ -n "$shmid" ]]; then
        ipcrm -m "$shmid" 2>/dev/null || true
    fi
    rm -f "$runtime/session.pid" "$runtime/started"
    touch "$runtime/failed"
}
trap cleanup EXIT
trap 'exit 0' TERM INT
echo "$$" >"$runtime/session.pid"

wait_for_log() {
    local pid=$1 pattern=$2 log=$3
    for ((attempt = 0; attempt < 100; attempt++)); do
        if grep -q "$pattern" "$log"; then
            return 0
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "Game process exited during startup: $log" >&2
            cat "$log" >&2
            return 1
        fi
        sleep 0.05
    done
    echo "Timed out waiting for game process: $log" >&2
    return 1
}

: >"$runtime/manager.log"
setsid stdbuf -oL -eL "$manager" <<< "$room" >"$runtime/manager.log" 2>&1 9>&- &
pids+=("$!")
wait_for_log "${pids[0]}" 'Room ready:' "$runtime/manager.log"
shmid=$(sed -n 's/.*shmid=\([0-9]*\).*/\1/p' "$runtime/manager.log" | head -n 1)

for player in 1 2; do
    : >"$runtime/client-$player.log"
    setsid stdbuf -oL -eL "$client" <<< "$room" >"$runtime/client-$player.log" 2>&1 9>&- &
    pids+=("$!")
    wait_for_log "$!" 'waiting\.\.\.' "$runtime/client-$player.log"
done
printf 'Started manager + 2 clients (room %s). Logs: %s\n' "$room" "$runtime" >"$runtime/started"
wait -n "${pids[@]}" || true
