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

   float x_coord = r * cos_t;
   float y_coord = yFactors.x * sin(yFactors.y - (yFactors.z * time));
   float z_coord = r * sin_t;

   gl_Position = projMat * viewMat * worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
};