# v15 — local input initialization

## On-device evidence before this patch

ADB capture diagnostics/aim-debug/trigger-live-20261001-125601/current-process.log showed the running v14-prediction-fallback process with Triggerbot + Back camera + Auto scope enabled and menu closed. Angle/shot hooks and visibility were ready, but input hook stayed false and input callbacks, automation rays, attack requests, observed shots and view restores stayed zero. There were 68 enabled automation heartbeat snapshots. This establishes an input-initialization failure, not a FOV/prediction or server-hit failure. Existing v14 logs did not distinguish skipped attempts from component/signature/vtable/probe rejections.

## Source fixes

Input initialization now has a dedicated one-second retry timer, independent of the common all-player camera/shot timer. Any verified UnityMain player callback may resolve the authoritative local player and attempt its input hook; it no longer depends on the local callback being the first callback after the common timer expires. Missing local player does not consume a retry; changing local identity retries immediately. Disabled automation resets the retry and a ready hook prevents repeated installation.

Component discovery used by both input and view restoration no longer stops when wmz's primary component array is empty. It searches all six matching-dump arrays and then the Dictionary<Type,chx> fallback. Layout references: dump.cs mfc TypeDefIndex 15022 (components 0x20/world 0x18), wmz 15023 (arrays 0x10/18/20/28/30/38, dictionary 0x40), chx 14951 (owner 0x18/world 0x20); il2cpp.h Dictionary<Type,chx> fields (entries 0x18/count 0x20) and generic reference Entry fields (stride 0x18/value 0x10, removed hash <0). Bounds, container class, component class, owner and world must match. No new guessed native getters, offsets, packet writes or bypasses are introduced.

Input installation keeps verified native signatures, exact validated vtable-target matching, writable-slot checks, a same-value write probe, readback and rollback. Failure reasons now distinguish missing local player, wrong thread, invalid local state, missing input component, native-signature rejection, class/slot rejection, readonly slot, failed write probe/readback and unsupported architecture/runtime. Repeated logs are throttled; heartbeat includes attempts and reason, and Aim → Automation displays the current installation reason. Optional scope enum failure still does not prevent plain input initialization; Auto scope still fails closed.

## Verification and limitations

Regression tests cover the actual onPlayer callback: a remote callback attempts local input even while the common install timer is busy, subsequent callbacks do not spam retries, missing/changed local identity and ready/disabled states are handled. Component tests cover all six arrays, dictionary fallback with deleted entries, mismatched owner/world, invalid counts/capacity and invalid container class. Existing prediction, camera fallback, first-hit trigger, original forwarding, math/config, native allocation, six-page UI and GLES tests remain part of ./test.sh.

Host unsupported-architecture tests do NOT execute the ARM64 installer or prove scoped-weapon scheduling in the real engine. The live evidence above is v14. v15 is built in out/arm64-v8a/liblemmingrmt.so but is NOT automatically loaded into the running game. No phone restart, game restart, deployment, module reload or live-memory write was performed by the assistant. Until the user loads v15 and repeats a test, on-device success is unverified. Silent damage remains unresolved.

Backup: backups/20261001-130136-before-local-input-init-fix/source-release.tar.gz. The one-off scripts/maintenance/legacy/patch_input_init_v15.py was applied once; do not rerun it. Preserve historical captures/docs for comparison.

These cheating features violate the game's ToS and can lead to a ban.
