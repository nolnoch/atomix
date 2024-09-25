#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in float rdp;

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
    //  FF0000 -> FFFF00 -> 00FF00 -> 00FFFF -> 0000FF -> FF00FF -> FFFFFF
    //    red     yellow     green     cyan      blue     magenta    white
    
    //  magenta    blue      green    yellow     red      
    //  FF00FF -> 0000FF -> 00FF00 -> FFFF00 -> FF0000
    //  16,711,935 -> 255 -> 65,280 -> 16,776,960 -> 16,711,680

    // 0000FF -> 00FFFF -> FFFFFF
    //  Blue      Cyan     White

    float alpha = 1.0f - (radius / 30.0f);

    /* if (rdp > 0.8f) {
        vertColour = vec4(rdp, 0.0f, 0.0f, alpha);
    } else if (rdp > 0.6f) {
        vertColour = vec4(rdp, rdp, 0.0f, alpha);
    } else if (rdp > 0.4f) {
        vertColour = vec4(0.0f, rdp, 0.0f, alpha);
    } else if (rdp > 0.2f) {
        vertColour = vec4(0.0f, 0.0f, rdp, alpha);
    } else {
        vertColour = vec4(rdp, 0.0f, rdp, alpha);
    } */

    if (rdp > 0.90f) {
        vertColour = vec4(rdp, rdp, rdp, alpha);
    } else if (rdp > 0.75f) {
        vertColour = vec4(rdp, 0.0f, 0.0f, alpha);
    } else if (rdp > 0.60f) {
        vertColour = vec4(rdp, rdp, 0.0f, alpha);
    } else if (rdp > 0.45f) {
        vertColour = vec4(0.0f, rdp, 0.0f, alpha);
    } else if (rdp > 0.30f) {
        vertColour = vec4(0.0f, rdp, rdp, alpha);
    } else if (rdp > 0.15f) {
        vertColour = vec4(0.0f, 0.0f, rdp, alpha);
    } else {
        vertColour = vec4(rdp, 0.0f, rdp, alpha);
    }

    // vertColour = vec4(1.0f);
    gl_Position = projMat * viewMat * worldMat * vec4(posX, posY, posZ, 1.0f);
};