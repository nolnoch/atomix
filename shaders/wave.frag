#version 450 core

in float peakTrough;

out vec4 FragColour;

void main() {
   float clampColour = clamp(peakTrough, 0.0, 1.0);
   float squeezedPeak = pow(peakTrough, 2);
   float invColour = 1.0 - squeezedPeak;
   float base = 0.8;
   float partial = ((1 - base) * invColour) + base;


   if (peakTrough > 0)
      FragColour = vec4(invColour, partial, 1.0, 1.0f);
   else
      FragColour = vec4(1.0f, invColour, invColour, 1.0f);
};