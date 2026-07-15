#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env_accessibility.sh"

dbus-run-session -- bash -c '
export DISPLAY='"$DISPLAY"'
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON='"$QT_LINUX_ACCESSIBILITY_ALWAYS_ON"'
export QT_ACCESSIBILITY='"$QT_ACCESSIBILITY"'
export QTWEBENGINE_RESOURCES_PATH='"$QTWEBENGINE_RESOURCES_PATH"'
export QTWEBENGINEPROCESS_PATH='"$QTWEBENGINEPROCESS_PATH"'
export QTWEBENGINE_CHROMIUM_FLAGS='"$QTWEBENGINE_CHROMIUM_FLAGS"'

cleanup() { kill $JASP_PID $ATSPI_BUS $ATSPI_REG 2>/dev/null; }
trap cleanup EXIT

/usr/lib/at-spi-bus-launcher --launch-immediately &
ATSPI_BUS=$!
sleep 2
/usr/lib/at-spi2-registryd &
ATSPI_REG=$!
sleep 1

'"$JASP_BIN"' >/dev/null 2>/dev/null &
JASP_PID=$!
sleep 10

/usr/bin/python3 '"$SCRIPT_DIR"'/test_help_accessibility.py
'