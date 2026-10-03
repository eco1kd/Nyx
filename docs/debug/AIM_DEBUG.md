> Current build: v16-command-phase-hitbox. See [COMMAND_PHASE_AND_SILENT_FIX_v16.md](../game/COMMAND_PHASE_AND_SILENT_FIX_v16.md) for the native early/late command pair, guarded cleanup, live selected-hitbox Silent, optional scope fallback and diagnostics. ARM64/full offline tests passed. No ADB device is connected; the assistant has not deployed/reloaded the module or restarted the game. Server damage remains unverified. Historical evidence follows.

# Aim debug — v10

Profile: `Standoff2-1.0.0-arm64-il2cppdump-v10-aim-debug`.
Artifact: `out/arm64-v8a/liblemmingrmt.so`.

This is a diagnostic build, not a claim that aiming or hit registration is fixed. Build, host tests and existing v9 logs do not validate v10 live hooks. Both modes remain OFF by default. The existing target filters, signature checks, UnityMain restriction and original-call forwarding remain enabled. No anti-cheat bypass was added.

## Current live evidence

The connected ARM64 Android 15 device ran v9 on 2026-10-01. Its bootstrap/profile marker was present, ESP update-hook calls increased, positions/camera were captured on UnityMain, and enemies were projected. No Aim hook-ready lines were found in that process's buffered logs. There was no periodic Aim diagnostic report in v9, so its enabled flags and exact failure stage cannot be inferred. Several menu-open intervals occurred; menu-pausing is an independent gate, not a confirmed root cause. The process later disappeared and ADB subsequently had no connected devices. No crash entry for that process was found in the accessible crash buffer; this does not prove how it exited.

Pre-change tagged logs: `diagnostics/aim-debug/before.log`.

## Repeat a bounded test

1. Reconnect the phone and load the new `.so` using your existing workflow. Rebuilding on the PC does NOT replace the module already loaded on the phone.
2. Confirm the `v10-aim-debug` bootstrap or `AIM DEBUG heartbeat build=v10-aim-debug` line. An old v9 marker is not a v10 test.
3. Enable Angles only; select Head and Chest. For a continuous diagnostic test, turn OFF Only while attacking. Leave menu-pausing ON and close the menu through the watermark. Face a nearby target in a test scene.
4. Run `./aim_debug.sh capture 45` in the project directory while reproducing the issue. Files are saved under `diagnostics/aim-debug/`; `latest-capture.txt` identifies the last capture. Run `./aim_debug.sh dump` to show buffered Aim lines.
5. Test Silent separately and fire several shots. Silent's callback is expected only on shot emission, not while idle. Its configuration is independent.

The capture script never installs, injects, restarts, signals the game, requires root, or clears logcat. It records only the LemmingRMT tag and stops after the requested duration (1..300 seconds). Set ADB or ANDROID_SERIAL when needed.

## Log interpretation

- Automatic report: six `AIM DEBUG` lines approximately every 6 seconds, including when modes are disabled or callbacks never arrive.
- `heartbeat`: angle/silent enabled, menu state, pause, attack-only, body masks, FOV and distance. FOV is an angular selection filter; v9/v10 do not draw an FOV circle.
- `callbacks`: player callbacks, active UnityMain callbacks, inactive/runtime/thread rejects, signature attempts/failures, camera/shot callbacks.
- `signature mismatch`: exact failing RVA, word index, received and expected instruction. Do not disable this guard to make a mismatched profile run.
- `hooks`: attempts, installed flags and failure reason. `vtable-scan-miss` or `camera-class-missing` differs from `vtable-write-probe-denied`. Silent logs callsite mismatch, relay allocation and memory-protection failures plus errno where available.
- `capture`: skipped snapshots/local/world/array/pose; collider/class/bone/native/bounds rejection counts; captured volume counts. Unknown bones stay rejected; no guessed remapping was added.
- `selection`: body/range/FOV rejection counts and nearest eligible-part angle/distance. Counts are cumulative, not unique players. Nearest 180 with distance 0 means no usable point was measured.
- `run`: disabled, menu-paused, empty-body-mask, wrong-thread, unvalidated-functions, rejected camera context/transform, not-attacking, no-target, applied, stack-span rejection or foreign-shooter/world rejection.
- `applications`: successful local angle/ray writes, NOT server hit acceptance.
- `local mask`: expected `fff`. Bits: 001 non-null owner; 002 expected player class; 004 readable local flag; 008 raw local=1; 010 non-null world; 020 snapshot readable; 040 snapshot local-owned; 080 valid stats; 100 alive; 200 valid team; 400 weapon part; 800 active weapon. `attackRaw` expected 1 while attacking; -1 means unreadable/not checked.
- Counts and samples are atomic but reports are not transactionally coherent snapshots; compare changes across consecutive heartbeats. Fresh hitbox totals are refreshed only during capture; if callbacks stop they can retain the last displayed count.

Debug adds counters and extra point calculations. Its live performance/stability still requires device testing. It does not implement visibility, penetration, hitchance or server-side validation.

Use violates the game's ToS and may result in a ban.
