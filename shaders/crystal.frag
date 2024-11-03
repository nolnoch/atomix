#version 450 core

layout(location = 0) in vec3 vertColour;

layout(location = 0) out vec4 FragColour;

void main() {
   FragColour = vec4(vertColour, 1.0f);
}