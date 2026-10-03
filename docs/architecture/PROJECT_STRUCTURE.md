# Project structure

This organization changes paths and build/test tooling, not game behavior. The v10 Aim diagnosis is still pending live verification. No anti-cheat bypass or new gameplay feature was added.

## Source map

```text
jni/
  Android.mk, Application.mk       Android NDK build configuration
  src/
    core/entry.cpp                Existing bootstrap, rendering and input integration
    game/offsets.hpp              Current version-specific game profile
    features/
      aim/                        Math/settings, runtime, RVAs and debug reports
      visuals/esp_runtime.hpp     Existing ESP runtime
      rage/, misc/                Reserved; not implemented
      config/, settings/          Reserved; not implemented
    ui/
      premium.hpp                 Current shared widgets, menu and ESP Editor
      pages/aim.hpp               Existing Aim controls
      pages/rage/, misc/          Reserved future UI
      pages/config/, settings/    Reserved future UI
      icons/, fonts/              Embedded C++ resources
  third_party/imgui/              Vendor code and its license
assets/
  icons/lucide/                   Original SVGs, source list and licenses
  fonts/mozilla_text/             Original TTF and OFL license
scripts/
  build.sh                       Android build implementation
  test.sh                        Reusable host unit/UI test runner
  check_structure.py             Includes, build inputs and layout validation
  debug/                         Existing ADB scripts
  maintenance/legacy/            Already-applied, non-repeatable migrations
  docs/, assets/, packaging/     Reserved future utilities (README only)
tests/
  aim/                           Math/runtime guards and diagnostic smoke test
  ui/                            Menu interaction/layout/GLES3 test
  stubs/android/log.h            Host-only Android logging stub
  rage/, misc/, config/           Reserved future tests
docs/
  architecture/                  This guide and file-move inventory
  debug/                         Current debugging guides
  game/                          Dump/RVA/ABI evidence
  archive/                       Older README and incompatible offset notes
reference/dumps/1.0.0/            Original dump, unchanged
build/
  ndk/                           New NDK intermediate files and libraries
  tests/                         Host test executables
  legacy-ndk/                    Previous generated caches, preserved
out/arm64-v8a/liblemmingrmt.so     Final Android library, same output path
diagnostics/
  aim-debug/                     Active Aim captures, same path as before
  tests/                         Fresh host test logs and screenshots
  organization/                  Migration and verification reports
  archive/                       Older sessions, loose logs and screenshots
backups/                         Source/release snapshot plus older backups
```

## Commands

From the root, continue using `./build.sh`, `./adb_logs.sh`, and `./aim_debug.sh capture 45`. These are small forwarding wrappers; their implementations live under scripts/. They resolve the project root correctly when invoked from another working directory.

New commands:
- `./test.sh` — all current host tests.
- `./test.sh unit` — Aim math/guards and debug formatting.
- `./test.sh ui` — interaction/layout and offscreen GLES3 when available.
- `python3 scripts/check_structure.py` — no device or compiler required.

NDK location can be overridden with ANDROID_NDK_HOME. Host compiler can be overridden with CXX. Build output remains out/arm64-v8a/. `./build.sh clean` now cleans intermediate NDK outputs only; it preserves the last release in out/ and historical backups. No clean was run during organization.

Test logs/screenshots are under diagnostics/tests/<run>/; latest-run.txt points to the newest run. The UI test no longer writes loose screenshots into diagnostics/.

## Compatibility and preservation

The old root `dump 1.0.0` name is a relative symlink to reference/dumps/1.0.0. Previous docs/AIM_DEBUG.md, docs/ADB_DEBUG.md and offset-document paths are also compatibility symlinks. Prefer canonical paths for new references. Historical log contents, absolute paths and old one-off patch scripts were not rewritten or executed.

file-moves.json records the renames. code-preservation.json records source-body comparisons made during migration. The backup contains a source/release tarball and a pre-migration SHA-256 inventory. Logs, dump bytes, vendor code, licenses and old build artifacts were preserved, not purged. Old .bak files are under backups/legacy-single-files/.

## Adding a future section

Place its math/settings and runtime under jni/src/features/<section>/ and presentation under jni/src/ui/pages/<section>/. Use explicit includes relative to jni/src (for example features/aim/core.hpp). Do not modify third_party/ merely to add a feature. Put host tests under tests/<section>/, not diagnostics/. Keep new .cpp build inputs explicit in Android.mk; README placeholders are intentionally not compiled.

The current entry.cpp, ESP runtime and premium menu remain monolithic internally. Moving them does not pretend to have extracted every subsystem; further splitting requires a separate behavior-preserving refactor and tests. Rage/Misc/Config/Settings directories are prepared but do not imply those features work.

Use of the game-modifying module violates the game's ToS and can lead to a ban.
