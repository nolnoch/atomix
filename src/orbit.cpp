/**
 * orbit.cpp
 *
 *    Created on: Oct 18, 2023
 *   Last Update: Oct 18, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023 Wade Burch (GPLv3)
 * 
 *  This file is part of atomix.
 * 
 *  atomix is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software 
 *  Foundation, either version 3 of the License, or (at your option) any later 
 *  version.
 * 
 *  atomix is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with 
 *  atomix. If not, see <https://www.gnu.org/licenses/>.
 */

#include "orbit.hpp"


Orbit::Orbit(WaveConfig cfg, Orbit *prior = nullptr)
    : config(cfg), priorOrbit(prior) {
    this->idx = prior ? prior->idx + 1 : 1;
    this->amplitude = A;
    this->two_pi_L = TWO_PI / L;
    this->two_pi_T = TWO_PI / T;
    this->deg_fac = 2.0 * M_PI / STEPS;

    updateOrbit(0);
}

Orbit::~Orbit() {

}

void Orbit::updateOrbit(double t) {
    double r = (double) this->idx;
    myVertices.clear();
    myIndices.clear();

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < STEPS; i++) {
        double theta = i * deg_fac;
        myIndices.push_back(i);

        double wavefunc = amplitude * sin((two_pi_L * r * theta) - (two_pi_T * t) + phase_const);

        float x = (wavefunc + r) * cos(theta);
        float y = 0.0f;
        float z = (wavefunc + r) * sin(theta);


        
        vec vertex = vec(x, y, z);
        vec colour = vec(1.0f, 1.0f, 1.0f);
        
        myVertices.push_back(vertex);
        myVertices.push_back(colour);

        myComponents.push_back(glm::vec2(wavefunc, r));
    }

    if (idx > 1 && SUPER)
        superposition();
}

void Orbit::proximityDetect() {
    int verts = myVertices.size();

    for (int dt = 0; dt < verts; dt += 2) {
        vec a = priorOrbit->myVertices[dt];
        vec b = myVertices[dt];

        double diffX = abs(a.x) - abs(b.x);
        double diffZ = abs(a.z) - abs(b.z);

        bool crossX = diffX > 0 && diffX < 0.05;
        bool crossZ = diffZ > 0 && diffZ < 0.05;

        if (crossX && crossZ) {
            myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }
    }
}

void Orbit::superposition() {
    int verts = myVertices.size();

    for (int dt = 0; dt < verts; dt += 2) {
        vec a = priorOrbit->myVertices[dt];
        vec b = myVertices[dt];

        double diffX = abs(a.x) - abs(b.x);
        double diffZ = abs(a.z) - abs(b.z);

        if (diffX >= 0 && diffZ >= 0) {
            myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }
    }
}

int Orbit::vertexCount() {
    return myVertices.size();
}

int Orbit::vertexSize() {
    int chunks = myVertices.size();
    int chunkSize = sizeof(glm::vec3);

    return chunks * chunkSize;
}

const float* Orbit::vertexData() {
    assert(myVertices.size());
    return glm::value_ptr(myVertices.front());
}

int Orbit::indexCount() {
    return myIndices.size();
}

int Orbit::indexSize() {
    int chunks = myIndices.size();
    int chunkSize = sizeof(uint);

    return chunks * chunkSize;
}

const uint* Orbit::indexData() {
    assert(myIndices.size());
    return &myIndices[0];
}
