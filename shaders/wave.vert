#version 450 core

layout(location = 0) in vec3 wavePos;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   gl_Position = projMat * viewMat * worldMat * vec4(wavePos, 1.0f);
};