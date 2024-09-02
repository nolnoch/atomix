# atomix
Modeling stable electron wave states as a factor of input energy (WIP)

_I am under no illusions and make no claims that this represents completely accurate physics!_

My goal with this project is to tactically/viscerally understand de Broglie wavelength stuff, inspired by this wonderful youtube video: [https://youtu.be/k581_XpaTnU](https://youtu.be/k581_XpaTnU).

Intended functionality is that, beginning with 0 input energy and 0 electron waves, the user will be able to increase input energy and see the electron orbits appear, evolve, and stabilize from the first orbit outwards.

This is still a work in progress as I'm slowly learning Qt and relearning OpenGL after 10 years of only dealing with ML code and corporate scripting. I'll flesh this out more another time.

Warning: I may end up taking this in many different directions since the magical world of graphics code is my imagination's playground for testing, failing, and learning.

I sincerely doubt anyone will ever see this but me, but it's always good to communicate and CYA, innit?

## Current State

#### Description and Features
As of 27 Aug 2024:

The model is a central hexahedral crystal with polar colouring, orbited by circular transverse waves, the wavefunction being either parallel or orthogonal to the equatorial plane (both are options). At the moment, colouring is inconsistent and potentially garish, but that's WIP.

I've now moved beyond the CLI controls and implemented them in the GUI as a dockable sidebar. All basic configurations are supported through GPU- and CPU-based rendering, however CPU-based render is necessary for turning on superposition at the moment because it requires knowledge of adjacent waves, which is antithetical to traditional GPU stream processing. I'm aware that a compute shader might solve this, but I don't want to shuffle buffers back and forth. I'll see if there's a better way.

In any case, both orthogonal and parallel (coplanar) waves can be rendered on either processor, allowing you to use shaders if desired (standard options are pre-loaded; custom shaders may be specified in config). Superposition is only relevant to coplanar waves, in which case it does work with a naive and fragile implementation (amplitude < 1.0f).

* Pre-define a configuration using the config.wave files (feel free to create your own!) and/or use the GUI options to adjust model parameters such as:
  * Number of orbit-waves (for *n*>=1, *n* waves where wave[*n*] has r = *n*)
  * Wave amplitude, period, and wavelength (in radians)
  * Wave resolution (in raw \# of vertices; suggest >360)
  * Custom vertex shader file selection (there are two; you may specify any)
  * CPU vs GPU rendering
  * Parallel-transverse (coplanar) or orthogonal-transverse wave orientation for circles
  * Spherical harmonic waves
* Mouse and keyboard controls
  * Left-click-drag translates the model up/down/left/right in the window
  * Right-click-drag rotates the model in 3-space
  * Middle-click-drag rotates the model in the style of Eulerian "roll" (for minor adjustments)
  * "Home" key resets the model to starting state in case you're upside down
  * "Spacebar" will seamlessly pause and unpause time in the model's world
  * "Escape" will exit the program
* Granularity options
  * Selecting which waves are rendered (n = {1, 2, 3, ...})
  * Selecting colors for base, peak, and trough
 
#### Development Platform and Possible Requirements:
- Ubuntu 24.04 Noble
- gcc/g++ 13.2.0+
- CMake 3.28.3+
- OpenGL 4.6
- Qt 6.5.3+  <== [https://www.qt.io/download-open-source](https://www.qt.io/download-open-source)

## Personal notes and scribbles

Subtituting real-world values for k = 2pi/l = p/h = E/hc would not be graphically interesting, and I would learn nothing.

Similarly, phase values of wt+p = t/f+p = ht/E+p (?) seem to lie in the complex plane, and I do not hate myself that much, yet. (Update: I think I will actually tackle this.)

#### //TODO List
* ~~Can I make a spherical plane-wave? It's just math, right?~~ Update: I did it!
* I can use fake constants to simulate input energy to the threshold of stable wave orbits and still complete my original vision for this project.
* I'm not sure whether to compare any of this to the Bohr-Sommerfeld model. I may revisit that in the future.
* As tin-foil or ignorant as this may sound, I want to simulate a photon/em wave traveling through 4-space whose electrical and magnetic waves orthogonally project in visible 3-space. It *feels* like the math should work, at least.
* Fix the colour options
* ~~Add config to GUI text readout~~ Update: Done
* ~~Add controls to GUI text readout?~~ Update: Done
* ~~File pickers for shaders and configs to avoid CLI~~ Update: Done-ish...it automatically loads present files. Need to add path selector.
