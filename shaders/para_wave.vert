#version 450 core

layout(location = 0) in vec3 factorsA;
layout(location = 1) in vec3 factorsB;

out vec3 vertColour;

uniform float time;

uniform mat4 worldMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main() {
   float amp = factorsA.x;
   float kx = factorsA.y;
   float w = factorsA.z;
   float r = factorsB.x;
   float cos_th = factorsB.y;
   float sin_th = factorsB.z;
   
   //       sin(2pi / L * x) - (2pi / T * t)
   float wavefunc = sin(kx - (w * time));
   float displacement = amp * wavefunc;

   float x_coord = (r + displacement) * cos_th;
   float z_coord = (r + displacement) * sin_th;

   vertColour = vec3(wavefunc, 1 - wavefunc, 1.0f);
   gl_Position = projMat * viewMat * worldMat * vec4(x_coord, 0.0f, z_coord, 1.0f);
};