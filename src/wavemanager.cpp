/**
 * wave.cpp
 *
 *    Created on: Oct 18, 2023
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023, 2024 Wade Burch (GPLv3)
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

#include "wavemanager.hpp"


WaveManager::WaveManager(AtomixConfig *config) {
    newConfig(config);
    create();
    mStatus.set(em::INIT);
}

WaveManager::~WaveManager() {
    resetManager();
}

void WaveManager::newConfig(AtomixConfig *config) {
    Manager::newConfig(config);

    this->waveResolution = cfg.resolution;
    this->waveAmplitude = cfg.amplitude;
    this->two_pi_L = TWO_PI / this->cfg.wavelength;
    this->two_pi_T = TWO_PI / this->cfg.period;
    this->deg_fac = TWO_PI / this->waveResolution;
}

void WaveManager::initManager() {
    create();
}

uint WaveManager::receiveConfig(AtomixConfig *config) {
    BitFlag flWaveCfg;
    BitFlag flGraphState;
    
    // Check for relevant config changes OR for recipes to require larger radius
    if (cfg.waves != config->waves) {                    // Requires new {Vertices[cpu/gpu], VBO, EBO}
        flWaveCfg.set(ewc::ORBITS);
    }
    if (cfg.amplitude != config->amplitude) {            // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::AMPLITUDE);
    }
    if (cfg.period != config->period) {                  // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::PERIOD);
    }
    if (cfg.wavelength != config->wavelength) {          // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::WAVELENGTH);
    }
    if (cfg.resolution != config->resolution) {          // Requires new {Vertices[cpu/gpu], VBO, EBO}
        flWaveCfg.set(ewc::RESOLUTION);
    }
    if (cfg.parallel != config->parallel) {              // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::PARALLEL);
    }
    if (cfg.superposition != config->superposition) {    // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::SUPERPOSITION);
    }
    if (cfg.cpu != config->cpu) {                        // Requires new {Vertices[cpu/gpu]}
        flWaveCfg.set(ewc::CPU);
    }
    if (cfg.sphere != config->sphere) {                  // Requires new {Vertices[cpu/gpu], VBO, EBO]}
        flWaveCfg.set(ewc::SPHERE);
    }
    if (cfg.vert != config->vert) {                      // Requires new {Shader}
        flWaveCfg.set(ewc::VERTSHADER);
    }
    if (cfg.frag != config->frag) {                      // Requires new {Shader}
        flWaveCfg.set(ewc::FRAGSHADER);
    }

    if ((flWaveCfg.hasAny(ewc::ORBITS | ewc::RESOLUTION | ewc::SPHERE | ewc::CPU)) ||
        (cfg.cpu && (flWaveCfg.hasAny(ewc::AMPLITUDE | ewc::PERIOD | ewc::WAVELENGTH | ewc::PARALLEL)))) {
        resetManager();
    }
    this->newConfig(config);
    
    // Re-create Waves with new params from config
    if ((flWaveCfg.hasAny(ewc::ORBITS | ewc::RESOLUTION | ewc::SPHERE | ewc::CPU)) ||
        (cfg.cpu && (flWaveCfg.hasAny(ewc::AMPLITUDE | ewc::PERIOD | ewc::WAVELENGTH | ewc::PARALLEL)))) {
        this->create();
        flGraphState.set(em::UPD_VBO | em::UPD_EBO);
        flGraphState.set(em::UPD_UNI_MATHS | em::UPD_UNI_COLOUR);
    } else if (!cfg.cpu && (flWaveCfg.hasAny(ewc::AMPLITUDE | ewc::PERIOD | ewc::WAVELENGTH))) {
        flGraphState.set(em::UPD_UNI_MATHS);
    }

    if (flWaveCfg.hasAny(ewc::VERTSHADER | ewc::FRAGSHADER)) {
        flGraphState.set(em::UPD_SHAD_V | em::UPD_SHAD_F);
        flGraphState.set(em::UPD_UNI_MATHS | em::UPD_UNI_COLOUR);
    }

    if (flWaveCfg.hasAny(ewc::ORBITS | ewc::RESOLUTION | ewc::SPHERE)) {
        flGraphState.set(em::UPD_VBO | em::UPD_EBO);
    } else if (flGraphState.hasAny(em::UPD_VBO)) {
        flGraphState.set(em::UPD_VBO);
    } else if (flGraphState.hasAny(em::UPD_EBO)) {
        flGraphState.set(em::UPD_EBO);
    }

    return flGraphState.bf;
}

void WaveManager::create() {
    for (int i = 0; i < cfg.waves; i++) {
        waveVertices.push_back(new vVec3);
        waveIndices.push_back(new uvec);
        phase_const.push_back(phase_base * i);
    
        if (cfg.sphere) {
            if (cfg.cpu)
                updateWaveCPUSphere(i, 0);
            else
                sphereWaveGPU(i);
        } else {
            if (cfg.cpu)
                updateWaveCPUCircle(i, 0);
            else
                circleWaveGPU(i);
        }
    }

    genVertexArray();
    genIndexBuffer();
}

void WaveManager::update(double time) {
    allVertices.clear();
    
    for (int i = 0; i < cfg.waves; i++) {
        if (renderedWaves & RENDORBS[i]) {
            waveVertices[i]->clear();

            if (cfg.sphere)
                updateWaveCPUSphere(i, time);
            else
                updateWaveCPUCircle(i, time);
        }
    }

    genVertexArray();
}

void WaveManager::newWaves() {
    resetManager();
    create();
}

uint WaveManager::selectWaves(int id, bool checked) {
    uint flag = id;

    if (checked)
        renderedWaves |= flag;
    else
        renderedWaves &= ~(flag);

    allIndices.clear();
    genIndexBuffer();

    return em::UPD_EBO;
}

void WaveManager::circleWaveGPU(int idx) {
    double radius = (double) (idx + 1);
    // int l = idx * this->waveResolution;
    uint pixelCount = idx * this->waveResolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;     

        float h = (float) theta;
        float p = (float) phase_const[idx];
        float d = (float) radius;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;

        vec3 factorsA = vec3(h, p, d);
        vec3 factorsB = vec3(r, g, b);
        
        waveVertices[idx]->push_back(factorsA);
        waveVertices[idx]->push_back(factorsB);

        // waveIndices[idx]->push_back(l + i);
        waveIndices[idx]->push_back(pixelCount++);
    }
}

void WaveManager::sphereWaveGPU(int idx) {
    double radius = (double) (idx + 1);
    // int l = idx * this->waveResolution * this->waveResolution;
    uint pixelCount = idx * this->waveResolution * this->waveResolution;

    for (int i = 0; i < this->waveResolution; i++) {
        int m = i * this->waveResolution;
        for (int j = 0; j < this->waveResolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;

            float h = (float) theta;
            float p = (float) phi;
            float d = (float) radius;
            float r = (float) phase_const[idx];
            float g = 0;
            float b = 0;

            vec3 factorsA = vec3(h, p, d);
            vec3 factorsB = vec3(r, g, b);
            
            waveVertices[idx]->push_back(factorsA);
            waveVertices[idx]->push_back(factorsB);

            // waveIndices[idx]->push_back(l + m + j);
            waveIndices[idx]->push_back(pixelCount++);
            //std::cout << glm::to_string(factorsB) << "\n";
        }
    }
}

void WaveManager::updateWaveCPUCircle(int idx, double t) {
    double radius = (double) (idx + 1);
    uint pixelCount = idx * this->waveResolution;

    /* y = A * sin((two_pi_L * r * theta) - (two_pi_T * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;
        float x, y, z;

        double wavefunc = cos((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const[idx]);
        double displacement = this->waveAmplitude * wavefunc;

        if (cfg.parallel) {
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

        vec3 vertex = vec3(x, y, z);
        vec3 colour = vec3(r, g, b);
        
        waveVertices[idx]->push_back(vertex);
        waveVertices[idx]->push_back(colour);

        if (!t) {
            waveIndices[idx]->push_back(pixelCount++);
        }
    }
    if (cfg.superposition && idx > 0) {
        superposition(idx);
    }
}

void WaveManager::updateWaveCPUSphere(int idx, double t) {
    double radius = (double) (idx + 1);
    uint pixelCount = idx * this->waveResolution * this->waveResolution;

    for (int i = 0; i < this->waveResolution; i++) {
        int m = i * this->waveResolution;
        for (int j = 0; j < this->waveResolution; j++) {
            double theta = i * deg_fac;
            double phi = j * deg_fac;

            float wavefunc = cos((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const[idx]);
            float displacement = this->waveAmplitude * wavefunc;

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

            vec3 vertex = vec3(x, y, z);
            vec3 colour = vec3(r, g, b);
            
            waveVertices[idx]->push_back(vertex);
            waveVertices[idx]->push_back(colour);

            if (!t) {
                waveIndices[idx]->push_back(pixelCount++);
            }
        }
    }
    if (cfg.superposition && idx > 0) {
        superposition(idx);
    }
}

void WaveManager::superposition(int idx) {
    int verts = waveVertices[idx]->size();
    vec3 red = vec3(1.0f, 0.0f, 0.0f);

    for (int dt = 0; dt < verts; dt += 2) {
        vec3 &a = (*waveVertices[idx - 1])[dt];
        vec3 &b = (*waveVertices[idx])[dt];

        if (length(a) > length(b)) {
            // Calculate interference
            vec3 avg = (a + b) * 0.5f;

            // Adjust vertices for interference
            b = avg;
            a = avg;

            // Highlight adjusted vertices
            (*waveVertices[idx])[dt+1] = red;
            (*waveVertices[idx - 1])[dt+1] = red;
        }
    }
}

void WaveManager::resetManager() {
    for (auto v : waveVertices) {
        delete (v);
    }
    waveVertices.clear();
    for (auto c : waveIndices) {
        delete (c);
    }
    waveIndices.clear();

    allVertices.clear();
    allIndices.clear();
    phase_const.clear();

    this->vertexCount = 0;
    this->vertexSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;

    this->waveAmplitude = 0.0;
    this->waveResolution = 0;
    this->two_pi_L = 0.0;
    this->two_pi_T = 0.0;

    this->renderedWaves = 255;
    this->deg_fac = 0.0;
    this->phase_base = PI_TWO;
}

void WaveManager::genVertexArray() {
    for (int i = 0; i < cfg.waves; i++) {
        std::copy(waveVertices[i]->begin(), waveVertices[i]->end(), std::back_inserter(this->allVertices));
    }

    Manager::genVertexArray();
}

void WaveManager::genIndexBuffer() {
    assert(!allIndices.size());

    for (int i = 0; i < cfg.waves; i++) {
        if (renderedWaves & (1 << i)) {
            std::copy(waveIndices[i]->begin(), waveIndices[i]->end(), std::back_inserter(this->allIndices));
        }
    }

    Manager::genIndexBuffer();
}
