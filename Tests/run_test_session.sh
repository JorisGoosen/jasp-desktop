#!/bin/bash
set -e
#
# run_test_session.sh  –  unified harness for JASP accessibility tests.
#
# Usage:
#   run_test_session.sh --test <script.py>
#       [--jasp-args <args>] [--wait <seconds>]
#       [--jasp-config key=value ...] [--no-headless]
#
#   --headless          force Xvfb even when a real display is present
#                       (without this flag, Xvfb is auto-started only
#                        when no running X server is detected)
#   --no-headless       prevent Xvfb autostart; require a real display
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env_accessibility.sh"

# ── defaults ──────────────────────────────────────────────────────────
TEST_SCRIPT=""
JASP_ARGS=""
WAIT_SEC=10
JASP_CONFIG_VARS=()
FORCE_HEADLESS=false
NO_HEADLESS=false

# ── parse args ────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --test)
            TEST_SCRIPT="$2"; shift 2 ;;
        --jasp-args)
            JASP_ARGS="$2"; shift 2 ;;
        --wait)
            WAIT_SEC="$2"; shift 2 ;;
        --jasp-config)
            JASP_CONFIG_VARS+=("$2"); shift 2 ;;
        --headless)
            FORCE_HEADLESS=true; shift ;;
        --no-headless)
            NO_HEADLESS=true; shift ;;
        *)
            echo "Unknown flag: $1"
            echo "Usage: $0 --test <script.py> [--jasp-args ...] [--wait N] [--jasp-config k=v] [--headless|--no-headless]"
            exit 2 ;;
    esac
done

if [[ -z "$TEST_SCRIPT" ]]; then
    echo "FATAL: --test is required"
    exit 1
fi

if [[ ! -f "$TEST_SCRIPT" ]]; then
    echo "FATAL: test script not found: $TEST_SCRIPT"
    exit 1
fi

TEST_NAME="$(basename "$TEST_SCRIPT" .py)"

# ── X server detection / Xvfb autostart ──────────────────────────────
XVFB_PID=""

_has_running_xserver() {
    # Check for a live X socket
    for sock in /tmp/.X11-unix/X*; do
        [[ -S "$sock" ]] && return 0
    done
    return 1
}

_start_xvfb() {
    echo "Starting Xvfb on $DISPLAY ..."
    Xvfb "$DISPLAY" -screen 0 1920x1080x24 +extension RANDR &
    XVFB_PID=$!
    sleep 1
    if ! kill -0 $XVFB_PID 2>/dev/null; then
        echo "FATAL: Xvfb failed to start"
        exit 1
    fi
    echo "Xvfb running (PID $XVFB_PID)"
}

if $NO_HEADLESS; then
    if [[ -z "${DISPLAY:-}" ]] || ! _has_running_xserver; then
        echo "FATAL: --no-headless specified but no X server found"
        exit 1
    fi
elif $FORCE_HEADLESS; then
    DISPLAY=:99
    _start_xvfb
elif ! _has_running_xserver; then
    [[ -z "${DISPLAY:-}" ]] && DISPLAY=:99
    _start_xvfb
fi

# ── JASP config (optional) ───────────────────────────────────────────
if [[ ${#JASP_CONFIG_VARS[@]} -gt 0 ]]; then
    JASP_CONFIG_DIR="${HOME}/.config/JASP"
    JASP_CONFIG_FILE="${JASP_CONFIG_DIR}/JASP.conf"
    mkdir -p "$JASP_CONFIG_DIR"

    echo "[General]" > "$JASP_CONFIG_FILE"
    for kv in "${JASP_CONFIG_VARS[@]}"; do
        echo "${kv}" >> "$JASP_CONFIG_FILE"
    done
    # Ensure the default section exists if user didn't prepend a section
    # (the [General] header is already written; subsequent sections are
    #  the caller's responsibility)
    echo "Config written: ${JASP_CONFIG_VARS[*]}"
fi

# ── log file ──────────────────────────────────────────────────────────
LOG_FILE="/tmp/jasp_${TEST_NAME}.log"

# ── export vars for the dbus sub-shell ─────────────────────────────────
export __jasp_bin="$JASP_BIN"
export __jasp_args="$JASP_ARGS"
export __jasp_log="$LOG_FILE"
export __jasp_wait="$WAIT_SEC"
export __jasp_test="$TEST_SCRIPT"
export __jasp_name="$TEST_NAME"
export __jasp_display="$DISPLAY"
export __jasp_qt_acc="$QT_LINUX_ACCESSIBILITY_ALWAYS_ON"
export __jasp_qt_acc2="$QT_ACCESSIBILITY"
export __jasp_qtwr="$QTWEBENGINE_RESOURCES_PATH"
export __jasp_qtwe="$QTWEBENGINEPROCESS_PATH"
export __jasp_qtcf="$QTWEBENGINE_CHROMIUM_FLAGS"

# ── main session ──────────────────────────────────────────────────────
dbus-run-session -- bash -c '
set +e
export DISPLAY="$__jasp_display"
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON="$__jasp_qt_acc"
export QT_ACCESSIBILITY="$__jasp_qt_acc2"
export QTWEBENGINE_RESOURCES_PATH="$__jasp_qtwr"
export QTWEBENGINEPROCESS_PATH="$__jasp_qtwe"
export QTWEBENGINE_CHROMIUM_FLAGS="$__jasp_qtcf"

cleanup() {
    kill $JASP_PID $ATSPI_BUS $ATSPI_REG 2>/dev/null
    wait $JASP_PID $ATSPI_BUS $ATSPI_REG 2>/dev/null
}
trap cleanup EXIT

/usr/lib/at-spi-bus-launcher --launch-immediately &
ATSPI_BUS=$!
sleep 2
/usr/lib/at-spi2-registryd &
ATSPI_REG=$!
sleep 1

echo "Starting JASP ..."
if [ -n "$__jasp_args" ]; then
    "$__jasp_bin" "$__jasp_args" >"$__jasp_log" 2>&1 &
else
    "$__jasp_bin" >"$__jasp_log" 2>&1 &
fi
JASP_PID=$!
sleep "$__jasp_wait"

if ! kill -0 $JASP_PID 2>/dev/null; then
  echo "FATAL: JASP exited prematurely (PID $JASP_PID)"
  tail -20 "$__jasp_log"
  exit 1
fi

echo "JASP running (PID $JASP_PID)"
echo "Running test: $__jasp_name"

/usr/bin/python3 "$__jasp_test"
rc=$?

if ! kill -0 $JASP_PID 2>/dev/null; then
  echo "WARNING: JASP exited during test run (PID $JASP_PID)"
  tail -20 "$__jasp_log"
fi

exit $rc
'
MAIN_RC=$?

# ── cleanup ───────────────────────────────────────────────────────────
if [[ -n "$XVFB_PID" ]] && kill -0 "$XVFB_PID" 2>/dev/null; then
    echo "Stopping Xvfb (PID $XVFB_PID)"
    kill "$XVFB_PID"
    wait "$XVFB_PID" 2>/dev/null
fi

exit $MAIN_RC