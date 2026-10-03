# Aim automation — v13, Standoff 2 1.0.0 ARM64

## Controls

Aim → Angles now tracks without waiting for attack: Only while attacking defaults OFF but remains optional. Existing smoothing, RCS, visibility, body masks, multipoints and team/world/health/freshness guards remain.

FOV sliders show full coverage 1–360 degrees. Internally Settings.fov remains a cone half-angle, now capped at 180. Old internal values are preserved: old half-angle 8 is displayed as coverage 16. Coverage 360 includes directly rear targets. The FOV overlay covers the viewport for half-angles ≥90; a rear hemisphere cannot be projected as a screen circle.

Aim → Automation: Triggerbot, trigger delay 0–300 ms, minimum cycle interval 30–1000 ms, timeout 200–2000 ms, Back camera and Auto scope. All automatic switches default OFF. Automatic fire always pauses while the menu is open, independent of the ordinary aim menu-pause setting.

Plain Triggerbot does not rotate the view. A real first-hit collider must belong to the selected live enemy; clear rays and obstacles are NOT hits. It uses normal targeting/body/range settings with a narrow candidate cone and a native ray as the final firing condition. Unknown weapons, empty magazines, reload/take/inspect, unknown scope states, unsupported profiles and non-main-thread calls fail closed.

Back camera requires Angles enabled plus Triggerbot. It saves the view once, snaps toward an eligible target, optionally waits for scope acknowledgement, requests normal attack, waits for a local native shot emission, and restores the view after the original emission returns. Set FOV coverage to 360 for rear targets. Continuous ordinary Angles tracking is suppressed while back-camera mode is enabled. Timeout, manual fire/scope, target loss, changed player/world/weapon or unsafe state abort the cycle. Restoration re-resolves the live camera instead of using a stale saved component.

Auto scope is only requested for the dumped GunWithScopeParameters definition, directly at qjf+0x60 or through GunDefinition.WeaponParameters+0x38. An initially open scope is preserved. Only a module-opened scope is closed afterward; cleanup waits across switch/reload transitions under a bounded 2-second lease. Normal scope animations, rate of fire, reload and cooldown are NOT bypassed. Other guns fire without opening a scope; knives and unknown definitions are rejected.

## Native provenance

Sources: reference/dumps/1.0.0/{dump.cs,il2cpp.h,script.json} and matching extracted AArch64 libunity.so. RVA checks use ELF PT_LOAD virtual-to-file translation or llvm-objdump, not raw-file indexing by RVA.

- mfc+0x20 → wmz+0x10 component array; bounded and verified by component name, owner chx+0x18 and world+0x20. NOT the parts-list layout.
- uys::iqkr(ExecuteTime) 0xAAF79D0; ExecuteTime is an 8-byte HFA. Persistent OWV is uys+0x40. Native raw mode at +0x48 is compared with 101; no semantic enum label is invented. Full tail 0xAAF7D40..0xAAF7EB4 refreshes/reuses/copies the command and returns. Hook calls original exactly once, then modifies only the local owned command. Temporary flags are restored before the next local original tick. Remote callbacks cannot release that lease.
- OWV Attack setter 0x944E2A8 → byte32; AlternativeAttack 0x945BB7C → byte33; AlternativeAttackContinuous 0x945B774 → byte34. Native store/return signatures are checked. ltc::itni 0x9497960 proves the WeaponAction mapping.
- Nullable view setter 0x94479F8: has byte, pad3, pitch float, yaw float; size12, ARM64 integer aggregate x1/w2, NOT Vec2 HFA. Full setter stores 12 bytes at OWV+0x1c. Only finite clamped angles are supplied.
- AimState byte at nrk+0x8c. Numeric llo values are NOT guessed. None/InScope/SwitchScope/ScopeReload are resolved from enum literal metadata on UnityMain via guarded field lookup/static literal-read APIs 0x6327AD0 and 0x6327C50; four distinct UInt4 values are required before input-hook readiness.
- Input installer matches an exact validated native pointer, publishes original before replacement, probes a writable slot, verifies readback and rolls back on failure. No guessed slot or comparison with encrypted adjacent metadata.
- Existing shot hook is shared with an observer even when Silent is OFF. Original Silent direction logic is preserved. Observer validates local owner/world/weapon/current ray span, acknowledges only one emission per cycle, and restores view only after original. Local emission is NOT damage or server acceptance.
- Collider hit is rechecked after applying aim/RCS before requesting fire. No forged hit requests, packet rewriting, ammo changes, direct weapon-state forcing or anti-cheat bypass.

## Debug and verification limits

Heartbeat build=v13-aim-automation. Automation telemetry separates input readiness, stage/reason, real ray checks, requested fire ticks, observed local shots, successful restores, scope pulses and timeouts. Neither requested nor observed shots prove damage.

Host tests: 360/rear selection, immediate-aim defaults, delay/cooldown/acknowledgement, scope preservation, cancellation/timeout/nonfinite clock, command ABI, own-flag release preserving physical holds, remote/wrong-thread passthrough and owned-hit versus blocked/clear/stale/team/nonfinite ray checks. UI covers all five Aim pages, new toggle clicks, six viewports, no scrollbars, finite vertices and offscreen GLES3. Native execution on the phone is NOT tested by these mocks.

No phone deployment/reload or game restart was performed. Actual input scheduling, dynamic metadata, supported definitions, scope transitions and server damage need an on-device test. The previous Silent damage problem is NOT claimed fixed.

## Rollback and risk

Backup: backups/20261001-111621-before-trigger-camera-return/source-release.tar.gz. The one-off scripts/maintenance/legacy/add_automation_v13.py has already been applied; do not rerun it. It does not reproduce subsequent verification refinements.

These cheat features violate the game's ToS and can lead to a ban.
