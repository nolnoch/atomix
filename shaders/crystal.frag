#version 450 core
out vec4 FragColour;

in vec3 verColour;

void main() {
   FragColour = vec4(verColour, 1.0f);
};