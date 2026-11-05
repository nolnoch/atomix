# atomix
Modeling stable electron wave states (WIP)

_I am under no illusions and make no claims that this represents completely accurate physics!_

My goal with this project is to tactically/viscerally understand de Broglie wavelength stuff, inspired by this wonderful youtube video: [https://youtu.be/k581_XpaTnU](https://youtu.be/k581_XpaTnU).

I sincerely doubt anyone will ever see this but me, but it's always good to communicate and CYA, innit?

## Download

Prebuilt packages are attached to each [release](https://github.com/nolnoch/atomix/releases). Every build on `master` also uploads them as artifacts of its [build workflow run](https://github.com/nolnoch/atomix/actions/workflows/build.yml).

| Platform | Package | Notes |
|---|---|---|
| Linux x86_64 | `atomix-<version>-x86_64.AppImage` | `chmod +x` it and run it. Needs a Vulkan driver (Mesa or your GPU vendor's) and glibc 2.39+ (Ubuntu 24.04, Fedora 40 or newer). |
| macOS, Apple silicon | `atomix-<version>-macos-arm64.dmg` | macOS 14.4 or newer. Open the DMG and drag atomix to Applications. |
| macOS, Intel | `atomix-<version>-macos-x64.dmg` | Same as above. |
| Windows x64 | `atomix-<version>-windows-x64.zip` | Unzip anywhere and run `atomix.exe`. The build isn't code-signed, so SmartScreen will warn the first time: choose "More info", then "Run anyway". |

Vulkan comes with every current GPU driver on Linux and Windows; on macOS it is bundled (MoltenVK).

## Current State
As of 3 Dec 2024:

#### Global program features and controls
* Now fully implemented in Vulkan (default) and multi-threaded (default)
* Verbose (--verbose) execution of binary for debug prints
* Folder selection for non-standard binary location relative to shader files
* Left-click-drag translates the model up/down/left/right in the window
* Right-click-drag rotates the model in 3-space
* Middle-click-drag rotates the model in the style of Eulerian "roll" (for minor adjustments)
* "Home" key resets the model to starting state in case you're upside down
* "Spacebar" will seamlessly pause and unpause time in the model's world
* "Escape" will exit the program
* "p" for Screen capture (currently buggy if alpha is used for the model and PNG is desired output)
* "d" for debug overlay (currently buggy since Qt does not treat Vulkan renderer as a native widget)

### Tab 0: Simple Waves

![waves](https://github.com/user-attachments/assets/6e769417-0f21-4dc9-8aab-291a6533ac43)
![sphere](https://github.com/user-attachments/assets/766c976b-120d-4e77-8229-da6c3a2958ea)

The center (i.e. nucleus) model is a central hexahedral crystal with polar colouring. It is surrounded by transverse standing waves, the wavefunction being either parallel or orthogonal to the equatorial plane (both are options) and expressed in circular or spherical format (both are options).

I've now moved beyond the CLI controls and implemented them in the GUI as a dockable sidebar. All basic configurations are supported through GPU- and CPU-based rendering, however CPU-based render is necessary for turning on superposition at the moment because it requires knowledge of adjacent waves, which is antithetical to traditional GPU stream processing. I'm aware that a compute shader might solve this, but I don't want to shuffle buffers back and forth. I'll see if there's a better way.

In any case, both orthogonal and parallel (coplanar) waves can be rendered on either processor, allowing you to use shaders if desired (standard options are pre-loaded; custom shaders may be specified in config). Superposition is only relevant to coplanar waves, in which case it does work with a naive and fragile implementation.

#### Options:
* Number of orbit-waves (for *n*>=1, *n* waves where wave[*n*] has r = *n*)
* Wave amplitude, period, and wavelength (in multiples of pi radians)
* Wave resolution (in raw \# of vertices; suggest >= 360)
* CPU or GPU rendering
* Parallel-transverse (coplanar) or orthogonal-transverse wave orientation for circles
* Spherical waves
* May select which waves are rendered (n = {1, 2, 3, ...})
* May select colors for base, peak, and trough
 
### Tab 1: Spherical Harmonics

![harmonics](https://github.com/user-attachments/assets/38c1b5c3-2f86-4fba-a46f-3701a851a368)

The center (i.e. nucleus) model is a central hexahedral crystal with polar colouring. The probability cloud (sum of results to Schroedinger's equations for selected n,l,m states) is rendered around the center with coloring indicating the relative probability of the electron's position.

#### Options:
* May superpose any combination of n,l,m states
* May slice through vertically (theta), horizontally (phi), or radially (r)
* May adjust brightness of background
* May adjust resolution in two dimensions (warning: very high resoltions yield 2+GB buffer sizes. A warning is given if you exceed 1GB.)

 
## Building from source

Requirements:
* CMake 3.30+ and Ninja
* A C++23 compiler: GCC 13+, Apple Clang 16+ (Xcode 16), or Visual Studio 2022 17.10+
* Qt 6.12 with Vulkan support. On Linux and Windows the [online installer](https://www.qt.io/download-open-source) or [aqtinstall](https://github.com/miurahr/aqtinstall) builds work. Qt's prebuilt macOS binaries are built without Vulkan, so on macOS build Qt with `packaging/macos/build-qt.sh <install-dir>` (30-60 minutes; builds only qtbase and qtdeclarative).
* [vcpkg](https://github.com/microsoft/vcpkg) (supplies glm, glslang, SPIRV-Tools, SPIRV-Cross, oneTBB and the Vulkan loader; see `vcpkg.json`)
* Linux only: X11/Wayland headers for vcpkg's Vulkan loader (Ubuntu: `sudo apt install libx11-dev libx11-xcb-dev libxcb1-dev libxrandr-dev libwayland-dev pkg-config`)
* macOS only: MoltenVK (from a [MoltenVK release](https://github.com/KhronosGroup/MoltenVK/releases), the Vulkan SDK, or Homebrew)

Point two environment variables at your installs, then use a CMake preset:

```sh
export VCPKG_ROOT=~/vcpkg               # vcpkg checkout (bootstrapped)
export QT_ROOT_DIR=~/Qt/6.12.0/gcc_64   # or C:/Qt/6.12.0/msvc2022_64, or your build-qt.sh prefix on macOS

# Development build (Debug + Release in one tree)
cmake --preset dev
cmake --build --preset dev-release
./build/dev/src/Release/atomix

# Release build + package, as CI does it (see .github/workflows/build.yml)
cmake --preset linux-release && cmake --build --preset linux-release
packaging/linux/build-appimage.sh build/linux-release dist
```

Other presets: `macos-arm64-release` / `macos-x64-release` (set `MOLTENVK_DIR` to the folder holding `libMoltenVK.dylib`; package with `packaging/macos/package-dmg.sh build/<preset>/install dist/atomix.dmg`) and `windows-release` (run from an x64 Visual Studio developer prompt; `cmake --install build/windows-release` produces a self-contained folder).

## Personal notes and scribbles

I had this working just fine in OpenGL 4.6 using DSA until I decided to make it run on macOS which only supports OpenGL up to 4.1.
I lost a month to learning and re-making my rendering library in Vulkan.

MacOS also doesn't respect Qt's styling defaults, so that UI is still currently buggy.

#### TODO and Future Goals:
* ~~Solve Vulkan freezing issue on MacOS~~ (fixed in 0.2.0: rendering inside Core Animation's display cycle deadlocked MoltenVK)
* May use this platform to explore gravity, the implications of the Planck length, and multidimensional rotations
