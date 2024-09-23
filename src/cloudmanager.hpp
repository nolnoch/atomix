/**
 * cloudmanager.hpp
 *
 *    Created on: Sep 4, 2024
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2024 Wade Burch (GPLv3)
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
#include <complex>
#include <format>
#include <map>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

#define MASK 0xFF
#define SHIFT(a, b) (static_cast<float>((a >> b) & MASK) / MASK)
#define DSQ(a, b) (((a<<1)*(a<<1)) + b)

using vVec3 = std::vector<glm::vec3>;
using vVec2 = std::vector<glm::vec2>;
using fvec = std::vector<float>;
using dvec = std::vector<double>;
using uvec = std::vector<uint>;
using vec3 = glm::vec3;
using ivec2 = glm::ivec2;
using fset = std::unordered_set<float>;


class CloudManager {
    public:
        CloudManager(WaveConfig *cfg);
        virtual ~CloudManager();
        
        void updateCloud(double time);

        void genOrbitalsThroughN(int n);
        void genOrbitalsOfN(int n);
        void genOrbitalExplicit(int n, int l, int m_l);
        void bakeOrbitalsForRender();
        void cloudTest(int n_max);
        
        void newConfig(WaveConfig *cfg);
        void newCloud();
        
        void RDPtoColours();

        uint getVertexSize();
        uint getColourSize();
        uint getRDPSize();
        uint getIndexSize();
        uint getIndexCount();
        const float* getVertexData();
        const float* getColourData();
        const float* getRDPData();
        const uint* getIndexData();

        void printMaxRDP(const int &n, const int &l, const int &m_l, const double &maxRDP);
        void printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP);
        void printIndices();
        void printVertices();

        double amplitude = 0;
        double two_pi_L = 0;
        double two_pi_T = 0;

        bool testBool = false;
        double max_r = 0, max_theta = 0, max_phi = 0;

    private:
        void createCloud();
        
        double wavefuncRadial(int n, int l, double r);
        std::complex<double> wavefuncAngular(int l, int m_l, double theta, double phi);
        std::complex<double> wavefuncAngExp(int m_l, double theta);
        double wavefuncAngLeg(int l, int m_l, double phi);
        std::complex<double> wavefuncPsi(double radial, std::complex<double> angular);
        double wavefuncRDP(double R, double r, int l);
        double wavefuncRDP2(std::complex<double> Psi, double r, int l);
        double wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi);
        void wavefuncNorms(int n);

        double genOrbital(int n, int l, int m_l);
        void genVertexArray();
        void genColourArray();
        void genRDPs();
        void genIndexBuffer();
        void resetManager();
        
        int setVertexCount();
        int setVertexSize();
        int setColourCount();
        int setColourSize();
        int setRDPCount();
        int setRDPSize();
        int setIndexCount();
        int setIndexSize();

        int64_t fact(int n);

        WaveConfig *config;
        std::vector<vVec3 *> pixelColours;
        dvec rdpStaging;
        dvec shellRDPMaxima;
        dvec shellRDPMaximaCum;
        float allRDPMaximum;
        
        vVec3 allVertices;
        vVec3 allColours;
        fvec allRDPs;
        uvec allIndices;
        
        std::unordered_map<int, double> norm_constR;
        std::unordered_map<int, double> norm_constY;

        std::unordered_set<int> activeShells;
        std::map<int, std::vector<glm::ivec2>> cloudOrbitals;

        int atomZ = 1;
        const int MAX_SHELLS = 8;
        
        uint vertexCount = 0;
        uint vertexSize = 0;
        uint colourCount = 0;
        uint colourSize = 0;
        uint RDPCount = 0;
        uint RDPSize = 0;
        uint indexCount = 0;
        uint indexSize = 0;
        uint64_t pixelCount = 0;
        
        int resolution = 0;
        int orbitalIdx = 0;
        double phase_base = PI_TWO;

        int cloudOrbitCount = 0;
        int cloudOrbitDivisor = 0;
        int cloudLayerCount = 0;
        double cloudLayerDelta = 0;

        bool update = false;
};

#endif