# v11 Aim hook installation fixes

## Live v10 evidence

Captured 2026-10-01 around 02:04 EEST, with the game running. The diagnostic v10 profile and native signatures were valid; player callbacks and hitbox captures were active. Both Angles and Silent hook invocation counts remained zero. The menu was closed during both test intervals.

Angles: the exact camera Tick address was present at virtual slot 17, but the old installer rejected it because the adjacent metadata did not match a plaintext MethodInfo interpretation (its first observed word was 17). This is an installer rejection, not a FOV/local-player failure.

Silent: repeated near-relay allocation failures, including errno=0. The old code tried non-fixed hints only +/-64 MiB around the callsite and discarded allocations returned outside BL reach. A failure here prevents emitRays and selection from running at all. The exact live free-gap layout was not read after ADB disconnected; zero candidates or permission failures are still possible on a new live test.

The saved tagged log is under diagnostics/aim-debug/live-20261001-020443/. No buffer clear, game restart, injection, root operation or process-memory write was performed by this diagnostic session.

## Code changes

- Angles matches the exact, signature-validated native pointer in the verified ios class. It retains text bounds, readable metadata/entry, writable slot, write probe and post-write verification. It does not decode the incompatible adjacent metadata as MethodInfo function pointers.
- Silent searches actual free gaps from its own /proc/self/maps across the full +/-128 MiB BL window. Page alignment supports 4K/16K/64K. MAP_FIXED_NOREPLACE avoids replacing live mappings; MAP_FIXED is never used. Returned addresses are checked even on older kernels. Occupied/racing gaps are rejected.
- Branch limits/alignment/opcodes are checked before a patch, and the original callsite is checked again immediately before writing.
- Relay search reports candidate/attempt/relocated counts, page size and last errno. New installation errors distinguish profile validation and slot verification failures.
- Targeting math, body masks, multipoints, world/team/local/alive/attack/menu gates, camera/ray handlers and original-call passthrough are unchanged. No guessed offsets, FOV circle, anti-cheat bypass or new gameplay feature was added.

## Verification and next live test

Run ./build.sh and ./test.sh. The new standalone hook-support test reproduces the metadata mismatch, branch boundaries, free gaps beyond the old hint range, page sizes and a real non-clobbering near allocation on the host. Host tests do NOT prove Android hook installation, target tracking, server acceptance or hit registration.

The new heartbeat identifies build=v11-aim-hook-fix. Load the new out/arm64-v8a/liblemmingrmt.so through your usual workflow, reconnect ADB, then test modes separately with the menu closed. For Angles, temporarily turn Only while attacking OFF; for Silent, actually fire. Keep Head/Chest selected and use a moderate FOV with a visible nearby target. Capture with ./aim_debug.sh capture 45. First check hook readiness/invocation counts, then local/selection/application counters. Do not interpret local mask=000 as a root cause while both handlers are not called.

Phone deployment/restart was not performed. ADB disappeared after the initial log read. Live v11 behavior remains unverified.

Using the game-modifying module violates the game's ToS and may result in a ban.
