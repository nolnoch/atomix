#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

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
    double wavefunction = 4.0 * exp(-2.0 * radius) * radius * radius;
    float rpd = float(clamp((wavefunction * wavefunction * 3.0), 0.0, 1.0));
    vec3 pointColour = vec3(0.0f);

    /* Assign Colour iff Visible */
    if (rpd >= 0.1f) {
        pointColour = vec3(rpd);
    }

    vertColour = vec4(pointColour, 0.1f);
    gl_Position = projMat * viewMat * worldMat * vec4(posX, posY, posZ, 1.0f);
};