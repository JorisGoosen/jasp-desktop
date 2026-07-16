#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env_accessibility.sh"

JASP_CONFIG_DIR="${HOME}/.config/JASP"
JASP_CONFIG_FILE="${JASP_CONFIG_DIR}/JASP.conf"

if [ ! -f "$JASP_BIN" ]; then
  echo "FATAL: JASP binary not found at $JASP_BIN"
  exit 1
fi

mkdir -p "$JASP_CONFIG_DIR"
cat > "$JASP_CONFIG_FILE" <<'EOF'
[General]
useNativeFileDialog=false
checkUpdatesAskUser=false
themeName=darkTheme
EOF

echo "Config written: native file dialogs disabled"

dbus-run-session -- bash -c '
set +e
export DISPLAY='"$DISPLAY"'
export LANG='"$LANG"'
export LANGUAGE='"$LANGUAGE"'

cleanup() { kill $JASP_PID $ATSPI_BUS $ATSPI_REG 2>/dev/null; }
trap cleanup EXIT

/usr/lib/at-spi-bus-launcher --launch-immediately &
ATSPI_BUS=$!
sleep 2
/usr/lib/at-spi2-registryd &
ATSPI_REG=$!
sleep 1

# Start JASP normally (test opens CSV via File Menu)
'"$JASP_BIN"' >/tmp/jasp_csv.log 2>&1 &
JASP_PID=$!
sleep 10

if ! kill -0 $JASP_PID 2>/dev/null; then
  echo "FATAL: JASP exited prematurely (PID $JASP_PID)"
  tail -20 /tmp/jasp_csv.log
  exit 1
fi

echo "JASP running (PID $JASP_PID)"

/usr/bin/python3 '"$SCRIPT_DIR"'/test_csv_loading.py
rc=$?

if ! kill -0 $JASP_PID 2>/dev/null; then
  echo "WARNING: JASP exited during test run (PID $JASP_PID)"
  tail -20 /tmp/jasp_csv.log
fi

exit $rc
'