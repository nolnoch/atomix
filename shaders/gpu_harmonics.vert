#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in float pdv;

layout(location = 0) out vec4 vertColour;

layout(set = 0, binding = 0) uniform WorldState {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} worldState;


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

    float alpha = clamp(1.0f - (radius / 150.0f), 0.0f, 1.0f);
    vec3 pdvColour;

    if (pdv > 0.90f) {          // 90% - 100% -- White
        pdvColour = vec3(pdv, pdv, pdv);
    } else if (pdv > 0.70f) {   // 70% - 90%  -- Red
        pdvColour = vec3(pdv, 0.0f, 0.0f);
    } else if (pdv > 0.50f) {   // 50% - 70%  -- Yellow
        pdvColour = vec3(pdv, pdv, 0.0f);
    } else if (pdv > 0.35f) {   // 35% - 50%  -- Green
        pdvColour = vec3(0.0f, pdv, 0.0f);
    } else if (pdv > 0.20f) {   // 20% - 35%  -- Cyan
        pdvColour = vec3(0.0f, pdv, pdv);
    } else if (pdv > 0.10f) {   // 10% - 20%  -- Blue
        float pdv_adj = pdv * 1.5f;
        pdvColour = vec3(0.0f, 0.0f, pdv_adj);
    } else {                    //  0% - 10%  -- Purple
        float pdv_adj = pdv * 2.0f;
        pdvColour = vec3(pdv_adj, 0.0f, pdv_adj);
    }

    vertColour = vec4(pdvColour, alpha);
    gl_Position = worldState.projMat * worldState.viewMat * worldState.worldMat * vec4(posX, posY, posZ, 1.0f);
    gl_PointSize = 1.3f;
}