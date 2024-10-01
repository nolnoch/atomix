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

enum eStages { INIT = 1, VERTICES = 2, RECIPES = 4, VBO = 8, EBO = 16, NEW_CFG = 32 };


class CloudManager : public Manager {
    public:
        CloudManager(AtomixConfig *cfg);
        virtual ~CloudManager();
        void newConfig(AtomixConfig *cfg) override;
        
        void createCloud();
        void updateCloud(double time);
        void receiveCloudMap(harmap &inMap, int numRecipes);
        void receiveCloudMapAndConfig(AtomixConfig *cloudMap, harmap &inMap, int numRecipes);
        void clearForNext();

        void genOrbitalsThroughN(int n);
        void genOrbitalsOfN(int n);
        void genOrbitalsOfL(int n, int l);
        void genOrbitalExplicit(int n, int l, int m_l);
        int bakeOrbitalsForRender();
        void cloudTest(int n_max);
        void cloudTestCSV();
        
        void RDPtoColours();

        uint getColourSize();
        uint getRDPSize();
        const float* getRDPData();

        bool hasVertices();
        bool hasBuffers();
        void printMaxRDP(const int &n, const int &l, const int &m_l, const double &maxRDP);
        void printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP);

        double max_r = 0, max_theta = 0, max_phi = 0;

    private:
        void resetManager() override;
        
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

        double genOrbital(int n, int l, int m_l);
        void genColourArray();
        void genRDPs();
        
        int setColourCount();
        int setColourSize();
        int setRDPCount();
        int setRDPSize();

        std::vector<vVec3 *> pixelColours;
        dvec rdpStaging;
        dvec shellRDPMaxima;
        dvec shellRDPMaximaCum;
        float allRDPMaximum;
        vVec3 allColours;
        fvec allRDPs;
        
        std::unordered_map<int, double> norm_constR;
        std::unordered_map<int, double> norm_constY;
        harmap cloudOrbitals;
        
        uint colourCount = 0;
        uint colourSize = 0;
        uint RDPCount = 0;
        uint RDPSize = 0;
        uint64_t pixelCount = 0;
        
        BitFlag flStages;

        int orbitalIdx = 0;
        int numOrbitals = 0;
        int atomZ = 1;
        int max_n = 0;
        const int MAX_SHELLS = 8;

        int cloudLayerCount = 0;
        int cloudResolution = 0;
        int cloudMaxRadius = 0;
        int cloudLayerDivisor = 0;
        double cloudTolerance = 0.01;
        const int cm_maxRadius[8] = { 5, 15, 30, 50, 75, 100, 130, 150 };
};

#endif