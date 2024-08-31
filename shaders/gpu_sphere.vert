#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

out vec3 vertColour;

uniform float two_pi_L;
uniform float two_pi_T;
uniform float amp;
uniform float time;

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

    float r_theta = r * theta;
    float r_phi = r * phi;
   
    //              sin((2pi / L * x)            - (2pi / T * t))
    // sin((k * cos_phi * x) + (k * sin_phi * y) - (2pi / T * t))
    float wavefunc = sin((two_pi_L * r_theta) - (two_pi_T * time));
    float displacement = amp * wavefunc;

    float x_coord = (r + displacement) * sin_phi * sin_theta;
    float z_coord = (r + displacement) * sin_phi * cos_theta;
    float y_coord = (r + 0) * cos_phi;

    vertColour = vec3(wavefunc, 1 - wavefunc, 1.0f);
    //vertColour = factorsB;
    gl_Position = projMat * viewMat * worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
};