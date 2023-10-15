#version 450 core

layout(location = 0) in vec3 wavePos;
layout(location = 1) in vec3 yFactors;

uniform float time;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   float y_coord = yFactors.x * sin(yFactors.y - (yFactors.z * time));
   gl_Position = projMat * viewMat * worldMat * vec4(wavePos.x, y_coord, wavePos.z, 1.0f);
};