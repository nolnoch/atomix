# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`atomix` is a Qt + Vulkan desktop application that renders 3D standing waves and hydrogen-orbital probability clouds (spherical harmonics × radial wavefunctions). It is a single-binary tool — no test suite, no library, no separate frontend.

## Build System

CMake (≥ 3.30) + Ninja + vcpkg + Qt 6.12. C++23. Vulkan instance version is whatever the loader reports (`mainwindow.cpp`, SPIR-V target picked to match).

Two environment variables drive everything; nothing per-machine is hardcoded:
- `VCPKG_ROOT`: a bootstrapped vcpkg checkout. `vcpkg.json` supplies glm, glslang, SPIRV-Tools, SPIRV-Cross, oneTBB and the Vulkan loader; overlay triplets (static, release-only) live in `cmake/triplets/`.
- `QT_ROOT_DIR`: the Qt kit (e.g. `~/Qt/6.12.0/gcc_64`). Prebuilt macOS Qt has no Vulkan, so on macOS build it with `packaging/macos/build-qt.sh <prefix>`.

oneDPL (header-only) is used from an installed package if `find_package` finds one, otherwise `src/CMakeLists.txt` downloads the release headers. On macOS, `ATOMIX_MOLTENVK_DIR` (presets read `$MOLTENVK_DIR`) points at `libMoltenVK.dylib`; the Vulkan loader and MoltenVK are bundled into the .app.

Presets (`CMakePresets.json`; `CMakeUserPresets.json` is no longer used):

```bash
# Development: Debug + Release in one tree (Ninja Multi-Config)
cmake --preset dev
cmake --build --preset dev-release        # or dev-debug
./build/dev/src/Release/atomix

# Release + package, as CI does it
cmake --preset linux-release && cmake --build --preset linux-release
packaging/linux/build-appimage.sh build/linux-release dist
```

Other release presets: `macos-arm64-release`, `macos-x64-release` (then `cmake --install build/<preset>` and `packaging/macos/package-dmg.sh build/<preset>/install dist/atomix.dmg`), and `windows-release` (from an x64 VS developer prompt; `cmake --install build/windows-release` gives a self-contained folder). Install layouts: macOS `atomix.app` (data in `Contents/Resources`), Windows flat folder, Linux FHS tree (`bin/`, `share/atomix/`) that linuxdeploy turns into an AppImage.

CI (`.github/workflows/build.yml`) builds all four packages on every push and PR and uploads them as artifacts. macOS DMGs are Developer ID signed and notarized when the signing secrets are present (listed at the top of the workflow). Pushing a `v*` tag also runs the `release` job, which attaches the four packages to a draft GitHub release. Release version comes from `project(VERSION)` in the top-level `CMakeLists.txt`. Actions caches are per-branch, so the first run on a new branch rebuilds Qt from source on both macOS runners (~30-60 min).

## Running

```bash
./atomix [--verbose] [-d <atomix-dir>] [-p|--profiling] [-t|--testing] [-r|--reset-geometry]
```

The binary needs an "atomix files directory" containing both `configs/` and `shaders/`. Resolution order (`src/main.cpp`):
1. `-d/--atomix-dir` CLI flag,
2. the packaged location relative to the executable: `../Resources` (macOS bundle), the exe's own folder (Windows), `../share/atomix` (Linux/AppImage),
3. `QSettings` group `atomixFiles/root` from a previous run,
4. `ATOMIX_DEV_DIR`, the source tree the binary was built from (so dev builds run from `build/` without `-d`).

If none resolve, the app pops a folder picker.

There is no test suite; `--testing`/`--profiling` set globals (`isTesting`, `isProfiling` in `global.hpp`) consumed ad hoc.

## Architecture

Single Qt application; rendering is Vulkan-only. The OpenGL backend (`programGL.*`) is left in the tree but is **not** in the executable's source list in `src/CMakeLists.txt` — treat it as historical.

Core layers:

- **`main.cpp` → `MainWindow`** (`mainwindow.{hpp,cpp}`) — Qt UI shell. Owns a `VKWindow` (the renderer) plus a dockable sidebar with two tabs: "Simple Waves" and "Spherical Harmonics". Persists window geometry and the atomix dir via `QSettings` (org `nolnoch`, app `atomix`).

- **`VKWindow` / `VKRenderer`** (`vkwindow.{hpp,cpp}`) — `QVulkanWindow` + `QVulkanWindowRenderer`. Drives the frame loop, mouse/keyboard interaction (`Quaternion`-based camera, see `quaternion.*`), pause, screen capture. Owns one `ProgramVK` and one active `Manager`.

- **`ProgramVK`** (`programVK.{hpp,cpp}`) — Vulkan pipeline/shader management. Compiles GLSL → SPIR-V at runtime via glslang, reflects with spirv-cross, builds pipelines, manages descriptor sets / UBOs / push constants. The header comment lists the required call sequence (`addShader → init → linkAndValidate → addSampler → enable/disable`) — keep it.

- **`Manager` hierarchy** (`manager.{hpp,cpp}`, `wavemanager.*`, `cloudmanager.*`) — produces vertex / index / data / colour buffers consumed by `ProgramVK`. Two concrete managers:
  - `WaveManager` — circular/spherical standing waves (Tab 0). Supports CPU and GPU paths; **superposition currently requires the CPU path** because it needs neighbouring waves (README explains the rationale for not using a compute shader).
  - `CloudManager` — Schrödinger orbital probability clouds (Tab 1). Heavy CPU compute over Legendre/Laguerre polynomials (see `special.*`); parallelised with oneDPL/TBB. Buffers can be very large — the UI warns above 1 GB.

  Both inherit a buffer-state `BitFlag` (see `global.hpp`) and dirty-update protocol; both speak `AtomixWaveConfig` / `AtomixCloudConfig` from `global.hpp`.

- **`FileHandler`** (`filehandler.{hpp,cpp}`) — Loads/saves JSON configs (`*.wave`, `*.cloud`) from `configs/`; `AtomixFiles` resolves the `shaders/`, `configs/`, `res/{fonts,icons}/` subpaths off the chosen root. `default.wave` is the bootstrap config. Cloud configs include a list of `recipes` (n/l/m/weight tuples) for superposed orbitals.

- **`special.*`** — Legendre / Laguerre polynomials. Apple Clang lacks `<cmath>` Special Math, so this file pulls the implementations directly to avoid the 4× slowdown of the Boost fallback (note `cloudmanager.hpp:38` for the rationale).

- **Shaders** (`shaders/`) — `gpu_circle.vert`, `gpu_sphere.vert`, `gpu_harmonics.vert`, plus `default.{vert,frag}`. Compiled at runtime; users may point at custom shaders via the config files.

Key cross-cutting types live in `global.hpp`: `AtomixWaveConfig`, `AtomixCloudConfig`, `harmap` (`map<int, vector<ivec3>>` of orbital recipes), `BitFlag`, the four `extern bool is*` debug globals, and physics constants. `Q_DECLARE_METATYPE` is used for queueing configs across Qt signal/slot boundaries.

## Conventions

- All headers use a uniform GPLv3 banner with author/date — preserve it on new files.
- Doxygen-style comments on `BitFlag` / `Manager` are the documented public API contract; keep them in sync with behaviour changes (recent commits are tagged `[Doc]`).
- macOS: `VKWindow` swallows `QEvent::Paint` and calls `requestUpdate()` instead, because rendering inside the CALayer display cycle deadlocks MoltenVK's `nextDrawable` (6d3893d, issue #9). Don't remove it.
- Known renderer and physics issues are tracked as GitHub issues #1-#8 and #10-#16.
