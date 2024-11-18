#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in float pdv;

layout(location = 0) out vec4 vertColour;

layout(set = 0, binding = 0) uniform WorldState {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} worldState;

layout (push_constant) uniform PushConstants {
    float max_radius;
} pConstCloud;


void main() {
    /* VBO Variables */
    float theta = factorsA.x;
    float phi = factorsA.y;
    float radius = factorsA.z;

    /* Position */
    float posX = radius * sin(phi) * sin(theta);
    float posY = radius * cos(phi);
    float posZ = radius * sin(phi) * cos(theta);

    /* Alpha */
    // float alpha = clamp(1.0f - (radius / pConstCloud.max_radius), 0.0f, 1.0f);       // Scale by radius (inversely)
    float alpha = pow(pdv, (1.0f / 3.0f));                                              // Scale by probability
    // float alpha = 1.0f;                                                              // No scaling
    
    /* Colours */
    vec3 colours[11] = {
        vec3(2.0f, 0.0f, 2.0f),      // [0-9%] -- Magenta
        vec3(0.0f, 0.0f, 1.5f),      // [10-19%] -- Blue
        vec3(0.0f, 0.5f, 1.0f),      // [20-29%] -- Cyan-Blue
        vec3(0.0f, 1.0f, 0.5f),      // [30-39%] -- Cyan-Green
        vec3(0.0f, 1.0f, 0.0f),      // [40-49%] -- Green
        vec3(1.0f, 1.0f, 0.0f),      // [50-59%] -- Yellow
        vec3(1.0f, 1.0f, 0.0f),      // [60-69%] -- Yellow
        vec3(1.0f, 0.0f, 0.0f),      // [70-79%] -- Red
        vec3(1.0f, 0.0f, 0.0f),      // [80-89%] -- Red
        vec3(1.0f, 1.0f, 1.0f),      // [90-99%] -- White
        vec3(1.0f, 1.0f, 1.0f)       // [100%] -- White
    };
    uint colourIdx = uint(pdv * 10.0f);
    vec3 pdvColour = colours[colourIdx] * pdv;

    vertColour = vec4(pdvColour, alpha);
    gl_Position = worldState.projMat * worldState.viewMat * worldState.worldMat * vec4(posX, posY, posZ, 1.0f);
    gl_PointSize = 1.4f;
}