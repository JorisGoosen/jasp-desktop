#!/bin/bash
set -e

export DISPLAY=:0.0
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 QT_ACCESSIBILITY=1
export QTWEBENGINE_RESOURCES_PATH=/home/virtuoos/JASP-screenreader/qt-install/usr/share/qt6/resources
export QTWEBENGINEPROCESS_PATH=/home/virtuoos/JASP-screenreader/qt-install/usr/lib/qt6/QtWebEngineProcess
export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu --disable-software-rasterizer"

JASP_BIN=/home/virtuoos/JASP-screenreader/jasp-desktop/build/Desktop/JASP
SLEEP_FILE="/home/virtuoos/JASP-screenreader/jasp-desktop/build/Resources/Data Sets/Data Library/1. Descriptives/Sleep.jasp"
SLEEP_ESC=$(printf '%q' "$SLEEP_FILE")

dbus-run-session -- bash -c '
export DISPLAY=:0.0
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 QT_ACCESSIBILITY=1
export QTWEBENGINE_RESOURCES_PATH=/home/virtuoos/JASP-screenreader/qt-install/usr/share/qt6/resources
export QTWEBENGINEPROCESS_PATH=/home/virtuoos/JASP-screenreader/qt-install/usr/lib/qt6/QtWebEngineProcess
export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu --disable-software-rasterizer"

cleanup() { kill $JASP_PID $ATSPI_BUS $ATSPI_REG 2>/dev/null; }
trap cleanup EXIT

/usr/lib/at-spi-bus-launcher --launch-immediately &
ATSPI_BUS=$!
sleep 2
/usr/lib/at-spi2-registryd &
ATSPI_REG=$!
sleep 1

'"$JASP_BIN"' '"$SLEEP_ESC"' &
JASP_PID=$!
sleep 20

DISPLAY=:0.0 /usr/bin/python3 /home/virtuoos/JASP-screenreader/jasp-desktop/Tests/test_results_accessibility.py
'