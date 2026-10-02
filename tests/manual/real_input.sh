#!/bin/sh
# Drives tests/manual/real_input.cpp with real X11 input events from xdotool,
# covering what injected SDL events cannot: isKeyPressed(), keys typed through
# the OS keyboard layout, and a real window close request.
#
# Usage: tests/manual/real_input.sh path/to/real_input
#   (build it with: cmake --build build --target real_input)
#
# Needs xdotool and an X11 session or XWayland; on Wayland the program is run
# with SDL_VIDEO_DRIVER=x11. It opens a window and sends it keys and mouse
# events, so don't type while it runs. It checks that the test window has
# keyboard focus before every key it sends, and stops if not.
set -u

bin=${1:?usage: $0 path/to/real_input}
log=$(mktemp)
pid=
cleanup() {
    [ -n "$pid" ] && kill "$pid" 2>/dev/null
    rm -f "$log"
}
trap cleanup EXIT

fail() {
    echo "real_input: $*"
    echo "--- program output:"
    cat "$log"
    exit 1
}

# wait_for TEXT: waits until the program prints a line containing TEXT.
wait_for() {
    i=0
    until grep -qF -- "$1" "$log"; do
        i=$((i + 1))
        [ "$i" -gt 200 ] && fail "timed out waiting for '$1'"
        sleep 0.05
    done
}

# Stops before sending keys anywhere but the test window.
require_focus() {
    [ "$(xdotool getactivewindow 2>/dev/null)" = "$wid" ] ||
        fail "the test window lost keyboard focus; not sending more keys"
}

# step NAME XDOTOOL-ARGS...: waits for the program to reach NAME, then acts.
step() {
    name=$1
    shift
    wait_for "WAIT $name"
    require_focus
    xdotool "$@"
}

SDL_VIDEO_DRIVER=x11 "$bin" >"$log" 2>&1 &
pid=$!

wid=$(timeout 10 xdotool search --sync --name '^canvas real input test$' | head -n 1)
[ -n "$wid" ] || fail "test window did not appear"
eval "$(xdotool getwindowgeometry --shell "$wid")"  # sets WIDTH and HEIGHT

# Focus the window: ask the window manager, and click inside it.
wait_for "WAIT start-click"
xdotool windowactivate --sync "$wid" 2>/dev/null
# The pointer must have entered the window before a click counts.
xdotool mousemove --window "$wid" $((WIDTH / 2)) $((HEIGHT / 2))
sleep 0.2
xdotool click 1
sleep 0.3

step left-down keydown Left
step left-up keyup Left
step a-down keydown a
step a-up keyup a
step shift-down keydown shift
step shift-up keyup shift
step space-down keydown space
step space-up keyup space
step left-tap key Left
step typing type --delay 30 'Hi!x'
require_focus
xdotool key BackSpace Return

step mouse-move mousemove --window "$wid" $((WIDTH * 3 / 4)) $((HEIGHT / 4))
step mouse-down mousedown 1
sleep 0.1
step mouse-up mouseup 1
step quick-click click 1

# Close the window like a user would; the program should exit with status 0.
wait_for "WAIT close"
xdotool windowquit "$wid"
i=0
while kill -0 "$pid" 2>/dev/null; do
    i=$((i + 1))
    [ "$i" -gt 100 ] && fail "program still running after the window was closed"
    sleep 0.05
done
wait "$pid"
status=$?
pid=

cat "$log"
grep -q '^FAIL' "$log" && exit 1
[ "$status" -eq 0 ] || { echo "real_input: exit status $status, expected 0"; exit 1; }
echo "real_input: all steps passed"
