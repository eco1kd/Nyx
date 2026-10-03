#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ADB="${ADB:-/home/leftcode/Projects/tools/android-tools-local/usr/bin/adb}"
if [[ ! -x "$ADB" ]]; then ADB="$(command -v adb)"; fi
A=("$ADB"); [[ -z "${ANDROID_SERIAL:-}" ]] || A+=(-s "$ANDROID_SERIAL")
ACTION="${1:-capture}"
DURATION="${2:-45}"
TAG=LemmingRMT
OUT="${LEMMING_DIAGNOSTICS_DIR:-$ROOT/diagnostics/aim-debug}"
mkdir -p "$OUT"
case "$ACTION" in
 dump) "${A[@]}" logcat -d -v threadtime "$TAG:D" '*:S' | grep -E 'AIM|bootstrap started|ARM64 profile' || true ;;
 capture)
  [[ "$DURATION" =~ ^[0-9]{1,3}$ ]] && (( 10#$DURATION>=1 && 10#$DURATION<=300 )) || { echo 'Duration must be 1..300 seconds' >&2; exit 2; }
  "${A[@]}" get-state >/dev/null
  FILE="$OUT/capture-$(date +%Y%m%d-%H%M%S).log"
  # Android logcat -T 1 follows from one recent line, preserving the buffer.
  set +e
  timeout "$DURATION" "${A[@]}" logcat -T 1 -v threadtime "$TAG:D" '*:S' > "$FILE" 2>&1
  CODE=$?
  set -e
  [[ $CODE == 0 || $CODE == 124 ]] || { cat "$FILE" >&2; exit "$CODE"; }
  printf '%s\n' "$FILE" > "$OUT/latest-capture.txt"
  grep -E 'AIM|bootstrap started|ARM64 profile' "$FILE" > "${FILE%.log}-aim.log" || true
  echo "Saved: $FILE"
  if ! grep -q 'AIM DEBUG heartbeat build=v12-angle-assists' "$FILE"; then echo 'No v12 heartbeat: load the debug library and restart through your usual workflow. No install/restart was performed by this script.'; fi
  ;;
 *) echo "Usage: $0 [capture [1..300 seconds]|dump]" >&2; exit 2 ;;
esac
