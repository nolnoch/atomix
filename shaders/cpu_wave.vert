#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 colour;

out vec3 vertColour;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   vertColour = colour;
   gl_Position = projMat * viewMat * worldMat * vec4(pos, 1.0f);
};