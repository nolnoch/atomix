#version 450 core

in vec4 vertColour;
//in float wavefunc;

out vec4 FragColour;

void main() {
    //uint r = vertColour.x;
    //uint g = vertColour.y;
    //uint b = vertColour.z;

    //float rVal = r / 255.0f;
    //float gVal = g / 255.0f;
    //float bVal = b / 255.0f;

    //float rTest = clamp(rVal, 0.0f, 1.0f);
    //float gTest = clamp(gVal, 0.0f, 1.0f);
    //float bTest = clamp(bVal, 0.0f, 1.0f);

    FragColour = vertColour;
    //FragColour = vec4(1.0f);
    //FragColour = vec4(wavefunc, 1.0f - wavefunc, 1.0f, 1.0f);
    //FragColour = vec4(rTest, gTest, bTest, 1.0f);
};