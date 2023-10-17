#version 450 core

layout(location = 0) in vec3 wavePos;
layout(location = 1) in vec3 yFactors;

uniform float time;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   float cos_t = wavePos.x;
   float sin_t = wavePos.z;
   float r = wavePos.y;

   float wave = yFactors.x * sin(yFactors.y - (yFactors.z * time));
   float x_coord = cos_t * (r + wave);
   float z_coord = sin_t * (r + wave);

   gl_Position = projMat * viewMat * worldMat * vec4(x_coord, 0.0f, z_coord, 1.0f);
};