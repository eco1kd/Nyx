# v16 — command-phase automation and live-hitbox Silent

Profile marker: `v16-command-phase-hitbox`. Target remains the verified Standoff 2 1.0.0 ARM64 dump. Offline changes are not a claim of verified game behavior or server damage. Cheats violate the game's Terms of Service and may lead to an account ban.

## Confirmed causes and limits

The previous v15 local initialization retried correctly but never found the assumed `uys` component on the recorded device. This release removes that backend dependency. The native `ltc` early tick gets the actual `owv` command and calls its gate before the weapon consumer. That gate immediately rejects Attack=0. Therefore a callback only after the gate cannot bootstrap idle Triggerbot.

Silent-only visibility preparation was omitted in v15. Its unrestricted lead could also move a cached head point outside the head volume. These are confirmed source defects; they are not proven to be the sole cause of the user's near-head or no-damage observations.

## Verified native command phases

- `ltc::iqkr` at 0x94A3028; gate `ltc::nhmy` at 0x94A0990.
- Early callsite 0x94A3354, original word 0x97FFF58F; adjacent self/command setup 0xAA1303E0 / 0xAA1403E1 and original result branch 0x360000C0.
- If the original gate accepts, the game invokes `mnvr` at 0x948B758 using its own real ExecuteTime. The module does not synthesize that time or force the gate's result.
- Late tick 0x94A681C separately consumes WeaponAction, then calls `land` at 0x949ECD8 through 0x94A6A1C (0x97FFE0AF). Cleanup is after that original call.
- `ltc` is a component extending `chx`, not a part. Only UnityMain, exact class, inherited local flag, current local owner/world, local weapon context and actual supplied `owv` may drive automation.
- The uint field at `xkg`/`owv` +0x10 is used as an uninterpreted command identity token. A token mismatch forbids restoring stale command bytes. This is not a full garbage-collection/lifetime proof.

Both relays are prepared and originals published before the paired text transaction. Exact native words, readback, RX restoration and paired rollback are checked. If rollback is uncertain, relay memory is retained and automation never becomes ready. Other threads and foreign owners always pass through the original behavior.

## Command and camera cleanup

Module-owned Attack/Scope/ScopeHold survive the late action consumer, then restore their previous values. Physical holds are preserved. The original 12-byte nullable Look value is also restored; a recycled/token-changed command is not rewritten. Original gate/end functions are forwarded once, and the gate result remains unchanged.

Back camera no longer silently requires the Angles toggle. Normal aim's short ownership fallback is preserved. Menu, disable, manual shoot/scope input, invalid owner/world, death, missing target, weapon changes and timeout still cancel via the existing state machine. Camera input deltas during a snap are not separately integrated into the saved return view.

Auto scope is optional: unresolved enum literals or an unsupported weapon disable only that scope step, not Triggerbot/Back camera. The GUI states when scope is unavailable. No guessed enum values, cooldown/ammo bypass, anti-cheat bypass or forged network hit/damage requests were added.

## Silent point policy

- The selected volume is retained with the selection; the selected bone is located again in the live player's own hitbox array at emission.
- Bounds/native method/class/bone/root distance are validated again. `HitBoxCollider` +0x30 is not assumed to be a direct player owner.
- Head is center-first, rather than choosing a head edge simply because it is nearer the crosshair. Other body multipoints retain their relative offset with conservative bounds.
- Optional extra motion lead has no second compensation for old snapshot age. It is clamped to 35% of the smallest current hitbox extent. A shifted point must raycast to the exact selected collider or fall back to the live center.
- With Silent visibility enabled, a clear ray or a hit on another bone is not accepted. With it disabled, the center fallback does not silently become a mandatory wall check.
- One point is resolved per ray batch. Each valid ray keeps its own origin and gets a normalized direction, with refreshed FOV/range checks. Originals are still forwarded once when validation fails.

A fixed client ray is not proof that the server accepts a headshot or damage. Prediction does not guess ping or bullet speed. Magazine +0x52 remains a byte; AimState +0x8C remains unchanged.

## Diagnostics and regression tests

The eight-line heartbeat is retained. New fields: `phaseEnds`, `gateRejects`, `safety`, `scopeReady`, `silentPoint[bone,leadCm,live,centerFallback,rejected]`.

Safety mask: identity=01, readable command/weapon data=02, supported gun=04, ammo=08, visibility runtime=10, scope enum resolved=20. Scope bit is optional; it must not block the other modes.

New tests cover paired install faults/rollback, command phase ownership and forwarding, scope/Look cleanup, physical holds, token changes, live selected bone refresh, large/NaN lead, exact collider checks, stale/native/bone rejection, FOV/range and 360 coverage. Existing aim, assists, automation, input retry, prediction, debug and six-viewport UI/GLES tests remain in `./test.sh all`. Host tests do not execute the game's ARM64 methods.

## Device validation still required

The assistant does not deploy, restart the game, reload or inject the module. Initial and final device inspections for this fix found no ADB device. Offline ARM64 build and the full host/UI/GLES test suite passed; logs and final QA records are under the current consumer-silent diagnostics folder.

For a controlled follow-up after the user loads this build: verify the v16 heartbeat, input install reason, increasing input/phaseEnds, gateRejects and requests/observedShots; then test Triggerbot alone, Back camera, and finally supported Auto scope. For Silent baseline, select Head only, set extra prediction to 0 ms, and compare logged bone=10/leadCm with actual outcomes before enabling additional bodies or lead. Observed shots remain distinct from damage.

Backup: `backups/20261001-134028-before-consumer-silent-fix/source-release.tar.gz`. It contains the pre-v16 source, tests, scripts, docs, README and v15 output. Consult the existing rollback guide before restoration; do not overwrite files containing new user work.
