/**
 * orbit.hpp
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

#ifndef ORBIT_H
#define ORBIT_H

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

/* Math constants */
const double TWO_PI = 2.0 * M_PI;   // 2pi is used a lot
const double H = 6.626070e-34;      // Planck's constant
const double C = 299792458;         // Speed of massless particles
const double HC = 1.98644586e-25;   // Convenience product of above

using gvec = std::vector<glm::vec3>;
using dvec = std::vector<glm::vec2>;
using ivec = std::vector<uint>;
using vec = glm::vec3;


class OrbitManager {
    public:
        OrbitManager(WaveConfig *cfg);
        virtual ~OrbitManager();

        void createOrbits();
        void updateOrbits(double time);
        
        void newOrbits();
        void newConfig(WaveConfig *cfg);
        
        int getVertexSize();
        int getIndexCount();
        int getIndexSize();
        const float* getVertexData();
        const uint* getIndexData();

        void printIndices();
        void printVertices();

        double amplitude = 0;
        double two_pi_L = 0;
        double two_pi_T = 0;

    private:
        void circleOrbitGPU(int idx);
        void sphereOrbitGPU(int idx);
        void updateOrbitCPUCircle(int idx, double time);
        void updateOrbitCPUSphere(int idx, double time);
        void proximityDetect(int idx);
        void superposition(int idx);

        void genVertexArray();
        void genIndexBuffer();
        void resetManager();
        
        int setVertexCount();
        int setVertexSize();
        int setIndexCount();
        int setIndexSize();

        WaveConfig *config;
        std::vector<gvec *> orbitVertices;
        std::vector<ivec *> orbitIndices;
        //dvec myComponents;
        //OrbitManager *priorOrbit = nullptr;
        gvec allVertices;
        ivec allIndices;

        int orbitCount = 0;
        int vertexCount = 0;
        int vertexSize = 0;
        int indexCount = 0;
        int indexSize = 0;

        int resolution = 0;
        
        double phase_const = 0;
        double deg_fac = 0;

        bool update = false;
};

#endif