#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

out vec3 vertColour;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
    vertColour = factorsA;
    gl_Position = projMat * viewMat * worldMat * vec4(factorsB, 1.0f);
};