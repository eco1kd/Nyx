# Angles assists — v12

Current profile: `Standoff2-1.0.0-arm64-il2cppdump-v12-angle-assists`.

## Options and preserved behavior

Aim → Angles, right-hand card: Show FOV, FOV color/thickness, Visible check, RCS, RCS strength (0–100%). Body masks remain under Targeting and Multipoints. The three new switches default OFF, preserving the v11 behavior until explicitly enabled. RCS requires Angles enabled and an active attack; it also has a delta-compensation path when no eligible target exists. Menu pause, local-player/context checks, original camera call, target freshness/team/world/health guards, and the v11 hook installers remain in place. Silent's handler and installers are unchanged; no claimed fix for its missing damage.

## Verified recoil provenance

Dump 1.0.0 ARM64: WeaponContext TypeDef 19198, RecoilContext 19191, RecoilState 19192. Boxed value-type field offsets include a 16-byte header and must be converted to physical offsets.

- `nrk` embeds unboxed WeaponContext at 0x30.
- RecoilContext multiplier: context +0x40 → nrk +0x70.
- RecoilState: context +0x68; ActualPoint +0x10 → nrk +0xA8.
- Native shot builder 0x9736F2C: at 0x9736FD8, `2d4f26a8` loads ActualPoint x/y from context +0x78.
- Native multiplier helper 0x972976C: at 0x97297B0, `bd404261` loads RecoilMult from context +0x40 and multiplies both components.
- Native rotation helper 0x97297E4 rotates forward by +point.y about Transform.up, then -point.x about Transform.right, using AngleAxis in degrees. Getter identities: forward 0x67F967C, right 0x67FB408, up 0x6809B90; Internal_AngleAxis 0x67E50D0.

Thus the first-order camera pitch/yaw recoil is (-x*multiplier, +y*multiplier); compensation uses the opposite signs. The component limits (30 degrees) and multiplier limits (0–10) reject implausible/nonfinite data. Strength is bounded, and the standalone path tracks the weighted recoil delta rather than repeatedly subtracting the full accumulated value. Weapon/menu/attack resets are explicit. Target smoothing is applied to the recoil-unbiased aim direction, followed by instantaneous strength-weighted compensation, so smoothing does not attenuate RCS itself. This is camera-angle compensation, not recoil-state mutation, spread removal, server hit fabrication, or a guarantee of perfect cancellation at extreme pitch/compound rotations. The native signatures are independently checked: a failed optional profile must not disable basic Angles with the switches off.

## Verified visibility ABI and policy

- PhysicsScene is 8 bytes (two int32 fields). Native default-scene getter 0x904D9C4 is exactly `mov x0,xzr; ret` in this ELF. Zero is verified for this build, not an invented handle.
- Native Raycast wrapper 0x538BD0C dereferences an 8-byte scene pointer, calls 0x557E1C4, and returns a boolean. Arguments: x0 scene*, x1 Ray*, x2 RaycastHit*, s0 distance, w3 layer mask, w4 QueryTriggerInteraction.
- Ray is 24 bytes. RaycastHit is **44 bytes**, with distance at +0x1c and **int32** collider entity ID at +0x28. Compile-time assertions cover these layouts.
- UnityEngine.Object.GetEntityId is 0x67F0658. Target-owned IDs are obtained through the verified function, not an invented managed m_InstanceID field. Capture records valid hitbox IDs and an optional PlayerBindings capsule/controller ID.

Queries run only on UnityMain. Each eligible point is checked; an occluded nearest candidate does not automatically hide another visible point/actor. First contact with a target-owned collider is accepted, a different collider blocks, and no contact to the endpoint means an unobstructed segment. Missing optional profile, missing owned IDs, invalid outputs, wrong thread, or the 64-query selection budget exhaustion fail closed. Layer mask -5 excludes IgnoreRaycast; trigger mode Collide includes hitboxes. This is a conservative first-collider line-of-sight policy, not mesh transparency/penetration or verification of the game's server hit rules. Custom map trigger/layer behavior and phone performance still need live testing.

## FOV overlay

The angular selection value is the cone half-angle. Radius derives from actual cached perspective projection: width/2 × m00 × tan(angle), height/2 × m11 × tan(angle); projection shift is respected. This avoids a fixed degree-to-pixel guess and adapts to viewport, aspect and zoom. A full-front-screen cone (90–120 degrees) or an ellipse covering all corners is represented by the viewport border instead of a negative/nonfinite tan radius. A stale/unavailable projection produces no fabricated ring. The draw runs independently of ESP and before the closed-menu return; it performs no Unity physics/transform calls on the renderer. Weapon-origin/camera-origin parallax is not eliminated.

## Diagnostics and testing

The `AIM DEBUG` heartbeat includes profile v12 and a seventh line with RCS readiness/reads/rejected/applied, current recoil axes, and visibility readiness/rays/clear/owned-target/blocked/unknown. Signature readiness is not proof of live feature correctness or damage. Application counters are not hits.

Host tests cover projection geometry, recoil signs/strength/deltas/resets, per-point visibility fallback, the exact native layouts, mocked first-contact ownership, thread guards and query budgets; UI tests exercise the new independent options and all four Aim pages at six resolutions. Additional GLES tests exercise the FOV ellipse and full-viewport draw helper with the menu closed and ESP disabled using a synthetic projection. Android ARM64 build and host/UI test outcomes belong in the feature session QA record; none alone proves phone behavior.

## Live test without automatic reload

Load the new output yourself. First verify the heartbeat says `v12-angle-assists`. Test Show FOV with ESP disabled and the menu closed; separately test RCS while firing and then Visible check against an open target and a wall. Use `./aim_debug.sh capture 45` for logs. No game restart, injection/reload, or live process memory write is performed by the assistant during this source/build task. Silent ray redirection is confirmed in the preceding v11 logs, but absent damage remains unexplained.

All addresses are ELF virtual RVAs, not raw file offsets. Original dump/native hashes and version-specific context remain in AIM_OFFSETS_1.0.0_ARM64.md.

Using this module violates game ToS and may lead to an account ban.
