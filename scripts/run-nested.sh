#!/bin/sh
# Open WindWM inside a Xephyr X11 window (on labwc/Xwayland or any Xorg WM).
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
NESTED_DISPLAY=${NESTED_DISPLAY:-:100}
SIZE=${WIND_SIZE:-1200x760}
command -v Xephyr >/dev/null 2>&1 || {
    echo 'Please install Xephyr: sudo apt install xserver-xephyr' >&2
    exit 1
}
[ -x "$ROOT/wind" ] && [ -x "$ROOT/wind-about" ] || {
    echo "Missing wind or wind-about binary in $ROOT. Run 'make' first." >&2
    exit 1
}
Xephyr "$NESTED_DISPLAY" -screen "$SIZE" -resizeable -nolisten tcp -title 'WindWM - nested X11' &
xephyr_pid=$!
wm_pid=
cleanup() {
    if [ -n "$wm_pid" ]; then kill "$wm_pid" 2>/dev/null || :; fi
    kill "$xephyr_pid" 2>/dev/null || :
}
trap cleanup EXIT HUP INT TERM
# Wait until the nested X server starts, without assuming a fixed startup time.
i=0
while ! DISPLAY="$NESTED_DISPLAY" xset q >/dev/null 2>&1; do
    i=$((i+1))
    if [ "$i" -ge 80 ]; then echo 'Xephyr failed to start' >&2; exit 1; fi
    sleep 0.1
done
cd "$ROOT"
DISPLAY="$NESTED_DISPLAY" ./wind &
wm_pid=$!
# First window is optional; the desktop application menu is always available.
if command -v xterm >/dev/null 2>&1; then
    DISPLAY="$NESTED_DISPLAY" xterm -title 'WindWM Terminal' &
fi
wait "$wm_pid"
