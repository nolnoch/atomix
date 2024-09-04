#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

out vec4 vertColour;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
    float theta = factorsA.x;
    float phi = factorsA.y;
    float r = factorsA.z;

    float sin_theta = sin(theta);
    float cos_theta = cos(theta);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    float x_coord = r * sin_phi * sin_theta;
    float z_coord = r * sin_phi * cos_theta;
    float y_coord = r * cos_phi;

    vertColour = vec4(factorsB, 1.0f);
    gl_Position = projMat * viewMat * worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
};