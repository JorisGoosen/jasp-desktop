#!/bin/bash
set -e
export DISPLAY=:99

Xvfb :99 -ac -screen 0 1280x1024x24 &
XVFB_PID=$!
sleep 2

export QT_QUICK_BACKEND=software
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 QT_ACCESSIBILITY=1
export QTWEBENGINE_RESOURCES_PATH=/home/virtuoos/JASP-screenreader/qt-install/usr/share/qt6/resources
export QTWEBENGINEPROCESS_PATH=/usr/lib/qt6/QtWebEngineProcess
export QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu"

dbus-run-session -- bash -c '
export DISPLAY=:99
export DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS
/usr/lib/at-spi-bus-launcher --launch-immediately & sleep 2
/usr/lib/at-spi2-registryd & sleep 1
/usr/bin/python3 /home/virtuoos/JASP-screenreader/jasp-desktop/Tests/test_accessibility.py
'

RET=$?
kill $XVFB_PID 2>/dev/null
wait 2>/dev/null
exit $RET