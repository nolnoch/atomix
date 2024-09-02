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
    newConfig(cfg);
    createOrbits();
}

OrbitManager::~OrbitManager() {
    resetManager();
}

void OrbitManager::createOrbits() {
    for (int i = 0; i < orbitCount; i++) {
        orbitVertices.push_back(new gvec);
        orbitIndices.push_back(new ivec);
        phase_const.push_back(phase_base * i);
    
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
        if (renderedOrbits & RENDORBS[i]) {
            orbitVertices[i]->clear();

            if (config->sphere)
                updateOrbitCPUSphere(i, time);
            else
                updateOrbitCPUCircle(i, time);
        }
    }

    genVertexArray();
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

void OrbitManager::newOrbits() {
    resetManager();
    createOrbits();
}

uint OrbitManager::selectOrbits(int id, bool checked) {
    uint flag = id;

    if (checked)
        renderedOrbits |= flag;
    else
        renderedOrbits &= ~(flag);

    allIndices.clear();
    genIndexBuffer();

    return renderedOrbits;
}

void OrbitManager::circleOrbitGPU(int idx) {
    double radius = (double) (idx + 1);
    int l = idx * this->resolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->resolution; i++) {
        double theta = i * deg_fac;
        orbitIndices[idx]->push_back(l + i);

        float h = (float) theta;
        float p = (float) phase_const[idx];
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
    int l = idx * this->resolution * this->resolution;

    for (int i = 0; i < this->resolution; i++) {
        int m = i * this->resolution;
        for (int j = 0; j < this->resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            orbitIndices[idx]->push_back(l + m + j);

            float h = (float) theta;
            float p = (float) phi;
            float d = (float) radius;
            float r = (float) phase_const[idx];
            float g = 0;
            float b = 0;

            vec factorsA = vec(h, p, d);
            vec factorsB = vec(r, g, b);
            
            orbitVertices[idx]->push_back(factorsA);
            orbitVertices[idx]->push_back(factorsB);
            //std::cout << glm::to_string(factorsB) << "\n";
        }
    }
}

void OrbitManager::updateOrbitCPUCircle(int idx, double t) {
    double radius = (double) (idx + 1);
    int l = idx * this->resolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->resolution; i++) {
        double theta = i * deg_fac;
        float x, y, z;
        
        if (!update)
            orbitIndices[idx]->push_back(l + i);

        double wavefunc = cos((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const[idx]);
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

        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float scale = abs(wavefunc);

        if (wavefunc >= 0) {
            r = (scale * SHIFT(peak, RED)) + ((1 - scale) * SHIFT(base, RED));
            g = (scale * SHIFT(peak, GREEN)) + ((1 - scale) * SHIFT(base, GREEN));
            b = (scale * SHIFT(peak, BLUE)) + ((1 - scale) * SHIFT(base, BLUE));
        } else {
            r = (scale * SHIFT(trough, RED)) + ((1 - scale) * SHIFT(base, RED));
            g = (scale * SHIFT(trough, GREEN)) + ((1 - scale) * SHIFT(base, GREEN));
            b = (scale * SHIFT(trough, BLUE)) + ((1 - scale) * SHIFT(base, BLUE));
        }

        vec vertex = vec(x, y, z);
        vec colour = vec(r, g, b);
        
        orbitVertices[idx]->push_back(vertex);
        orbitVertices[idx]->push_back(colour);
    }
    if (config->superposition && idx > 0) {
        superposition(idx);
    }
}

void OrbitManager::updateOrbitCPUSphere(int idx, double t) {
    double radius = (double) (idx + 1);
    int l = idx * this->resolution * this->resolution;

    for (int i = 0; i < this->resolution; i++) {
        int m = i * this->resolution;
        for (int j = 0; j < this->resolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;
            
            if (!update)
                orbitIndices[idx]->push_back(l + m + j);

            float wavefunc = cos((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const[idx]);
            float displacement = amplitude * wavefunc;

            float x = (float) (radius + displacement) * (sin(phi) * sin(theta));
            float y = (float) cos(phi);
            float z = (float) (radius + displacement) * (sin(phi) * cos(theta));

            float r = 0;
            float g = 0;
            float b = 0;
            
            float scale = abs(wavefunc);

            if (wavefunc >= 0) {
                r = (scale * SHIFT(peak, RED)) + ((1 - scale) * SHIFT(base, RED));
                g = (scale * SHIFT(peak, GREEN)) + ((1 - scale) * SHIFT(base, GREEN));
                b = (scale * SHIFT(peak, BLUE)) + ((1 - scale) * SHIFT(base, BLUE));
                //final.a = (scale * SHIFT(peak, ALPHA)) + ((1 - scale) * SHIFT(base, ALPHA));
            } else {
                r = (scale * SHIFT(trough, RED)) + ((1 - scale) * SHIFT(base, RED));
                g = (scale * SHIFT(trough, GREEN)) + ((1 - scale) * SHIFT(base, GREEN));
                b = (scale * SHIFT(trough, BLUE)) + ((1 - scale) * SHIFT(base, BLUE));
                //final.a = (scale * SHIFT(trough, ALPHA)) + ((1 - scale) * SHIFT(base, ALPHA));
            }

            vec vertex = vec(x, y, z);
            vec colour = vec(r, g, b);
            
            orbitVertices[idx]->push_back(vertex);
            orbitVertices[idx]->push_back(colour);
        }
    }
    if (config->superposition && idx > 0) {
        superposition(idx);
    }
}

void OrbitManager::superposition(int idx) {
    int verts = orbitVertices[idx]->size();
    vec red = vec(1.0f, 0.0f, 0.0f);

    for (int dt = 0; dt < verts; dt += 2) {
        vec &a = (*orbitVertices[idx - 1])[dt];
        vec &b = (*orbitVertices[idx])[dt];

        if (length(a) > length(b)) {
            // Calculate interference
            vec avg = (a + b) * 0.5f;

            // Adjust vertices for interference
            b = avg;
            a = avg;

            // Highlight adjusted vertices
            (*orbitVertices[idx])[dt+1] = red;
            (*orbitVertices[idx - 1])[dt+1] = red;
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
    assert(!allIndices.size());

    for (int i = 0; i < orbitCount; i++) {
        if (renderedOrbits & (1 << i)) {
            std::copy(orbitIndices[i]->begin(), orbitIndices[i]->end(), std::back_inserter(this->allIndices));
        }
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
    if (!indexCount)
        return 0;

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
