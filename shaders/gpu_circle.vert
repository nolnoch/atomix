#version 450 core

layout(location = 0) in vec3 factorsA;

layout(location = 0) out vec4 vertColour;

layout(set = 0, binding = 0) uniform MatrixUBO {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} worldState;

layout(set = 1, binding = 0) uniform WaveUBO {
    vec3 waveMaths;
    uvec3 waveColours;
} waveState;

layout(push_constant) uniform constants {
    float time;
    uint mode;
    float phase;
} pConst;


void main() {
    float theta = factorsA.x;
    float phase_const = factorsA.y;
    float r = factorsA.z;

    float two_pi_L = waveState.waveMaths.x;
    float two_pi_T = waveState.waveMaths.y;
    float amp = waveState.waveMaths.z;

    uint peak = waveState.waveColours.x;
    uint base = waveState.waveColours.y;
    uint trough = waveState.waveColours.z;

    float cos_th = cos(theta);
    float sin_th = sin(theta);
   
    /* Circular Wavefunction */
    //                             sin(2pi / L * x) - (2pi / T * t)
    float wavefunc = cos((two_pi_L * r * theta) - (two_pi_T * pConst.time) + phase_const);
    float displacement = amp * wavefunc;

    /* Assign vertices */
    float x_coord, y_coord, z_coord;
    if (pConst.mode == 0) {
        // Orthogonal waves
        x_coord = r * cos_th;
        y_coord = displacement;
        z_coord = r * sin_th;
    } else if (pConst.mode == 1) {
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
    gl_Position = worldState.projMat * worldState.viewMat * worldState.worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
    gl_PointSize = 2.0f;
}