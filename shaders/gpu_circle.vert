#version 450 core

layout(location = 0) in vec3 factorsA;

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
    uint mode;
} ubo;

void main() {
    float theta = factorsA.x;
    float phase_const = factorsA.y;
    float r = factorsA.z;

    float cos_th = cos(theta);
    float sin_th = sin(theta);
   
    /* Circular Wavefunction */
    //                             sin(2pi / L * x) - (2pi / T * t)
    float wavefunc = cos((ubo.two_pi_L * r * theta) - (ubo.two_pi_T * ubo.time) + phase_const);
    float displacement = ubo.amp * wavefunc;

    /* Assign vertices */
    float x_coord, y_coord, z_coord;
    if (ubo.mode == 1) {
        // Orthogonal waves
        x_coord = r * cos_th;
        y_coord = clamp(displacement, 0.0f, 1.0f);
        z_coord = r * sin_th;
    } else if (ubo.mode == 2) {
        // Parallel waves
        x_coord = (r + displacement) * cos_th;
        y_coord = 0.0f;
        z_coord = (r + displacement) * sin_th;
    }

    /* Assign colours */
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