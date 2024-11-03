#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 colour;

layout(location = 0) out vec4 vertColour;

layout(binding = 0) uniform UniformBuffer {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} ubo;

void main() {
    vertColour = vec4(colour, 0.1f);
    gl_Position = ubo.projMat * ubo.viewMat * ubo.worldMat * vec4(pos, 1.0f);
}