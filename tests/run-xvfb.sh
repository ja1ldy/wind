#!/bin/sh
# Optional regression suite. Requires Xvfb, libxtst-dev and x11-xserver-utils.
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
DISPLAY_ID=${WIND_TEST_DISPLAY:-:111}
CONFIG=$(mktemp)
LOG=$(mktemp)
TESTBIN=$(mktemp)
server_pid=
wm_pid=
cleanup() {
    [ -z "$wm_pid" ] || kill "$wm_pid" 2>/dev/null || :
    [ -z "$server_pid" ] || kill "$server_pid" 2>/dev/null || :
    rm -f "$CONFIG" "$LOG" "$TESTBIN"
}
trap cleanup EXIT HUP INT TERM
cat > "$CONFIG" <<'LUA'
wind = { theme="sunset", wallpaper="none", focus_follows_mouse=true }
LUA
cc -std=c11 -O2 -Wall -Wextra -Wpedantic "$ROOT/tests/x11-smoke.c" -o "$TESTBIN" $(pkg-config --cflags --libs x11 xtst)
Xvfb "$DISPLAY_ID" -screen 0 1024x768x24 -nolisten tcp >"$LOG" 2>&1 &
server_pid=$!
i=0
until DISPLAY="$DISPLAY_ID" xprop -root >/dev/null 2>&1; do
    i=$((i+1))
    [ "$i" -lt 60 ] || { cat "$LOG" >&2; exit 1; }
    sleep 0.1
done
(cd "$ROOT"; DISPLAY="$DISPLAY_ID" exec ./wind --config "$CONFIG") >"$LOG" 2>&1 &
wm_pid=$!
sleep 0.3
DISPLAY="$DISPLAY_ID" "$TESTBIN"
