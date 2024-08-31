#version 450 core

in uvec3 vertColour;
in float wavefunc;

out vec4 FragColour;

void main() {
   uint base = vertColour.x;
   uint peak = vertColour.y;
   uint trgh = vertColour.z;

   uint mask = 0xFF;

   float baseA = (base & mask) / mask;
   float baseB = ((base >> 8) & mask) / mask;
   float baseG = ((base >> 16) & mask) / mask;
   float baseR = ((base >> 24) & mask) / mask;

   float peakA = (peak & mask) / mask;
   float peakB = ((peak >> 8) & mask) / mask;
   float peakG = ((peak >> 16) & mask) / mask;
   float peakR = ((peak >> 24) & mask) / mask;

   float trghA = (trgh & mask) / mask;
   float trghB = ((trgh >> 8) & mask) / mask;
   float trghG = ((trgh >> 16) & mask) / mask;
   float trghR = ((trgh >> 24) & mask) / mask;

   vec4 final = vec4(0.0f);
   float scale = abs(wavefunc);

   if (wavefunc >= 0) {
      final.r = (scale * peakR) + ((1 - scale) * baseR);
      final.g = (scale * peakG) + ((1 - scale) * baseG);
      final.b = (scale * peakB) + ((1 - scale) * baseB);
      final.a = (scale * peakA) + ((1 - scale) * baseA);
   } else {
      final.r = (scale * trghR) + ((1 - scale) * baseR);
      final.g = (scale * trghG) + ((1 - scale) * baseG);
      final.b = (scale * trghB) + ((1 - scale) * baseB);
      final.a = (scale * trghA) + ((1 - scale) * baseA);
   }

   FragColour = vec4(final);
};