# atomix
Modeling stable electron wave states (WIP)

_I am under no illusions and make no claims that this represents completely accurate physics!_

My goal with this project is to tactically/viscerally understand de Broglie wavelength stuff, inspired by this wonderful youtube video: [https://youtu.be/k581_XpaTnU](https://youtu.be/k581_XpaTnU).

Originally intended functionality is that the user will be able to increase input energy and see the electron orbits appear, evolve, and stabilize from the first orbit outwards.

Warning: I may end up taking this in many different directions since the magical world of graphics code is my imagination's playground for testing, failing, and learning.

I sincerely doubt anyone will ever see this but me, but it's always good to communicate and CYA, innit?

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

![circle2](https://github.com/user-attachments/assets/c783cbc9-f63a-4fca-a612-0b4f85d1dac1)![sphere](https://github.com/user-attachments/assets/b9e53017-f075-4549-84c6-6dce2ea18f3f)


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

![harmonics](https://github.com/user-attachments/assets/08d7bc23-a29a-4e7d-91cf-55e6e0e5f50b)

The center (i.e. nucleus) model is a central hexahedral crystal with polar colouring. The probability cloud (sum of results to Schroedinger's equations for selected n,l,m states) is rendered around the center with coloring indicating the relative probability of the electron's position.

#### Options:
* May superpose any combination of n,l,m states
* May slice through vertically (theta), horizontally (phi), or radially (r)
* May adjust brightness of background
* May adjust resolution in two dimensions (warning: very high resoltions yield 2+GB buffer sizes. A warning is given if you exceed 1GB.)

 
## Development Platform and Possible Requirements:
Ubuntu 24.04 Noble
* gcc/g++ 13.2.0
* CMake 3.28.3

macOS 15 Sequoia
* Apple clang 16.0.0
* CMake 3.31.1

OpenGL 4.6 / Vulkan 1.3  
Qt 6.8.0+  <== [https://www.qt.io/download-open-source](https://www.qt.io/download-open-source)

## Personal notes and scribbles

I had this working just fine in OpenGL 4.6 using DSA until I decided to make it run on macOS which only supports OpenGL up to 4.1.
I lost a month to learning and re-making my rendering library in Vulkan.

MacOS also doesn't respect Qt's styling defaults, so that UI is still currently buggy.

#### TODO and Future Goals:
* Need to go back and add imaginary influence to spherical harmonics for accurate lobes
* May use this platform to explore gravity, the implications of the Planck length, and multidimensional rotations
