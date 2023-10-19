#version 450 core

in vec3 vertColour;

out vec4 FragColour;

void main() {
   FragColour = vec4(vertColour, 1.0f);
};