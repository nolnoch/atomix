#version 450 core

layout(location = 0) in vec3 wavePos;
layout(location = 1) in vec3 yFactors;

out float peakTrough;

uniform float time;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   float cos_th = wavePos.x;
   float sin_th = wavePos.z;
   float r = wavePos.y;
   
   //                   (2pi / L * x) - (2pi / T * t)
   float normRange = sin(yFactors.y - (yFactors.z * time));

   //               A      *   wave
   float wave = yFactors.x * normRange;
   float x_coord = r * cos_th;
   float z_coord = r * sin_th;

   peakTrough = normRange;
   gl_Position = projMat * viewMat * worldMat * vec4(x_coord, wave, z_coord, 1.0f);
};