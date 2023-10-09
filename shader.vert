#version 450 core
    layout(location = 0) in vec3 svPos;
    layout(location = 1) in vec3 svCol;
    
    out vec3 verColour;
    
    void main() {
       gl_Position = vec4(svPos.x, svPos.y, svPos.z, 1.0);
       verColour = svCol;
    };