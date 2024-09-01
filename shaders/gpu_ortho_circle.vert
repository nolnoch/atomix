#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

out vec4 vertColour;

uniform float two_pi_L;
uniform float two_pi_T;
uniform float amp;
uniform float time;

uniform uint base;
uniform uint peak;
uniform uint trough;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
    float theta = factorsA.x;
    float r = factorsA.z;

    float cos_th = cos(theta);
    float sin_th = sin(theta);
   
    //                         sin(2pi / L * x) - (2pi / T * t)
    float wavefunc = sin((two_pi_L * r * theta) - (two_pi_T * time));
    float displacement = amp * wavefunc;

    float x_coord = r * cos_th;
    float z_coord = r * sin_th;

    //vertColour = vec3(wavefunc, 1 - wavefunc, 1.0f);
    //vertColour = uvec3(base, peak, trough);
    //uvec3 vertTest = uvec3(255);

    uint mask = 0xFF;

    float baseA = ((base & mask) / mask);
    float baseB = (((base >> 8) & mask) / mask);
    float baseG = (((base >> 16) & mask) / mask);
    float baseR = (((base >> 24) & mask) / mask);

    float peakA = (peak & mask) / mask;
    float peakB = ((peak >> 8) & mask) / mask;
    float peakG = ((peak >> 16) & mask) / mask;
    float peakR = ((peak >> 24) & mask) / mask;

    float trghA = (trough & mask) / mask;
    float trghB = ((trough >> 8) & mask) / mask;
    float trghG = ((trough >> 16) & mask) / mask;
    float trghR = ((trough >> 24) & mask) / mask;

    vec4 final = vec4(0.0f);
    float scale = abs(wavefunc);

    if (wavefunc >= 0) {
        final.r = (scale * peakR) + ((1 - scale) * baseR);
        final.g = (scale * peakG) + ((1 - scale) * baseG);
        final.b = (scale * peakB) + ((1 - scale) * baseB);
        final.a = (scale * peakA) + ((1 - scale) * baseA);
    } else {
        final.r = (scale * trghR) + ((1 - scale) * baseR);
        final.g = (scale * trghG) + ((1 - scale) * baseG);
        final.b = (scale * trghB) + ((1 - scale) * baseB);
        final.a = (scale * trghA) + ((1 - scale) * baseA);
    }

    vertColour = final;
    gl_Position = projMat * viewMat * worldMat * vec4(x_coord, displacement, z_coord, 1.0f);
};