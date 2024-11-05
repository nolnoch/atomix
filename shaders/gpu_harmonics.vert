#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in float pdv;

layout(location = 0) out vec4 vertColour;

layout(binding = 0) uniform UniformBuffer {
    mat4 worldMat;
    mat4 viewMat;
    mat4 projMat;
} ubo;

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

    if (pdv > 0.90f) {          // 90% - 100% -- White
        vertColour = vec4(pdv, pdv, pdv, alpha);
    } else if (pdv > 0.70f) {   // 70% - 90%  -- Red
        vertColour = vec4(pdv, 0.0f, 0.0f, alpha);
    } else if (pdv > 0.50f) {   // 50% - 70%  -- Yellow
        vertColour = vec4(pdv, pdv, 0.0f, alpha);
    } else if (pdv > 0.35f) {   // 35% - 50%  -- Green
        vertColour = vec4(0.0f, pdv, 0.0f, alpha);
    } else if (pdv > 0.20f) {   // 20% - 35%  -- Cyan
        vertColour = vec4(0.0f, pdv, pdv, alpha);
    } else if (pdv > 0.10f) {   // 10% - 20%  -- Blue
        float pdv_adj = pdv * 1.5f;
        vertColour = vec4(0.0f, 0.0f, pdv_adj, alpha);
    } else {                    //  0% - 10%  -- Purple
        float pdv_adj = pdv * 2.0f;
        vertColour = vec4(pdv_adj, 0.0f, pdv_adj, 0.0f);
    }

    gl_Position = ubo.projMat * ubo.viewMat * ubo.worldMat * vec4(posX, posY, posZ, 1.0f);
}