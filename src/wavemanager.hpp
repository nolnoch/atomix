/**
 * wavemanager.hpp
 *
 *    Created on: Oct 18, 2023
 *   Last Update: Sep 23, 2024
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

#ifndef WAVEMANAGER_H
#define WAVEMANAGER_H

#include "manager.hpp"

#define MASK 0xFF
#define SHIFT(a, b) (static_cast<float>((a >> b) & MASK) / MASK)

enum ewc { ORBITS = 1, AMPLITUDE = 2, PERIOD = 4, WAVELENGTH = 8, RESOLUTION = 16, PARALLEL = 32, SUPERPOSITION = 64, CPU = 128, SPHERE = 256, VERTSHADER = 512, FRAGSHADER = 1024 };


class WaveManager : public Manager {
    public:
        WaveManager();
        virtual ~WaveManager();
        void newConfig(AtomixWaveConfig *cfg);
        void receiveConfig(AtomixWaveConfig *config);
        void initManager() override final;

        double create() override final;
        void update(double time) override final;
        
        void selectWaves(int id, bool checked);
        void setTime(double inTime) { this->time = inTime; }

        void setPeak(uint colourBits) { this->waveColours.r = colourBits; }
        void setBase(uint colourBits) { this->waveColours.g = colourBits; }
        void setTrough(uint colourBits) { this->waveColours.b = colourBits; }

        uint getMode() { return this->cfg.parallel; }
        float getPhase() { return this->phase_base; }
        bool getSuperposition() { return this->cfg.superposition; }
        bool getSphere() { return this->cfg.sphere; }
        void getMaths(glm::vec3 &maths) { maths = this->waveMaths; }
        void getColours(glm::uvec3 &colours) { colours = this->waveColours; }

    private:
        void resetManager() override;

        void circleWaveGPU(int idx);
        void sphereWaveGPU(int idx);
        void updateWaveCPUCircle(int idx, double time);
        void updateWaveCPUSphere(int idx, double time);
        void superposition(int idx);

        void genVertexArray() override;
        void genIndexBuffer() override;
        
        AtomixWaveConfig cfg;
        
        std::vector<vVec3 *> waveVertices;
        std::vector<uvec *> waveIndices;
        std::vector<double> phase_const;

        glm::vec3 waveMaths = glm::vec3(0, 0, 0);
        glm::uvec3 waveColours = glm::uvec3(0xFF00FFFF, 0x0000FFFF, 0x00FFFFFF);
        glm::dvec3 waveMathsCPU = glm::dvec3(0, 0, 0);
        
        ushort renderedWaves = 255;
        
        int waveResolution;
        double phase_base = PI_TWO;
        double time = 0.0;
};

#endif