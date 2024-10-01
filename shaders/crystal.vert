#version 450 core

layout(location = 0) in vec3 svPos;
layout(location = 1) in vec3 svCol;
    
out vec3 verColour;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;
    
void main() {
   gl_Position = projMat * viewMat * worldMat * vec4(svPos, 1.0f);
   verColour = svCol;
}