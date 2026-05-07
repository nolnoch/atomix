# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`atomix` is a Qt + Vulkan desktop application that renders 3D standing waves and hydrogen-orbital probability clouds (spherical harmonics × radial wavefunctions). It is a single-binary tool — no test suite, no library, no separate frontend.

## Build System

CMake (≥ 3.30) + vcpkg + Qt 6.8+ (Linux/Windows) or 6.9+ (macOS). C++23, requires Vulkan 1.3.

`VCPKG_ROOT` must point at a vcpkg checkout (the Linux preset defaults it to `${sourceParentDir}/vcpkg`). `vcpkg.json` pins dependencies (glm, glslang, spirv-tools/cross/reflect, tbb, vulkan).

`src/CMakeLists.txt` hardcodes per-platform locations that you may need to retarget:
- `Qt6_DIR` (Linux: `/home/braer/Qt/6.8.0/gcc_64`, macOS: `/usr/local/Qt-6.9.0`, Windows: `C:/Qt/6.8.1/msvc2022_64`)
- `oneDPL_DIR` on Linux (`/opt/intel/oneapi/2025.1/lib/cmake/oneDPL`)

`CMakePresets.json` includes a per-host file (`CMake${hostSystemName}Presets.json`) and exposes hidden presets that `CMakeUserPresets.json` extends with the `*A` suffix (which is what you actually invoke):

```bash
# Configure + build (Release is the intended default)
cmake --preset defaultConfigReleaseA
cmake --build --preset defaultBuildReleaseA

# Debug
cmake --preset defaultConfigDebugA
cmake --build --preset defaultBuildDebugA

# Build + install + run platform deploy script
cmake --build --preset defaultInstallA
```

`install` triggers `doc/deploy/{linux,mac,win}/<plat>deploy.{sh,bat}`, which assembles an AppImage / .app / Windows installer. The Linux deploy script references `~/bin/appimagetool` and a hardcoded VulkanSDK path — expect to edit it before running on a new machine.

`atomix_clean` is a custom target that runs the platform `*clean` script.

## Running

```bash
./atomix [--verbose] [-d <atomix-dir>] [-p|--profiling] [-t|--testing] [-r|--reset-geometry]
```

The binary needs an "atomix files directory" containing both `configs/` and `shaders/`. Resolution order (`src/main.cpp`):
1. `-d/--atomix-dir` CLI flag,
2. `QSettings` group `atomixFiles/root` from a previous run,
3. `<cwd>/../Resources` (macOS) or `<cwd>/../usr` (other) — matches the deploy layout, not the build tree.

If none resolve, the app pops a folder picker. When developing from `build/`, pass `-d <repo-root>`.

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
- Per `README.md` and `src/TODOList.txt`, the macOS Qt+Vulkan freeze on focus change is a known unresolved issue; commit `e6613e6` partially handles it.
