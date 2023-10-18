# atomix
Modeling stable electron wave states as a factor of input energy (WIP)

_I am under no illusions and make no claims that this represents accurate physics!_

My goal with this project is to tactically/viscerally understand de Broglie wavelength stuff, inspired by this wonderful youtube video: [https://youtu.be/k581_XpaTnU](https://youtu.be/k581_XpaTnU).

Intended functionality is that, beginning with 0 input energy and 0 electron waves, the user will be able to increase input energy and see the electron orbits appear, evolve, and stabilize from the first orbit outwards.

This is still a work in progress as I'm slowly learning Qt and relearning OpenGL after 10 years of only dealing with ML code and corporate scripting. I'll flesh this out more another time.

Warning: I may end up taking this in many different directions since the magical world of graphics code is my imagination's playground for testing, failing, and learning.

I sincerely doubt anyone will ever see this but me, but it's always good to communicate and CYA, innit?

## Current State

As of 17 Oct 2023:

The model is a central hexahedral crystal with polar colouring, orbited by circular transverse waves. There are two pre-loaded shaders for waves either parallel or orthogonal to the equatorial plane. A the moment, the peaks and troughs are coloured, but I plan to make that optional.
* Use the config.wave files (feel free to create your own!) to adjust model parameters such as:
  * Number of orbit-waves (for *n*>=1, *n* waves where wave[*n*] has r = *n*)
  * Wave amplitude, period, and wavelength (in radians)
  * Wave-circle resolution (in raw \# of vertices; suggest >360)
  * Custom vertex shader file selection (there are two; you may use any)
* Robust mouse and keyboard controls
  * Left-click-drag translates the model up/down/left/right in the window
  * Right-click-drag rotates the model in 3-space
  * Middle-click-drag rotates the model in the style of Eulerian "roll" (for minor adjustments)
  * "Home" key resets the model to starting state in case you're upside down
  * "Spacebar" will seamlessly pause and unpause time in the model's world
  * "Escape" will exit the program

## Personal notes and scribbles

Subtituting real-world values for k = 2pi/l = p/h = E/hc would not be graphically interesting, and I would learn nothing.

Similarly, phase values of wt+p = t/f+p = ht/E+p (?) seem to lie in the complex plane, and I do not hate myself that much, yet.

#### //TODO List
* Can I make a spherical plane-wave? It's just math, right?
* I can use fake constants to simulate input energy to the threshold of stable wave orbits and still complete my original vision for this project.
* I'm not sure whether to compare any of this to the Bohr-Sommerfeld model. I may revisit that in the future.
* As tin-foil or ignorant as this may sound, I want to simulate a photon/em wave traveling through 4-space whose electrical and magnetic waves orthogonally project in visible 3-space. It *feels* like the math should work, at least.
