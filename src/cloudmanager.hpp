/**
 * cloudmanager.hpp
 *
 *    Created on: Sep 4, 2024
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

#ifndef CLOUDMANAGER_H
#define CLOUDMANAGER_H

#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

#define MASK 0xFF
#define SHIFT(a, b) (static_cast<float>((a >> b) & MASK) / MASK)

using gvec = std::vector<glm::vec3>;
using dvec = std::vector<glm::vec2>;
using ivec = std::vector<uint>;
using vec = glm::vec3;


class CloudManager {
    public:
        CloudManager(WaveConfig *cfg);
        virtual ~CloudManager();

        void createCloud();
        void updateCloud(double time);

        void orbit1s();
        void orbit2p();
        
        void newConfig(WaveConfig *cfg);
        void newCloud();
        uint selectCloud(int id, bool checked);
        
        int getVertexSize();
        int getColourSize();
        int getIndexCount();
        int getIndexSize();
        const float* getVertexData();
        const float* getColourData();
        const uint* getIndexData();

        void printIndices();
        void printVertices();

        uint peak = 0xFF00FFFF;
        uint base = 0x0000FFFF;
        uint trough = 0x00FFFFFF;
        
        double amplitude = 0;
        double two_pi_L = 0;
        double two_pi_T = 0;

        bool testBool = false;

    private:
        void circleOrbitGPU(int idx);
        void sphereOrbitGPU(int idx);
        void updateOrbitCPUCircle(int idx, double time);
        void updateOrbitCPUSphere(int idx, double time);
        void superposition(int idx);

        void genVertexArray();
        void genColourArray();
        void genIndexBuffer();
        void resetManager();
        
        int setVertexCount();
        int setVertexSize();
        int setColourCount();
        int setColourSize();
        int setIndexCount();
        int setIndexSize();

        double wavefuncRadial(int n, int l, double r);
        double wavefuncSpherical(int l, int m, double theta, double phi);
        double wavefuncPsi(double R, double Y);
        double wavefuncRadialDistribution(double P, double r, int n);

        WaveConfig *config;
        std::vector<gvec *> pixelVertices;
        std::vector<gvec *> pixelColours;
        std::vector<ivec *> pixelIndices;
        std::vector<int> dirtyLayers;
        gvec allVertices;
        gvec allColours;
        ivec allIndices;
        std::vector<double> phase_const;
        
        ushort renderedOrbits = 255;
        int orbitCount = 0;
        int vertexCount = 0;
        int vertexSize = 0;
        int colourCount = 0;
        int colourSize = 0;
        int indexCount = 0;
        int indexSize = 0;
        
        int resolution = 0;
        double deg_fac = 0;
        double phase_base = PI_TWO;

        int cloudOrbitCount = 0;
        int cloudOrbitDivisor = 0;
        int cloudLayerCount = 0;
        double cloudLayerDelta = 0;

        bool update = false;
};

#endif