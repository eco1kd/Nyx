#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${ADB:-}" ]]; then
  ADB="$ADB"
elif command -v adb >/dev/null 2>&1; then
  ADB="$(command -v adb)"
else
  ADB="/home/leftcode/Projects/tools/android-tools-local/usr/bin/adb"
fi
ACTION="${1:-follow}"
PACKAGE="${2:-${PACKAGE:-com.axlebolt.standoff2}}"
TAG="LemmingRMT"

usage() {
  cat <<EOF
Usage: $0 [follow|dump|clear|diag|toggle|snapshot|crash] [package]

  follow    Follow only LemmingRMT logcat messages (default)
  dump      Print current buffered LemmingRMT messages
  clear     Clear logcat
  diag      Send SIGUSR2 to request a diagnostic dump
  toggle    Send SIGUSR1 to toggle the menu
  snapshot  Request diagnostics when permitted, then print tagged logs
  crash     Print crash buffer plus recent fatal/crash markers

Environment:
  ADB=/path/to/adb
  PACKAGE=com.axlebolt.standoff2
EOF
}

pid_for_package() {
  local pid
  pid="$($ADB shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r\n' || true)"
  if [[ -z "$pid" ]]; then
    echo "Process not found: $PACKAGE" >&2
    exit 2
  fi
  printf '%s' "$pid"
}

send_signal() {
  local signal="$1"
  local pid
  pid="$(pid_for_package)"
  echo "Sending $signal to $PACKAGE (pid $pid)"
  if "$ADB" shell "kill -$signal $pid" >/dev/null 2>&1; then
    return 0
  fi
  if "$ADB" shell su -c "kill -$signal $pid" >/dev/null 2>&1; then
    return 0
  fi
  echo "Signal permission denied (expected on non-root/non-debuggable devices). Use the in-menu diagnostics button." >&2
  return 1
}

case "$ACTION" in
  follow)
    exec "$ADB" logcat -v threadtime "$TAG:D" '*:S'
    ;;
  dump)
    exec "$ADB" logcat -d -v threadtime "$TAG:D" '*:S'
    ;;
  clear)
    exec "$ADB" logcat -c
    ;;
  diag)
    send_signal USR2
    ;;
  toggle)
    send_signal USR1
    ;;
  snapshot)
    send_signal USR2 || true
    sleep 1
    exec "$ADB" logcat -d -v threadtime "$TAG:D" '*:S'
    ;;
  crash)
    echo "===== crash buffer ====="
    "$ADB" logcat -b crash -d -v threadtime || true
    echo "===== recent fatal markers ====="
    "$ADB" logcat -d -v threadtime | grep -E "FATAL EXCEPTION|Fatal signal|DEBUG|crash_dump|tombstone|$TAG" | tail -n 800 || true
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac
