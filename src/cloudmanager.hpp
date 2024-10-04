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

#include <cmath>
#include <complex>
#include <format>
#include <map>
#include <unordered_set>
#include "manager.hpp"

#define DSQ(a, b) (((a<<1)*(a<<1)) + b)


class CloudManager : public Manager {
    public:
        CloudManager(AtomixConfig *cfg, harmap &inMap, int numRecipes);
        virtual ~CloudManager();
        void newConfig(AtomixConfig *cfg) override;
        void initManager() override final;
        
        uint receiveCloudMapAndConfig(AtomixConfig *cloudMap, harmap &inMap, int numRecipes);
        
        void cloudTest(int n_max);
        void cloudTestCSV();
        
        const uint getColourSize();

        bool hasVertices();
        bool hasBuffers();
        void printMaxRDP(const int &n, const int &l, const int &m_l, const double &maxRDP);
        void printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP);

    private:
        void receiveCloudMap(harmap &inMap, int numRecipes);

        void create() override final;
        void update(double time) override final;
        void bakeOrbitalsForRender();
        void cullRDPs();

        void clearForNext() override final;
        void resetManager() override final;
        
        double wavefuncRadial(int n, int l, double r);
        std::complex<double> wavefuncAngular(int l, int m_l, double theta, double phi);
        std::complex<double> wavefuncAngExp(int m_l, double theta);
        double wavefuncAngLeg(int l, int m_l, double phi);
        std::complex<double> wavefuncPsi(double radial, std::complex<double> angular);
        double wavefuncRDP(double R, double r, int l);
        double wavefuncRDP2(std::complex<double> Psi, double r, int l);
        double wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi);
        void wavefuncNorms(int n);
        int64_t fact(int n);

        void genOrbital(int n, int l, int m_l, double weight);
        
        int setColourCount();
        int setColourSize();

        void printBuffer(fvec buf, std::string name);
        void printBuffer(uvec buf, std::string name);

        // void genOrbitalsThroughN(int n);
        // void genOrbitalsOfN(int n);
        // void genOrbitalsOfL(int n, int l);
        // void genOrbitalExplicit(int n, int l, int m_l);
        // void genColourArray();
        // void RDPtoColours();

        std::vector<vVec3 *> pixelColours;
        vVec3 allColours;
        dvec rdpStaging;
        dvec shellRDPMaximaN;
        dvec shellRDPMaximaL;
        dvec shellRDPMaximaCum;
        float allRDPMaximum;
        
        std::unordered_map<int, double> norm_constR;
        std::unordered_map<int, double> norm_constY;
        harmap cloudOrbitals;
        
        uint colourCount = 0;
        uint colourSize = 0;
        uint64_t pixelCount = 0;
        
        int orbitalIdx = 0;
        int numOrbitals = 0;
        int atomZ = 1;
        int max_n = 0;
        const int MAX_SHELLS = 8;

        int cloudResolution = 0;
        int cloudLayerDivisor = 0;
        double cloudTolerance = 0.01;

        uint printCounter = 0;

        // N                               1   2   3   4   5    6    7    8
        const int cm_maxRadius[4][8] = { { 4, 12, 24, 40, 60,  85, 113, 146 },\
        // E2                              1   3   5   8  11   13   17   20
                                         { 5, 15, 29, 48, 71,  98, 130, 166 },\
        // E3                              2   3   5   7  10   12   15   17
                                         { 7, 18, 34, 55, 81, 110, 145, 183 },\
        // E4                              1   3   5   7   8   11   13   16
                                         { 8, 21, 39, 62, 89, 121, 158, 199 } };
};

#endif