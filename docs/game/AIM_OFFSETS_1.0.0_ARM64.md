# Aim integration — Standoff 2 1.0.0 ARM64

Experimental integration; no live Android/device/game test and no guarantee of server hit registration.

## Inputs / provenance

Current dump.cs SHA256: `0c1c10058f7593ed8fd6fab830f35c2834ab9a35799052af4893c75e5ecdf651`.
Matching `../1.0.0 dump x64/extracted/libunity.so` is ELF AArch64 despite its folder name. SHA256: `a5d530e0ee7f4982d4bafec50c6109737b6119525935cac3e423e5145ff33e0f`.
The older first-offset document uses a different dump and was not used.

## Angles

| RVA | Method | Evidence |
| --- | --- | --- |
| 0xA0903DC | ios::iqkr(ExecuteTime) | Dump declaration and matching native tick |
| 0xA08C2B4 | ios::kjuc(Vector2) | Pitch/yaw clamp, s0/s1 input and HFA output |
| 0xA08C460 | ios::iwlo(Vector2) | Writes full quaternion to eir+0x10 and yaw-only quaternion to eir+0x20 |
| 0xA08C4F8 | ios::jqcv() | Applies pitch/yaw transforms and copies state to yyl |

ios owner +0x18, world +0x20, rotation +0x38, yyl +0x48, pitch/yaw transforms +0x50/+0x58. ExecuteTime is two unboxed floats. The vtable hook calls original first and uses game clamp/apply/refresh only for an alive local player. No guessed Euler fields.

## Silent

jxy::vrsg at 0x973A818 allocates a stack span of 24-byte rays, calls the weapon to generate them, emits the shot and then casts those rays.

jxy::rxjx at 0x9739FA4 EMITS shot data: it is NOT a ray generator. Early research used that label; binary inspection corrected it. Call site 0x973A9E0 contains `0x97fffd71` (BL rxjx). ABI: x0 handler, x1 unboxed WeaponContext pointer, x2 Ray buffer, x3 packed count/padding. No consumed MethodInfo argument.

Silent rewrites the stack-span directions BEFORE forwarding original rxjx exactly once, so event and subsequent casts share them. No camera, quaternion, HitTransform or movement writes. Owner ID, current world, active weapon and health are checked. Spans outside the current thread stack, over 64 elements, disabled modes and missing targets pass through unchanged.

One aligned BL is atomically redirected via an RX relay within +/-128 MiB. Non-fixed mmap hints cannot overwrite mappings. No prologue overwrite or general relocator. Expected instruction and helper prologues must match. Hook installation remains device-untested and may fail; such failure leaves the mode inactive.

Unboxed context: Owner +0x00, HitTransform +0x10, ActiveWeapon +0x48, WeaponState +0x50. nrk embeds context at +0x30. WeaponState raw Attack=1, confirmed by matching static field order/cctor. Modifiers are separate.

owv::jtka at 0x944B550 is a MOVEMENT vector, not a shot direction, and is deliberately not used.

## Hitboxes / points

Model part jgi+0x18 -> HitBoxCollider array; bone +0x20, Collider +0x28, cached native pointer +0x10. ColliderGetBounds at 0x538B650 is `void(nativeCollider*, Bounds*)`: 24-byte center/extents, native vtable +0xF0 with x8 indirect return. Only validated box/capsule/sphere colliders are read on UnityMain after original player update. Bounds must be finite, reasonably sized, within 4 m of the player's current pose, same world and under 100 ms old. No synthetic fallback.

Parts: Head, Neck, Chest/UpperChest, Stomach/Spine, Pelvis/Hips, Arms, Legs, Feet. Numeric bone mapping uses standard Unity HumanBodyBones constants; dump lists names but omits enum numbers, so mapping has NOT been independently verified against live metadata.

Points: center and +/-X/Y/Z using min(AABB extents)*scale <=0.9. They stay within the world AABB; this does NOT guarantee every sample lies inside an arbitrarily rotated collider's exact shape. Per-mode body bitmask, FOV, range, priority, lock and scale. Empty mask means no target. Normal smoothing is exponential with shortest-path yaw.

## Limits and testing

No visibility raycast, penetration scan, recoil compensation, auto-fire, hitchance, anti-cheat bypass or config persistence. Rage/Config placeholders remain unimplemented. Both modes default OFF and pause in the open menu. Disabling a mode makes its installed hook pass through; hooks remain installed until unload.

Host math/UI/memory mocks, ELF signature checks and ARM64 compilation do not validate live hook installation, shot acceptance or gameplay. See diagnostics/archive/loose/aim-*.log, diagnostics/tests/ and tests/aim/aim_test.cpp for test details.

This changes game behavior, violates game ToS and can lead to a ban.
