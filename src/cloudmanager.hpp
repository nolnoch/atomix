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
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

#define MASK 0xFF
#define SHIFT(a, b) (static_cast<float>((a >> b) & MASK) / MASK)
#define DSQ(a, b) (((a<<1)*(a<<1)) + b)

using gvec = std::vector<glm::vec3>;
using dvec = std::vector<glm::vec2>;
using fvec = std::vector<float>;
using ivec = std::vector<uint>;
using vec = glm::vec3;


class CloudManager {
    public:
        CloudManager(WaveConfig *cfg);
        virtual ~CloudManager();

        void createCloud();
        void updateCloud(double time);

        void genShell(int n, int l, int m_l);
        
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

        void RDPtoColours();

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
        double wavefuncRadial(int n, int l, double r);
        std::complex<double> wavefuncAngular(int l, int m_l, double theta, double phi);
        std::complex<double> wavefuncPsi(double radial, std::complex<double> angular);
        double wavefuncRDP(double R, double r, int l);
        double wavefuncRDP2(std::complex<double> Psi, double r, int l);
        double wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi);
        void wavefuncNorms(int n);

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

        int fact(int n);

        WaveConfig *config;
        std::vector<gvec *> pixelVertices;
        std::vector<gvec *> pixelColours;
        std::vector<fvec *> pixelRDPs;
        std::vector<ivec *> pixelIndices;
        std::vector<int> dirtyLayers;
        gvec allVertices;
        gvec allColours;
        fvec allRDPs;
        ivec allIndices;
        std::vector<double> phase_const;
        std::unordered_map<int, double> norm_constR;
        std::unordered_map<int, double> norm_constY;
        std::vector<float> max_RDPs;
        std::vector<float> max_rads;

        int atomZ = 1;
        const int MAX_SHELLS = 8;
        
        ushort renderedOrbits = 255;
        int orbitCount = 0;
        int vertexCount = 0;
        int vertexSize = 0;
        int colourCount = 0;
        int colourSize = 0;
        int RDPCount = 0;
        int RDPSize = 0;
        int indexCount = 0;
        int indexSize = 0;
        
        int resolution = 0;
        //double deg_fac = 0;
        double phase_base = PI_TWO;

        int cloudOrbitCount = 0;
        int cloudOrbitDivisor = 0;
        int cloudLayerCount = 0;
        double cloudLayerDelta = 0;

        bool update = false;
};

#endif