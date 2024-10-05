#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in float rdp;

out vec4 vertColour;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
    /* VBO Variables */
    float theta = factorsA.x;
    float phi = factorsA.y;
    float radius = factorsA.z;

    /* Position */
    float posX = radius * sin(phi) * sin(theta);
    float posY = radius * cos(phi);
    float posZ = radius * sin(phi) * cos(theta);

    /* Colour */
    vertColour = vec4(1.0f);
    if (theta > 3.14159f) {
        vertColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // vertColour = vec4(1.0f);
    gl_Position = projMat * viewMat * worldMat * vec4(posX, posY, posZ, 1.0f);
}