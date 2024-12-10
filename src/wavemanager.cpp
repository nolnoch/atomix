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


WaveManager::WaveManager() {
}

WaveManager::~WaveManager() {
    resetManager();
}

void WaveManager::newConfig(AtomixWaveConfig *config) {
    this->cfg.waves = config->waves;
    this->cfg.amplitude = config->amplitude;
    this->cfg.period = config->period;
    this->cfg.wavelength = config->wavelength;
    this->cfg.resolution = config->resolution;
    this->cfg.parallel = config->parallel;
    this->cfg.superposition = config->superposition;
    this->cfg.cpu = config->cpu;
    this->cfg.sphere = config->sphere;
    this->cfg.visibleOrbits = config->visibleOrbits;

    this->waveResolution = cfg.resolution;
    this->waveMathsCPU.x = TWO_PI / this->cfg.wavelength;
    this->waveMathsCPU.y = TWO_PI / this->cfg.period;
    this->waveMathsCPU.z = cfg.amplitude;
    this->waveMaths.x = waveMathsCPU.x;
    this->waveMaths.y = waveMathsCPU.y;
    this->waveMaths.z = waveMathsCPU.z;
    this->deg_fac = TWO_PI / this->waveResolution;
}

void WaveManager::initManager() {
    create();
    mStatus.set(eInitFlags);
}

void WaveManager::receiveConfig(AtomixWaveConfig *config) {
    BitFlag flWaveCfg;

    if (mStatus.hasNone(em::INIT)) {
        newConfig(config);
        create();
        mStatus.set(eInitFlags | em::INIT);
        return;
    }
    
    // Check for relevant config changes OR for recipes to require larger radius
    if (cfg.waves != config->waves) {                    // Requires new {Vertices[cpu/gpu], VBO, IBO}
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
    if (cfg.resolution != config->resolution) {          // Requires new {Vertices[cpu/gpu], VBO, IBO}
        flWaveCfg.set(ewc::RESOLUTION);
    }
    if (cfg.parallel != config->parallel) {              // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::PARALLEL);
    }
    if (cfg.superposition != config->superposition) {    // Requires new {Vertices[cpu]}
        flWaveCfg.set(ewc::SUPERPOSITION);
    }
    if (cfg.cpu != config->cpu) {                        // Requires new {Vertices[cpu/gpu], VBO, IBO}
        flWaveCfg.set(ewc::CPU);
    }
    if (cfg.sphere != config->sphere) {                  // Requires new {Vertices[cpu/gpu], VBO, IBO}
        flWaveCfg.set(ewc::SPHERE);
    }

    if ((flWaveCfg.hasAny(ewc::ORBITS | ewc::RESOLUTION | ewc::SPHERE | ewc::CPU)) ||
        (this->cfg.cpu && (flWaveCfg.hasAny(ewc::AMPLITUDE | ewc::PERIOD | ewc::WAVELENGTH | ewc::PARALLEL)))) {
        resetManager();
        this->newConfig(config);
        this->create();
        mStatus.set(em::UPD_VBO | em::UPD_IBO);
    } else {
        this->newConfig(config);
    }

    if (flWaveCfg.hasAny(ewc::SPHERE)) {
        mStatus.set(em::UPD_SHAD_V);
    }

    if (flWaveCfg.hasAny(ewc::SUPERPOSITION | ewc::CPU)) {
        mStatus.set(em::UPD_SHAD_V);
    }

    if (flWaveCfg.hasAny(ewc::AMPLITUDE | ewc::PERIOD | ewc::WAVELENGTH)) {
        mStatus.set(em::UPD_UNI_MATHS);
    }

    if (flWaveCfg.hasAny(ewc::PARALLEL)) {
        mStatus.set(em::UPD_PUSH_CONST);
    }

    if (flWaveCfg.hasAny(ewc::VERTSHADER)) {
        mStatus.set(em::UPD_SHAD_V);
        mStatus.set(em::UPD_UNI_MATHS | em::UPD_UNI_COLOUR);
    }
    if (flWaveCfg.hasAny(ewc::FRAGSHADER)) {
        mStatus.set(em::UPD_SHAD_F);
    }
}

double WaveManager::create() {
    int pixelCount = (waveResolution * ((cfg.sphere) ? waveResolution : 1));

    for (int i = 0; i < cfg.waves; i++) {
        waveVertices.push_back(new vVec3((pixelCount << int(cfg.cpu)), glm::vec3(0.0, 0.0, 0.0)));
        waveIndices.push_back(new uvec(pixelCount, 0));
        phase_const.push_back(phase_base * i);
        double startTime = (this->time) ? (this->time) : 0.0;

        if (cfg.sphere) {
            if (cfg.cpu)
                updateWaveCPUSphere(i, startTime);
            else
                sphereWaveGPU(i);
        } else {
            if (cfg.cpu)
                updateWaveCPUCircle(i, startTime);
            else
                circleWaveGPU(i);
        }
    }
    mStatus.set(em::VERT_READY | em::INDEX_READY);

    if (!init) {
        init = true;
    }

    if (cfg.cpu) {
        mStatus.set(em::CPU_RENDER);
    }

    genVertexArray();
    genIndexBuffer();
    return 0.0;
}

void WaveManager::update(double inTime) {
    Manager::update(inTime);
    if (!cfg.cpu) {
        return;
    }
    allVertices.clear();
    
    for (int i = 0; i < cfg.waves; i++) {
        if (renderedWaves & RENDORBS[i]) {
            if (cfg.sphere)
                updateWaveCPUSphere(i, inTime);
            else
                updateWaveCPUCircle(i, inTime);
        }
    }
    mStatus.set(em::VERT_READY | em::CPU_RENDER);

    genVertexArray();
}

void WaveManager::selectWaves(int id, bool checked) {
    uint flag = id;

    if (checked)
        this->cfg.visibleOrbits |= flag;
    else
        this->cfg.visibleOrbits &= ~flag;

    mStatus.set(em::INDEX_READY);
    genIndexBuffer();
}

void WaveManager::circleWaveGPU(int idx) {
    double radius = (double) (idx + 1);
    // int l = idx * this->waveResolution;
    uint idxCount = idx * this->waveResolution;
    uint pxCount = 0;

    /* y = A * sin((this->waveMaths.x * r * theta) - (this->waveMaths.y * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;     

        float h = (float) theta;
        float p = (float) phase_const[idx];
        float d = (float) radius;

        vec3 factorsA = vec3(h, p, d);
        
        (*waveVertices[idx])[pxCount] = factorsA;

        (*waveIndices[idx])[pxCount++] = idxCount++;
    }
}

void WaveManager::sphereWaveGPU(int idx) {
    double radius = double(idx + 1);
    uint idxCount = idx * this->waveResolution * this->waveResolution;
    uint pxCount = 0;

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;
        for (int j = 0; j < this->waveResolution; j++) {
            double phi = j * deg_fac;

            float h = (float) theta;
            float p = (float) phi;
            float d = (float) radius;

            vec3 factorsA = vec3(h, p, d);
            
            (*waveVertices[idx])[pxCount] = factorsA;

            (*waveIndices[idx])[pxCount++] = idxCount++;
        }
    }
}

void WaveManager::updateWaveCPUCircle(int idx, double t) {
    double radius = (double) (idx + 1);
    uint idxCount = idx * this->waveResolution;
    uint pxCount = 0;
    uint idxIdx = 0;
    uint peak = this->waveColours.r;
    uint base = this->waveColours.g;
    uint trough = this->waveColours.b;

    double two_pi_L = this->waveMathsCPU.x;
    double two_pi_T = this->waveMathsCPU.y;
    double amp = this->cfg.amplitude;

    /* y = A * sin((this->waveMaths.x * r * theta) - (this->waveMaths.y * t) + (p = 0)) */
    /* y = A * sin(  (  k   *   x )    -    (   w   *  t )   +   p    */
    /* y = A * sin(  ( p/h  *   x )    -    (  1/f  *  t )   +   p    */
    /* y = A * sin(  ( E/hc  *  x )    -    (  h/E  *  t )   +   p    */

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;
        float x, y, z, r, g, b, scale;

        double wavefunc = cos((two_pi_L * radius * theta) - (two_pi_T * t) + phase_const[idx]);
        double displacement = amp * wavefunc;

        if (cfg.parallel) {
            x = (displacement + radius) * cos(theta);
            y = 0.0f;
            z = (displacement + radius) * sin(theta);
        } else {
            x = radius * cos(theta);
            y = displacement;
            z = radius * sin(theta);
        }

        r = 1.0f;
        g = 1.0f;
        b = 1.0f;
        scale = abs(wavefunc);

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
        
        (*waveVertices[idx])[pxCount++] = vertex;
        (*waveVertices[idx])[pxCount++] = colour;

        if (!init) {
            (*waveIndices[idx])[idxIdx++] = idxCount++;
        }
    }
    if (cfg.superposition && idx > 0) {
        superposition(idx);
    }
}

void WaveManager::updateWaveCPUSphere(int idx, double t) {
    double radius = (double) (idx + 1);
    uint idxCount = idx * this->waveResolution * this->waveResolution;
    uint pxCount = 0;
    uint idxIdx = 0;
    uint peak = this->waveColours.r;
    uint base = this->waveColours.g;
    uint trough = this->waveColours.b;

    double two_pi_L = this->waveMathsCPU.x;
    double two_pi_T = this->waveMathsCPU.y;
    double amp = this->cfg.amplitude;

    for (int i = 0; i < this->waveResolution; i++) {
        double theta = i * deg_fac;
        double r_theta = radius * theta;

        for (int j = 0; j < this->waveResolution; j++) {
            float x, y, z, r, g, b, scale;
            double phi = j * deg_fac;

            double wavefunc = cos((two_pi_L * r_theta) - (two_pi_T * t) + phase_const[idx]);
            double displacement = amp * wavefunc;

            x = (float) ((radius + displacement) * (sin(phi) * sin(theta)));
            y = (float) ((radius + displacement) * cos(phi));
            z = (float) ((radius + displacement) * (sin(phi) * cos(theta)));

            r = 0;
            g = 0;
            b = 0;
            
            scale = abs(wavefunc);

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
            
            (*waveVertices[idx])[pxCount++] = vertex;
            (*waveVertices[idx])[pxCount++] = colour;

            if (!init) {
                (*waveIndices[idx])[idxIdx++] = idxCount++;
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

    this->waveMaths.z = 0.0;
    this->waveResolution = 0;
    this->waveMaths.x = 0.0;
    this->waveMaths.y = 0.0;

    this->renderedWaves = 255;
    this->deg_fac = 0.0;
    this->phase_base = PI_TWO;
    this->init = false;

    this->mStatus.setTo(em::INIT);
}

void WaveManager::genVertexArray() {
    allVertices.clear();

    for (int i = 0; i < cfg.waves; i++) {
        std::copy(waveVertices[i]->begin(), waveVertices[i]->end(), std::back_inserter(this->allVertices));
    }

    Manager::genVertexArray();
}

void WaveManager::genIndexBuffer() {
    allIndices.clear();

    for (int i = 0; i < cfg.waves; i++) {
        if (this->cfg.visibleOrbits & (1 << i)) {
            std::copy(waveIndices[i]->begin(), waveIndices[i]->end(), std::back_inserter(this->allIndices));
            // If we choose to use lines, then we need primitive restart (0xFFFFFFFF)
        }
    }

    Manager::genIndexBuffer();
}
