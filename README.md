# LemmingRMT

ARM64 / GLES3 project for Standoff 2 1.0.0, organized by responsibility.

## Current state

The current profile is v16-command-phase-hitbox. Automation now uses the verified native early/late weapon-command pair, before the original Attack gate and after late WeaponAction consumption. It no longer depends on finding uys. Module-owned buttons and the nullable Look command are cleaned up with local/world/token guards; Back camera is independent of the Angles toggle, and unavailable Auto scope does not block Triggerbot/Back camera. Silent refreshes the live selected hitbox, uses center-first Head and bounds any lead inside its current volume, with exact selected-collider validation and center fallback. See docs/game/COMMAND_PHASE_AND_SILENT_FIX_v16.md. Final ARM64 build and full host/UI/GLES regressions passed; neither proves on-device behavior or server damage. ADB has no connected device; no deployment, restart, reload or injection occurred. Both aim modes and automatic shooting still default OFF. Existing FOV/RCS/visibility/360/body controls and the no-scroll six-page menu remain. Rage/Config remain scaffolding; Skeleton, penetration, forged damage and anti-cheat bypass remain absent.

## Quick commands

```bash
./build.sh                    # ARM64 library -> out/arm64-v8a/
./test.sh                     # current host tests + structure check
./test.sh unit                # math/runtime guards + diagnostic formatting
./test.sh ui                  # menu/layout and offscreen GLES3 when available
python3 scripts/check_structure.py
./aim_debug.sh capture 45      # bounded, tag-only ADB capture
./adb_logs.sh dump             # buffered module logs
```

Set ANDROID_NDK_HOME for a different NDK, CXX for a different host compiler, ADB / ANDROID_SERIAL for a different ADB/device. `./build.sh clean` preserves out/ and backups and cleans only NDK intermediate files. No clean was run during organization.

## Where things live

- jni/src/core/: existing bootstrap/render/input integration.
- jni/src/game/: versioned game profile.
- jni/src/features/aim/: settings/math, runtime, Aim offsets, debug counters.
- jni/src/features/visuals/: existing ESP runtime.
- jni/src/features/rage/, misc/, config/, settings/: reserved future sections.
- jni/src/ui/: menu/editor, Aim page and embedded resources; future page folders are reserved.
- jni/third_party/imgui/: vendor code and license.
- assets/: original icons/fonts and licenses.
- scripts/: build, debug, validation and test tooling.
- tests/: reusable current host test sources and Android stub.
- docs/: architecture, debug guides, game evidence and historical notes.
- reference/dumps/1.0.0/: original dump, unchanged.
- build/: generated NDK files and host binaries; previous caches retained under legacy-ndk/.
- out/arm64-v8a/liblemmingrmt.so: final library, unchanged output path.
- diagnostics/aim-debug/: current Aim captures, unchanged path.
- diagnostics/tests/: fresh test logs/screenshots.
- diagnostics/archive/: preserved older sessions/reports/screenshots.
- backups/: source/release snapshots; no previous backup was deleted.

Detailed map and extension conventions: [PROJECT_STRUCTURE.md](docs/architecture/PROJECT_STRUCTURE.md).
Debug guide: [AIM_DEBUG.md](docs/debug/AIM_DEBUG.md).
Offset evidence: [AIM_OFFSETS_1.0.0_ARM64.md](docs/game/AIM_OFFSETS_1.0.0_ARM64.md).
Renames: [file-moves.json](docs/architecture/file-moves.json).

The original README is preserved in docs/archive/README_before_organization.md. Previous root scripts are forwarding wrappers; old dump and document names remain compatibility symlinks. Code was moved, not redesigned internally: entry.cpp, ESP and the shared premium menu still contain the existing implementations.

Use modifies game behavior, violates the game's ToS and may result in a ban.
