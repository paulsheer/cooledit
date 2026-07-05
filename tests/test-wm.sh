#!/bin/bash
# test-wm.sh - run cooledit inside a specific WM under Xephyr
# Usage: ./test-wm.sh [icewm|fvwm2]
# Also accepts: icewm, fvwm, fvwm2

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COOLEDIT="${SCRIPT_DIR}/../editor/cooledit"

usage() {
    echo "Usage: $0 <icewm|fvwm2|dwm|ratpoison|blackbox>"
    echo ""
    echo "  icewm      Start cooledit inside IceWM under Xephyr"
    echo "  fvwm2      Start cooledit inside FVWM2 under Xephyr"
    echo "  fvwm       Same as fvwm2"
    echo "  dwm        Start cooledit inside dwm under Xephyr"
    echo "  ratpoison  Start cooledit inside ratpoison under Xephyr"
    echo "  blackbox   Start cooledit inside Blackbox under Xephyr"
    exit 1
}

WM="${1:-}"
case "$WM" in
    icewm)
        WM_BIN="icewm"
        WM_NAME="IceWM"
        ;;
    fvwm|fvwm2)
        WM_BIN="fvwm2"
        WM_NAME="FVWM2"
        ;;
    dwm)
        WM_BIN="dwm"
        WM_NAME="dwm"
        ;;
    ratpoison)
        WM_BIN="ratpoison"
        WM_NAME="ratpoison"
        ;;
    blackbox)
        WM_BIN="blackbox"
        WM_NAME="Blackbox"
        ;;
    *)
        usage
        ;;
esac

# Find a free display number (try :10..:19)
DISPLAY_NUM=""
for n in 10 11 12 13 14 15 16 17 18 19; do
    if [ ! -e "/tmp/.X11-unix/X${n}" ] || ! kill -0 "$(cat /tmp/.X11-unix/X${n}.pid 2>/dev/null)" 2>/dev/null; then
        DISPLAY_NUM="$n"
        break
    fi
done

if [ -z "$DISPLAY_NUM" ]; then
    echo "Error: no free X display (:10..:19) found" >&2
    exit 1
fi

XEPHYR_DISPLAY=":${DISPLAY_NUM}"
PARENT_DISPLAY="${DISPLAY:-:0}"

cleanup() {
    echo ""
    echo "=== Cleaning up ==="
    # Kill cooledit first
    pkill -f "cooledit.*${XEPHYR_DISPLAY}" 2>/dev/null || true
    # Kill the WM
    pkill -f "${WM_BIN}.*${XEPHYR_DISPLAY}" 2>/dev/null || true
    sleep 1
    # Kill Xephyr
    if [ -n "$XEPHYR_PID" ]; then
        kill "$XEPHYR_PID" 2>/dev/null || true
        wait "$XEPHYR_PID" 2>/dev/null || true
    fi
    echo "Cleaned up."
}

trap cleanup EXIT INT TERM

echo "=== Starting Xephyr on ${XEPHYR_DISPLAY} (parent: ${PARENT_DISPLAY}) ==="
Xephyr "$XEPHYR_DISPLAY" \
    -display "$PARENT_DISPLAY" \
    -screen 1024x768 \
    -ac \
    -reset \
    -noreset \
    2>/dev/null &
XEPHYR_PID=$!
sleep 2

if ! kill -0 "$XEPHYR_PID" 2>/dev/null; then
    echo "Error: Xephyr failed to start" >&2
    exit 1
fi

echo "=== Starting ${WM_NAME} ==="
export DISPLAY="${XEPHYR_DISPLAY}"
"$WM_BIN" &
WM_PID=$!
sleep 2

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "Error: ${WM_NAME} failed to start" >&2
    exit 1
fi

echo "=== Starting cooledit -S -m ==="
"$COOLEDIT" -S -m &
COOLEDIT_PID=$!
sleep 3

if ! kill -0 "$COOLEDIT_PID" 2>/dev/null; then
    echo "Error: cooledit failed to start" >&2
    exit 1
fi

echo ""
echo "=== All processes running ==="
echo "    Xephyr  PID: ${XEPHYR_PID}  display: ${XEPHYR_DISPLAY}"
echo "    ${WM_NAME}    PID: ${WM_PID}"
echo "    cooledit PID: ${COOLEDIT_PID}"
echo ""
echo "Press Enter to quit and clean up..."

read -r

# Cleanup happens via trap
