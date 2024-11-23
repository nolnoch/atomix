#version 450 core

layout(location = 0) in vec3 factorsA;

layout(location = 0) out vec4 vertColour;

layout(set = 0, binding = 0) uniform WorldState {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} worldState;

layout(set = 1, binding = 0) uniform WaveState {
    vec3 waveMaths;
    uvec3 waveColours;
} waveState;

layout(push_constant) uniform PushConstants {
    float time;
    uint mode;
} pConstWave;


void main() {
    float theta = factorsA.x;
    float phi = factorsA.y;
    float r = factorsA.z;

    float two_pi_L = waveState.waveMaths.x;
    float two_pi_T = waveState.waveMaths.y;
    float amp = waveState.waveMaths.z;

    float sin_theta = sin(theta);
    float cos_theta = cos(theta);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    float r_theta = r * theta;
    float r_phi = r * phi;

    float phase_const = 0;
   
    /* Wavefunction */
    //                             sin((2pi / L * x) - (2pi / T * t)) + phase
    //                         sin((k * cos_phi * x) + (k * sin_phi * y) - (2pi / T * t))
    float wavefunc = cos((two_pi_L * r_theta) - (two_pi_T * pConstWave.time) + phase_const);
    float displacement = amp * wavefunc;

    /* Assign vertices (Sphere) */
    float x_coord = (r + displacement) * sin_phi * sin_theta;
    float z_coord = (r + displacement) * sin_phi * cos_theta;
    float y_coord = (r + displacement) * cos_phi;

    /* Assign colours */
    uint peak = waveState.waveColours.x;
    uint base = waveState.waveColours.y;
    uint trough = waveState.waveColours.z;

    uint mask = 0xFF;
    float fMask = float(mask);
    vec4 finalPos = vec4(0.0f);
    vec4 finalNeg = vec4(0.0f);
    vec4 final = vec4(0.0f);
    float scale = abs(wavefunc);

    float baseA = (base & mask) / fMask;
    float baseB = ((base >> 8) & mask) / fMask;
    float baseG = ((base >> 16) & mask) / fMask;
    float baseR = ((base >> 24) & mask) / fMask;

    float peakA = (peak & mask) / fMask;
    float peakB = ((peak >> 8) & mask) / fMask;
    float peakG = ((peak >> 16) & mask) / fMask;
    float peakR = ((peak >> 24) & mask) / fMask;

    float trghA = (trough & mask) / fMask;
    float trghB = ((trough >> 8) & mask) / fMask;
    float trghG = ((trough >> 16) & mask) / fMask;
    float trghR = ((trough >> 24) & mask) / fMask;
    
    finalPos.r = mix(baseR, peakR, scale);
    finalPos.g = mix(baseG, peakG, scale);
    finalPos.b = mix(baseB, peakB, scale);
    finalPos.a = mix(baseA, peakA, scale);

    finalNeg.r = mix(baseR, trghR, scale);
    finalNeg.g = mix(baseG, trghG, scale);
    finalNeg.b = mix(baseB, trghB, scale);
    finalNeg.a = mix(baseA, trghA, scale);

    final = mix(finalNeg, finalPos, step(0.0f, wavefunc));

    vertColour = final;
    gl_Position = worldState.projMat * worldState.viewMat * worldState.worldMat * vec4(x_coord, y_coord, z_coord, 1.0f);
    gl_PointSize = 2.0f;
}