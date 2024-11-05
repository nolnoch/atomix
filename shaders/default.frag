#version 450 core

layout(location = 0) in vec4 vertColour;

layout(location = 0) out vec4 FragColour;

void main() {
    FragColour = vertColour;
}
