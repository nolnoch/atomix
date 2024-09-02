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
    
    //                   sin(2pi / L * x) - (2pi / T * t)
    float wavefunc = sin((two_pi_L * r * theta) - (two_pi_T * time));
    float displacement = amp * wavefunc;

    float x_coord = (r + displacement) * cos_th;
    float z_coord = (r + displacement) * sin_th;

    //uvec3 colorsCPU = uvec3(factorsB.x * 255, factorsB.y * 255, factorsB.z * 255);
    //vertColour = (length(factorsB) == 0) ? uvec3(base, peak, trough) : colorsCPU;
    //vertColour = factorsB;
    
    uint mask = 0xFF;
    float fMask = float(mask);
    vec4 final = vec4(0.0f);
    float scale = abs(wavefunc);

    float baseA = (base & mask) / fMask;
    float baseB = ((base >> 8) & mask) / fMask;
    float baseG = ((base >> 16) & mask) / fMask;
    float baseR = ((base >> 24) & mask) / fMask;

    if (wavefunc >= 0) {
        float peakA = (peak & mask) / fMask;
        float peakB = ((peak >> 8) & mask) / fMask;
        float peakG = ((peak >> 16) & mask) / fMask;
        float peakR = ((peak >> 24) & mask) / fMask;

        final.r = (scale * peakR) + ((1 - scale) * baseR);
        final.g = (scale * peakG) + ((1 - scale) * baseG);
        final.b = (scale * peakB) + ((1 - scale) * baseB);
        final.a = (scale * peakA) + ((1 - scale) * baseA);
    } else {
        float trghA = (trough & mask) / fMask;
        float trghB = ((trough >> 8) & mask) / fMask;
        float trghG = ((trough >> 16) & mask) / fMask;
        float trghR = ((trough >> 24) & mask) / fMask;

        final.r = (scale * trghR) + ((1 - scale) * baseR);
        final.g = (scale * trghG) + ((1 - scale) * baseG);
        final.b = (scale * trghB) + ((1 - scale) * baseB);
        final.a = (scale * trghA) + ((1 - scale) * baseA);
    }

    vertColour = final;
    gl_Position = projMat * viewMat * worldMat * vec4(x_coord, 0.0f, z_coord, 1.0f);
};