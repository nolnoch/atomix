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

#include <format>
#include <map>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include <complex>
#include "manager.hpp"

/*  Mac's Clang does not support Special Math functions from STL. Must use Boost, which is 4x slower  */
#ifdef MAC_OS
    #include <boost/math/special_functions.hpp>
    const inline auto& legp = static_cast<double(*)(int, int, double)>(boost::math::legendre_p);
    const inline auto& lagp = static_cast<double(*)(uint, uint, double)>(boost::math::laguerre);
#else
    const inline auto& legp = static_cast<double(*)(uint, uint, double)>(std::assoc_legendre);
    const inline auto& lagp = static_cast<double(*)(uint, uint, double)>(std::assoc_laguerre);
#endif

/*  BS::thread_pool Priority slows the program down for optional benefit of low/high prio  */
// #define BS_THREAD_POOL_ENABLE_PRIORITY
#include "BS_thread_pool.hpp"


using std::chrono::_V2::system_clock;
#define DSQ(a, b) (((a<<1)*(a<<1)) + b)

// N                                1   2   3   4   5    6    7    8
const uint cm_maxRadius[4][8] = { { 4, 12, 24, 40, 60,  85, 113, 146 },\
// E2                               1   3   5   8  11   13   17   20
                                  { 5, 15, 29, 48, 71,  98, 130, 166 },\
// E3                               2   3   5   7  10   12   15   17
                                  { 7, 18, 34, 55, 81, 110, 145, 183 },\
// E4                               1   3   5   7   8   11   13   16
                                  { 8, 21, 39, 62, 89, 121, 158, 199 } };


class CloudManager : public Manager {
    public:
        CloudManager(AtomixConfig *cfg, harmap &inMap, int numRecipes);
        virtual ~CloudManager();
        void newConfig(AtomixConfig *cfg) override;

        void initManager() override final;
        void receiveCloudMapAndConfig(AtomixConfig *config, harmap *inMap, int numRecipes);
        
        void cloudTest(int n_max);
        void cloudTestCSV();
        
        const uint getColourSize();
        const uint getMaxRadius(double tolerance, int n_max, int divisor);
        bool hasVertices();
        bool hasBuffers();

        void printRecipes();
        void printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP);

        void testThreadingInit();

    private:
        void receiveCloudMap(harmap *inMap, int numRecipes);

        double create() override final;
        double createThreaded();
        double bakeOrbitals();
        double bakeOrbitalsThreaded();
        double bakeOrbitalsThreadedAlt();
        double cullTolerance();
        double cullToleranceThreaded();
        double cullSlider();
        double cullSliderThreaded();

        void update(double time) override final;

        void clearForNext() override final;
        void resetManager() override final;
        
        double wavefuncRadial(int n, int l, double r);
        std::complex<double> wavefuncAngular(int l, int m_l, double theta, double phi);
        std::complex<double> wavefuncAngExp(int m_l, double theta);
        double wavefuncAngLeg(int l, int m_l, double phi);
        std::complex<double> wavefuncPsi(double radial, std::complex<double> angular);
        double wavefuncRDP(double R, double r, int l);
        double wavefuncPDV(std::complex<double> Psi, double r, int l);
        double wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi);
        void wavefuncNorms(int n);
        int64_t fact(int n);

        void genOrbital(int n, int l, int m_l, double weight);
        
        int setColourCount();
        int setColourSize();

        void printBuffer(fvec buf, std::string name);
        void printBuffer(uvec buf, std::string name);
        void printTimes();

        // void genOrbitalsThroughN(int n);
        // void genOrbitalsOfN(int n);
        // void genOrbitalsOfL(int n, int l);
        // void genOrbitalExplicit(int n, int l, int m_l);
        // void genColourArray();
        // void RDPtoColours();

        BS::thread_pool cloudPool;

        std::vector<vVec3 *> pixelColours;
        vVec3 allColours;
        dvec pdvStaging;
        uvec idxCulledTolerance;
        uvec idxCulledSlider; // Not needed with threading
        double allPDVMaximum;
        
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
        float cm_culled = 0;
        const int MAX_SHELLS = 8;

        uint cm_pixels;
        bool cm_threading = true;
        uint cm_pool = 0;
        uint cm_loop = 0;
        uint cm_vecs = 0;
        std::mutex cm_proc_coarse;
        std::mutex cm_proc_fine;
        std::array<double, 4> cm_times = { 0.0, 0.0, 0.0, 0.0 };
        std::array<std::string, 4> cm_labels = { "Create():        ", "BakeOrbitals():  ", "CullTolerance(): ", "CullSlider():    " };

        int cloudResolution = 0;
        int cloudLayerDivisor = 0;
        float cloudTolerance = 0.05f;

        uint printCounter = 0;
};

#endif