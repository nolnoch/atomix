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
    this->amplitude = config.amplitude;
    this->two_pi_L = TWO_PI / config.wavelength;
    this->two_pi_T = TWO_PI / config.period;
    this->deg_fac = TWO_PI / config.resolution;

    if (config.sphere) {
        sphereOrbitB();
    } else if (!config.cpu) {
        genOrbit();
    } else {
        updateOrbit(0);
    }
}

Orbit::~Orbit() {

}

void Orbit::sphereOrbitA() {
    myVertices.clear();
    myIndices.clear();

    for (int i = 0; i < config.resolution; i++) {
        double theta = i * deg_fac;
        
        myIndices.push_back(i);

        float x = 1.0f;
        float y = (float) (this->idx - 1);          //idx=1: 0, idx=2: 1
        float z = (float) -(this->idx - 2);         //idx=1: 1, idx=2: 0
        float h = (float) theta;
        float c = (float) cos(theta);
        float s = (float) sin(theta);

        vec factorsA = vec(x, y, z);
        vec factorsB = vec(h, c, s);
        
        myVertices.push_back(factorsA);
        // std::cout << glm::to_string(factorsA) << "\n";
        myVertices.push_back(factorsB);
    }
}

void Orbit::sphereOrbitB() {
    double radius = (double) this->idx;
    myVertices.clear();
    myIndices.clear();

    for (int i = 0; i < config.resolution; i++) {
        for (int j = 0; j < config.resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            myIndices.push_back(i*config.resolution + j);

            float a = amplitude;
            float k = two_pi_L;
            float w = two_pi_T;
            float h = (float) theta;
            float p = (float) phi;
            float r = (float) radius;

            vec factorsA = vec(a, k, w);
            vec factorsB = vec(h, p, r);
            
            myVertices.push_back(factorsA);
            myVertices.push_back(factorsB);
            //std::cout << glm::to_string(factorsB) << "\n";
        }
    }

    std::cout << "Sphere " << this->idx << " generation complete." << std::endl;
}

void Orbit::sphereOrbitCPU() {
    double radius = (double) this->idx;
    myVertices.clear();
    myIndices.clear();

    for (int i = 0; i < config.resolution; i++) {
        for (int j = 0; j < config.resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            myIndices.push_back(i*config.resolution + j);

            float r = 0.8f;
            float g = 0.8f;
            float b = 0.8f;

            float x = (float) (sin(phi) * sin(theta));
            float y = (float) cos(phi);
            float z = (float) (sin(phi) * cos(theta));

            vec factorsA = vec(r, g, b);
            vec factorsB = vec(x, y, z);
            
            myVertices.push_back(factorsA);
            myVertices.push_back(factorsB);
        }
    }
}

void Orbit::genOrbit() {
    double radius = (double) this->idx;
    myVertices.clear();
    myIndices.clear();

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < config.resolution; i++) {
        double theta = i * deg_fac;
        myIndices.push_back(i);

        float a = amplitude;
        float k = two_pi_L * radius * theta;
        float w = two_pi_T;
        float r = (float) radius;
        float c = cos(theta);
        float s = sin(theta);

        vec factorsA = vec(a, k, w);
        vec factorsB = vec(r, c, s);
        
        myVertices.push_back(factorsA);
        myVertices.push_back(factorsB);
    }
}

void Orbit::updateOrbit(double t) {
    double r = (double) this->idx;
    myVertices.clear();
    myIndices.clear();

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < config.resolution; i++) {
        float x, y, z;
        double theta = i * deg_fac;
        myIndices.push_back(i);

        double wavefunc = amplitude * sin((two_pi_L * r * theta) - (two_pi_T * t) + phase_const);

        if (config.parallel) {
            x = (wavefunc + r) * cos(theta);
            y = 0.0f;
            z = (wavefunc + r) * sin(theta);
        } else {
            x = r * cos(theta);
            y = wavefunc;
            z = r * sin(theta);
        }

        vec vertex = vec(x, y, z);
        vec colour = vec(1.0f, 1.0f, 1.0f);
        
        myVertices.push_back(vertex);
        myVertices.push_back(colour);
    }

    if (idx > 1 && config.superposition)
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

        /* Interesection highlight */
        if (crossX && crossZ) {
            myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }

        /* Crossing region highlight 
        if (diffX >= 0 && diffZ >= 0) {
            myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }*/
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
            //myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            //priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);

            float avgX = (a.x + b.x) / 2;
            float avgZ = (a.z + b.z) / 2;

            vec avg = vec(avgX, 0.0f, avgZ);

            myVertices[dt] = avg;
            priorOrbit->myVertices[dt] = avg;
        }
    }
}

int Orbit::vertexCount() {
    return myVertices.size();
}

int Orbit::vertexSize() {
    int chunks = myVertices.size();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "MyVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
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
