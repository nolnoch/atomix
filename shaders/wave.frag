#version 450 core

in vec4 vertColour;
//in float wavefunc;

out vec4 FragColour;

void main() {
    FragColour = vertColour;
}
