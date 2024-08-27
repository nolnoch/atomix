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

#include "orbitmanager.hpp"


OrbitManager::OrbitManager(WaveConfig *cfg)
    : config(cfg) {
    this->orbitCount = config->orbits;
    this->amplitude = config->amplitude;
    this->resolution = config->resolution;
    this->two_pi_L = TWO_PI / config->wavelength;
    this->two_pi_T = TWO_PI / config->period;
    this->deg_fac = TWO_PI / this->resolution;

    createOrbits();
}

OrbitManager::~OrbitManager() {
    resetManager();
}

void OrbitManager::createOrbits() {
    for (int i = 0; i < orbitCount; i++) {
        orbitVertices.push_back(new gvec);
        orbitIndices.push_back(new ivec);
    
        if (config->sphere) {
            if (config->cpu)
                updateOrbitCPUSphere(i, 0);
            else
                sphereOrbitGPU(i);
        } else {
            if (config->cpu)
                updateOrbitCPUCircle(i, 0);
            else
                circleOrbitGPU(i);
        }
    }

    genVertexArray();
    genIndexBuffer();
}

void OrbitManager::updateOrbits(double time) {
    this->update = true;
    allVertices.clear();
    
    for (int i = 0; i < orbitCount; i++) {
        orbitVertices[i]->clear();

        if (config->sphere)
            updateOrbitCPUSphere(i, time);
        else
            updateOrbitCPUCircle(i, time);
    }

    genVertexArray();
}

void OrbitManager::newOrbits() {
    resetManager();
    createOrbits();
}

void OrbitManager::newConfig(WaveConfig *cfg) {
    this->config = cfg;
    this->orbitCount = config->orbits;
    this->amplitude = config->amplitude;
    this->resolution = config->resolution;
    this->two_pi_L = TWO_PI / config->wavelength;
    this->two_pi_T = TWO_PI / config->period;
    this->deg_fac = TWO_PI / this->resolution;
}

/*
void OrbitManager::sphereOrbitA() {
    orbitVertices[idx]->clear();
    orbitIndices[idx]->clear();

    for (int i = 0; i < config->resolution; i++) {
        double theta = i * deg_fac;
        
        orbitIndices[idx]->push_back(i);

        float x = 1.0f;
        float y = (float) (this->idx - 1);          //idx=1: 0, idx=2: 1
        float z = (float) -(this->idx - 2);         //idx=1: 1, idx=2: 0
        float h = (float) theta;
        float c = (float) cos(theta);
        float s = (float) sin(theta);

        vec factorsA = vec(x, y, z);
        vec factorsB = vec(h, c, s);
        
        orbitVertices[idx]->push_back(factorsA);
        // std::cout << glm::to_string(factorsA) << "\n";
        orbitVertices[idx]->push_back(factorsB);
    }
}
*/
/*
void OrbitManager::sphereOrbitCPU() {
    double radius = (double) this->idx;
    orbitVertices[idx]->clear();
    orbitIndices[idx]->clear();

    for (int i = 0; i < config->resolution; i++) {
        for (int j = 0; j < config->resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            orbitIndices[idx]->push_back(i*config->resolution + j);
            vaoIndices->push_back(i*config->resolution + j);

            float r = 0.8f;
            float g = 0.8f;
            float b = 0.8f;

            float wavefunc = sin((two_pi_L * radius * theta) - (two_pi_T * 0));
            float displacement = amplitude * wavefunc;

            float x = (float) (radius + displacement) * (sin(phi) * sin(theta));
            float y = (float) cos(phi);
            float z = (float) (radius + displacement) * (sin(phi) * cos(theta));

            vec factorsA = vec(x, y, z);
            vec factorsB = vec(r, g, b);
            
            orbitVertices[idx]->push_back(factorsA);
            orbitVertices[idx]->push_back(factorsB);

            vaoVerts->push_back(factorsA);
            vaoVerts->push_back(factorsB);
        }
    }
}
*/

void OrbitManager::circleOrbitGPU(int idx) {
    double radius = (double) (idx + 1);
    int l = idx * config->resolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < config->resolution; i++) {
        double theta = i * deg_fac;
        orbitIndices[idx]->push_back(l + i);

        float h = theta;
        float p = 0;
        float d = (float) radius;
        float r = 0;
        float g = 0;
        float b = 0;

        vec factorsA = vec(h, p, d);
        vec factorsB = vec(r, g, b);
        
        orbitVertices[idx]->push_back(factorsA);
        orbitVertices[idx]->push_back(factorsB);
    }
}

void OrbitManager::sphereOrbitGPU(int idx) {
    double radius = (double) (idx + 1);
    int l = idx * config->resolution * config->resolution;

    for (int i = 0; i < config->resolution; i++) {
        int m = i * config->resolution;
        for (int j = 0; j < config->resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            orbitIndices[idx]->push_back(l + m + j);

            float h = (float) theta;
            float p = (float) phi;
            float d = (float) radius;
            float r = 0;
            float g = 0;
            float b = 0;

            vec factorsA = vec(h, p, d);
            vec factorsB = vec(r, g, b);
            
            orbitVertices[idx]->push_back(factorsA);
            orbitVertices[idx]->push_back(factorsB);
            //std::cout << glm::to_string(factorsB) << "\n";
        }
    }

    //std::cout << "Sphere " << this->idx << " generation complete." << std::endl;
}

void OrbitManager::updateOrbitCPUCircle(int idx, double t) {
    double radius = (double) (idx + 1);
    int l = idx * config->resolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < config->resolution; i++) {
        double theta = i * deg_fac;
        float x, y, z;
        
        if (!update)
            orbitIndices[idx]->push_back(l + i);

        float r = 0.8f;
        float g = 0.8f;
        float b = 0.8f;

        double wavefunc = amplitude * sin((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const);
        double displacement = amplitude * wavefunc;

        if (config->parallel) {
            x = (displacement + radius) * cos(theta);
            y = 0.0f;
            z = (displacement + radius) * sin(theta);
        } else {
            x = radius * cos(theta);
            y = displacement;
            z = radius * sin(theta);
        }

        vec vertex = vec(x, y, z);
        vec colour = vec(r, g, b);
        
        orbitVertices[idx]->push_back(vertex);
        orbitVertices[idx]->push_back(colour);
    }

    if (config->superposition && idx > 0)
        superposition(idx);
}

void OrbitManager::updateOrbitCPUSphere(int idx, double t) {
    double radius = (double) (idx + 1);
    int l = idx * config->resolution * config->resolution;

    for (int i = 0; i < config->resolution; i++) {
        int m = i * config->resolution;
        for (int j = 0; j < config->resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            if (!update)
                orbitIndices[idx]->push_back(l + m + j);

            float r = 0.8f;
            float g = 0.8f;
            float b = 0.8f;

            float wavefunc = sin((two_pi_L * radius * theta) - (two_pi_T * t));
            float displacement = amplitude * wavefunc;

            float x = (float) (radius + displacement) * (sin(phi) * sin(theta));
            float y = (float) cos(phi);
            float z = (float) (radius + displacement) * (sin(phi) * cos(theta));

            vec vertex = vec(x, y, z);
            vec colour = vec(r, g, b);
            
            orbitVertices[idx]->push_back(vertex);
            orbitVertices[idx]->push_back(colour);
        }
    }
}

void OrbitManager::proximityDetect(int idx) {
    int verts = orbitVertices[idx]->size();

    for (int dt = 0; dt < verts; dt += 2) {
        vec a = (*orbitVertices[idx - 1])[dt];
        vec b = (*orbitVertices[idx])[dt];

        double diffX = abs(a.x) - abs(b.x);
        double diffZ = abs(a.z) - abs(b.z);

        bool crossX = diffX > 0 && diffX < 0.05;
        bool crossZ = diffZ > 0 && diffZ < 0.05;

        /* Interesection highlight */
        if (crossX && crossZ) {
            (*orbitVertices[idx])[dt+1] = vec(1.0f, 0.0f, 0.0f);
            (*orbitVertices[idx - 1])[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }

        /* Crossing region highlight 
        if (diffX >= 0 && diffZ >= 0) {
            myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
        }*/
    }
}

void OrbitManager::superposition(int idx) {
    int verts = orbitVertices[idx]->size();

    for (int dt = 0; dt < verts; dt += 2) {
        vec a = (*orbitVertices[idx - 1])[dt];
        vec b = (*orbitVertices[idx])[dt];

        double diffX = abs(a.x) - abs(b.x);
        double diffZ = abs(a.z) - abs(b.z);

        if (diffX >= 0 && diffZ >= 0) {
            //myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);
            //priorOrbit->myVertices[dt+1] = vec(1.0f, 0.0f, 0.0f);

            float avgX = (a.x + b.x) / 2;
            float avgZ = (a.z + b.z) / 2;

            vec avg = vec(avgX, 0.0f, avgZ);

            (*orbitVertices[idx])[dt] = avg;
            (*orbitVertices[idx - 1])[dt] = avg;
        }
    }
}

void OrbitManager::resetManager() {
    for (auto v : orbitVertices) {
        delete (v);
    }
    orbitVertices.clear();
    for (auto c : orbitIndices) {
        delete (c);
    }
    orbitIndices.clear();

    allVertices.clear();
    allIndices.clear();

    this->vertexCount = 0;
    this->vertexSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;
    this->update = false;
}

void OrbitManager::genVertexArray() {
    for (int i = 0; i < orbitCount; i++) {
        std::copy(orbitVertices[i]->begin(), orbitVertices[i]->end(), std::back_inserter(this->allVertices));
    }

    this->vertexCount = setVertexCount();
    this->vertexSize = setVertexSize();
}

void OrbitManager::genIndexBuffer() {
    for (int i = 0; i < orbitCount; i++) {
        std::copy(orbitIndices[i]->begin(), orbitIndices[i]->end(), std::back_inserter(this->allIndices));
    }

    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();
}

int OrbitManager::getVertexSize() {
    return this->vertexSize;
}

int OrbitManager::getIndexCount() {
    return this->indexCount;
}

int OrbitManager::getIndexSize() {
    return this->indexSize;
}

int OrbitManager::setVertexCount() {
    return allVertices.size();
}

int OrbitManager::setVertexSize() {
    int chunks = vertexCount ?: setVertexCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

const float* OrbitManager::getVertexData() {
    assert(vertexCount);
    return glm::value_ptr(allVertices.front());
}

int OrbitManager::setIndexCount() {
    return allIndices.size();
}

int OrbitManager::setIndexSize() {
    int chunks = indexCount ?: setIndexCount();
    int chunkSize = sizeof(uint);

    //std::cout << "allIndices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

const uint* OrbitManager::getIndexData() {
    assert(indexCount);
    return &allIndices[0];
}

void OrbitManager::printIndices() {
    for (auto v : this->allIndices) {
        std::cout << v << ", ";
    }
    std::cout << std::endl;
}

void OrbitManager::printVertices() {
    for (auto v : this->allVertices) {
        std::cout << glm::to_string(v) << ", ";
    }
    std::cout << std::endl;
}
