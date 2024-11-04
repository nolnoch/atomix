#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

layout(location = 0) out vec4 vertColour;

layout(binding = 0) uniform UniformBuffer {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
    float two_pi_L;
    float two_pi_T;
    float amp;
    float time;
    uint base;
    uint peak;
    uint trough;
} ubo;

void main() {
    float theta = factorsA.x;
    float phi = factorsA.y;
    float r = factorsA.z;
    float phase_const = factorsB.x;

    float sin_theta = sin(theta);
    float cos_theta = cos(theta);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    float r_theta = r * theta;
    float r_phi = r * phi;
   
    //              sin((2pi / L * x)            - (2pi / T * t))
    // sin((k * cos_phi * x) + (k * sin_phi * y) - (2pi / T * t))
    float wavefunc = cos((ubo.two_pi_L * r_theta) - (ubo.two_pi_T * ubo.time) + phase_const);
    //float wavefunc = cos((two_pi_L * r * cos_phi) + (two_pi_L * r * sin_phi) - (two_pi_T * time) + phase_const);
    //float wavefunc = abs(sin_theta * cos_theta);
    float displacement = ubo.amp * wavefunc;

    float x_coord = (r + displacement) * sin_phi * sin_theta;
    float z_coord = (r + displacement) * sin_phi * cos_theta;
    float y_coord = (r + displacement) * cos_phi;

    //vertColour = vec3(wavefunc, 1 - wavefunc, 1.0f);
    //vertColour = factorsB;

    uint mask = 0xFF;
    float fMask = float(mask);
    vec4 final = vec4(0.0f);
    float scale = abs(wavefunc);

    float baseA = (ubo.base & mask) / fMask;
    float baseB = ((ubo.base >> 8) & mask) / fMask;
    float baseG = ((ubo.base >> 16) & mask) / fMask;
    float baseR = ((ubo.base >> 24) & mask) / fMask;

    if (wavefunc >= 0) {
        float peakA = (ubo.peak & mask) / fMask;
        float peakB = ((ubo.peak >> 8) & mask) / fMask;
        float peakG = ((ubo.peak >> 16) & mask) / fMask;
        float peakR = ((ubo.peak >> 24) & mask) / fMask;

        final.r = (scale * peakR) + ((1 - scale) * baseR);
        final.g = (scale * peakG) + ((1 - scale) * baseG);
        final.b = (scale * peakB) + ((1 - scale) * baseB);
        final.a = (scale * peakA) + ((1 - scale) * baseA);
    } else {
        float trghA = (ubo.trough & mask) / fMask;
        float trghB = ((ubo.trough >> 8) & mask) / fMask;
        float trghG = ((ubo.trough >> 16) & mask) / fMask;
        float trghR = ((ubo.trough >> 24) & mask) / fMask;

        final.r = (scale * trghR) + ((1 - scale) * baseR);
        final.g = (scale * trghG) + ((1 - scale) * baseG);
        final.b = (scale * trghB) + ((1 - scale) * baseB);
        final.a = (scale * trghA) + ((1 - scale) * baseA);
    }

    vertColour = final;
    gl_Position = ubo.projMat * ubo.viewMat * ubo.worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
}