#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ACTION="${1:-all}"
case "$ACTION" in all|unit|ui) ;; *) echo "Usage: $0 [all|unit|ui]" >&2; exit 2 ;; esac
cd "$ROOT"
CXX="${CXX:-g++}"
BIN="$ROOT/build/tests/current"
LOG="$ROOT/diagnostics/tests/$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$BIN" "$LOG/screenshots"
printf '%s\n' "$LOG" > "$ROOT/diagnostics/tests/latest-run.txt"
INC=(-Ijni/src -Ijni/third_party/imgui -Ijni/third_party/imgui/backends -Itests/stubs)
run_case() {
 local name="$1"; shift
 "$CXX" -std=c++17 -O2 "${INC[@]}" "$@" -o "$BIN/$name" > "$LOG/$name-compile.log" 2>&1 || { cat "$LOG/$name-compile.log" >&2; return 1; }
 local args=(); [[ "$name" != ui ]] || args=("$LOG/screenshots/menu.ppm" "$LOG/screenshots/esp-editor.ppm")
 LEMMING_TEST_SCREENSHOT_DIR="$LOG/screenshots" "$BIN/$name" "${args[@]}" > "$LOG/$name-test.log" 2>&1 || { cat "$LOG/$name-test.log" >&2; return 1; }
 grep -E '^(PASS|SKIP)' "$LOG/$name-test.log" || true
}
python3 "$ROOT/scripts/check_structure.py" | tee "$LOG/structure.log"
if [[ "$ACTION" != ui ]]; then
 run_case aim tests/aim/aim_test.cpp -ldl -lpthread
 run_case debug_smoke tests/aim/debug_smoke.cpp -ldl -lpthread
 run_case hook_support tests/aim/hook_support_test.cpp -ldl -lpthread
 run_case assists tests/aim/assists_test.cpp -ldl -lpthread
 run_case input_init tests/aim/input_init_test.cpp -ldl -lpthread
 run_case prediction tests/aim/prediction_test.cpp -ldl -lpthread
 run_case automation tests/aim/automation_test.cpp -ldl -lpthread
 run_case automation_runtime tests/aim/automation_runtime_test.cpp -ldl -lpthread
 run_case command_phase tests/aim/command_phase_test.cpp -ldl -lpthread
 run_case silent_accuracy tests/aim/silent_accuracy_test.cpp -ldl -lpthread
 run_case native_pair tests/aim/native_pair_test.cpp -ldl -lpthread
fi
if [[ "$ACTION" != unit ]]; then
 run_case ui tests/ui/ui_test.cpp jni/third_party/imgui/imgui.cpp jni/third_party/imgui/imgui_draw.cpp jni/third_party/imgui/imgui_tables.cpp jni/third_party/imgui/imgui_widgets.cpp jni/third_party/imgui/backends/imgui_impl_opengl3.cpp -DIMGUI_IMPL_OPENGL_ES3 -lEGL -lGLESv2 -ldl -lpthread
fi
echo "Tests completed. Logs: $LOG"
