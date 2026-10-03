#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-$ROOT/../android-ndk-r30-linux/android-ndk-r30}"
if [[ ! -x "$NDK/build/ndk-build" && -x "$HOME/Downloads/android-ndk-r30/build/ndk-build" ]]; then
    NDK="$HOME/Downloads/android-ndk-r30"
fi
NDK_BUILD="$NDK/build/ndk-build"
[[ -x "$NDK_BUILD" ]] || { echo "NDK build tool not found: $NDK_BUILD" >&2; exit 1; }
cd "$ROOT"
COMMON=(NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk "NDK_OUT=$ROOT/build/ndk/obj" "NDK_LIBS_OUT=$ROOT/build/ndk/libs")
case "${1:-}" in
 clean) "$NDK_BUILD" "${COMMON[@]}" clean; echo 'Intermediate files cleaned; out/ and historical backups are preserved.'; exit 0 ;;
 '' ) ;;
 *) echo "Usage: $0 [clean]" >&2; exit 2 ;;
esac
"$NDK_BUILD" "${COMMON[@]}" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
mkdir -p "$ROOT/out/arm64-v8a"
cp -f "$ROOT/build/ndk/libs/arm64-v8a/liblemmingrmt.so" "$ROOT/out/arm64-v8a/"
echo "Built: $ROOT/out/arm64-v8a/liblemmingrmt.so"
