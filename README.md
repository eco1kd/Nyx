# Nyx — Android Native Overlay Framework

A research-oriented native overlay framework for Android, written in C++17
with OpenGL ES 3 rendering via EGL. Designed for studying Android native
development, dynamic instrumentation, and IL2CPP runtime introspection.

> **Educational / research project.** Not intended for distribution,
> production use, or any application that violates third-party terms of
> service. The author assumes no responsibility for misuse.

---

## Overview

Nyx is a modular native library (`.so`) that attaches to a running Android
application and renders a Dear ImGui overlay inside the application's own
OpenGL ES context. The framework demonstrates:

- **EGL frame interception** via GOT/PLT relocation patching
- **JNI input bridge** via `RegisterNatives` on Java-side native methods
- **Vtable hooking** of managed methods in Unity/IL2CPP runtimes
- **Safe cross-process memory reads** (`process_vm_readv` with fallback)
- **Dear ImGui rendering** inside a foreign GLES3 context
- **Touch input mapping** between physical screen and framebuffer space
- **IME integration** through a hidden `EditText` + `InputMethodManager`

The runtime is **ARM64-only** and targeted at applications built with
Unity/IL2CPP.

---

## Architecture

```
jni/
├── src/
│   ├── core/entry.cpp          Bootstrap, EGL hooks, JNI bridge, IME
│   ├── game/offsets.hpp        Version-specific IL2CPP symbol map
│   ├── features/
│   │   ├── visuals/esp_runtime.hpp   World-to-screen projection module
│   │   └── aim/                      Target selection & math utilities
│   └── ui/premium.hpp          ImGui overlay UI
├── Android.mk
└── Application.mk
scripts/
├── build.sh                    NDK build wrapper
└── debug/                      ADB log capture helpers
docs/                           Design notes and offset references
```

---

## Building

### Requirements

- Android NDK r30 (or newer)
- CMake / GNU make
- Target: `arm64-v8a`, minimum API 23

### Build

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk-r30
./build.sh
```

Output: `out/arm64-v8a/liblemmingrmt.so`

---

## Runtime Components

### 1. Frame Interception

The library patches EGL entry points in the target process's GOT via
`dl_iterate_phdr`, allowing the overlay to draw immediately before
`eglSwapBuffers`. Frames are rendered into the same EGL context used by
the host application.

### 2. Input Bridge

Touch events are captured through a JNI hook on
`UnityPlayer.nativeInjectEvent`. Coordinates are rescaled from physical
screen space to the current framebuffer size (which may differ due to
adaptive resolution). Events consumed by the overlay are not forwarded
to the host.

### 3. IL2CPP Introspection

Managed classes and methods are resolved at runtime through the IL2CPP
API (`il2cpp_domain_get`, `il2cpp_class_from_name`, …). Metadata is
read only — the framework does not modify managed state.

### 4. UI Layer

A fully custom Dear ImGui interface with:

- Section-based navigation (General / Aim / Visuals / Misc / Config / Settings)
- Animated transitions, sliding indicators, backdrop dimming
- Watermark overlay with FPS telemetry
- Optional login screen with guest access
- Window scaling, locking, and repositioning

---

## Versioning

Symbol addresses and field offsets are version-specific. The current
profile targets a single known build (`kLibUnityBuildId` in
`jni/src/game/offsets.hpp`). For a different build, regenerate offsets
with a compatible IL2CPP dumper and update `offsets.hpp`.

---

## Limitations

- x86 and 32-bit ARM are not supported
- Managed GC may invalidate cached object references — all reads use
  guarded, bounds-checked helpers with automatic invalidation
- Adaptive resolution requires per-frame rescaling of projection input
- SELinux and system integrity checks may prevent loading on hardened
  devices

---

## Roadmap

- [ ] Config persistence (save / load presets)
- [ ] Restructure `entry.cpp` into smaller translation units
- [ ] Offline math/UI test suite
- [ ] Bone hierarchy traversal utilities

---

## License

MIT — see LICENSE for details.

---

## Disclaimer

This project is provided for educational and research purposes only.
It exists to demonstrate native Android development techniques such as
EGL interception, JNI bridging, and safe cross-process memory access.
Use of this software to modify or interact with third-party applications
may violate their terms of service; the author does not endorse or
support such use.
