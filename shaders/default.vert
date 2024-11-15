#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 colour;
    
layout(location = 0) out vec4 vertColour;

layout(set = 0, binding = 0) uniform WorldState {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} worldState;
    
void main() {
    vertColour = vec4(colour, 1.0f);
    gl_Position = worldState.projMat * worldState.viewMat * worldState.worldMat * vec4(pos, 1.0f);
}