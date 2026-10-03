# v14 — motion prediction and automation fallback

## Confirmed code defect and correction

v13 cameraTick returned whenever Triggerbot + Back camera were enabled, even if the input hook, scope metadata, raycast profile, local command or cycle were unavailable. This could disable ordinary Angles indefinitely. This branch was found in the actual source, not inferred from phone logs.

v14 suppresses ordinary Angles only under a fresh camera lease, created after successful automatic aim application. The lease requires enabled Angles/Triggerbot/Back camera, ready input/shot/raycast profiles, menu closed, matching player/world and active saved view or observed/restored shot. Without a running input callback it expires after 80 ms. An observed shot extends a restored-view hold for at most 120 ms (bounded by the cycle interval), preventing continuous Angles from immediately undoing the return. Disabling automation, unsafe state, manual fire/scope or cancellation releases ownership. No checkbox-only early return remains. Diagnostics identify automation-camera-owner instead of silently returning with stale status.

Scope enum lookup is now optional for installing the plain input hook. If its metadata is unavailable, plain Triggerbot and ordinary Angles remain independent. Auto scope itself still fails closed without verified enum literals. Scope-close cleanup is marked sent only after the setter actually runs, rather than before writability is established.

This confirms and repairs a source defect. It does NOT establish every cause of the user's on-device symptom, input scheduling or scope behaviour; no phone is connected for this test.

## Built-in motion prediction

Aim → Prediction has independent Angle/Silent prediction switches and Extra prediction lead 0–100 ms. Both default ON with 20 ms extra lead. This does not enable either aim mode or automatic shooting.

Prediction estimates target velocity from already-existing UnityMain player-position snapshots with their actual captured timestamps. It does not invent game-memory offsets, native velocity fields, bullet speed, ping or network latency. Re-reading an identical snapshot does not zero the tracker. Position updates use a 45 ms velocity filter; stops and direction reversals respond immediately. Samples under 5 ms are accumulated rather than differentiated into noise. Identity/world/team changes, stale (>250 ms) or backwards time, nonfinite positions, displacement >3 m, or speed >30 m/s reset the estimate. A new/reused actor slot starts with no history.

Predicted point = selected present hitbox point + velocity × bounded horizon. The horizon includes snapshot age, configured extra lead, and up to 150 ms of smoothing compensation for Angles. Silent does not add camera smoothing delay. Horizon is capped at 250 ms and displacement at 1 m. Prediction falls back to the real selected point for stale (>100 ms), invalid or insufficient samples, excessive resulting range, or a blocked/unavailable visibility check. Body/FOV/priority selection remains present-time and retains the selected bone/point.

Triggerbot/Back camera continue to select present-time geometry and require a real native first-hit collider before firing; prediction never substitutes a hypothetical future hit for that test. This is bounded linear tracking-delay compensation, NOT an assertion of exact projectile interception or a fix for Silent server damage.

## Offline verification and rollback

Tests include velocity/duplicate snapshots, stopping/reversals, stale/teleport/world/team/NaN resets, lead caps, a constant-velocity smoothing comparison, exact-once original camera callback, all automation toggles with profiles unavailable/no lease, and live/stale camera lease ownership. UI tests cover six Aim pages, independent prediction toggle clicks and six viewport sizes without scrollbars. Synthetic tests are not a phone test.

Pre-change backup: backups/20261001-122936-before-prediction-automation-fix/source-release.tar.gz. The one-off scripts/maintenance/legacy/patch_prediction_v14.py has already been applied and does not reproduce subsequent verification refinements; do not rerun it. Output remains out/arm64-v8a/liblemmingrmt.so. No game restart, module reload, phone deployment or ADB request was performed in this change. Earlier v13 documentation describes historical implementation where it conflicts with this page.

These cheating features violate the game's ToS and can result in a ban.
