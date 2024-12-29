/**
 * cloudmanager.hpp
 *
 *    Created on: Sep 4, 2024
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>

#include "manager.hpp"

// Mac's Clang does not support Special Math functions from STL. Must use Boost, which is 4x slower 
// Got around this by yanking the STD math functions out and calling them directly.
#include "special.hpp"
const inline auto& legp = static_cast<double(*)(uint, uint, double)>(atomix::special::atomix_legendre);
const inline auto& lagp = static_cast<double(*)(uint, uint, double)>(atomix::special::atomix_laguerre);

// BS::thread_pool Priority slows the program down for optional benefit of low/high prio 
// #define BS_THREAD_POOL_ENABLE_PRIORITY
// #include "BS_thread_pool.hpp"


using std::chrono::steady_clock;
#define DSQ(a, b) (((a<<1)*(a<<1)) + b)

// N                                1   2   3   4   5    6    7    8
const uint cm_maxRadius[4][8] = { {  6, 13, 24, 39, 58,  80, 107, 138 },    // Tolerance = 0.1
                                  {  8, 17, 30, 47, 69,  94, 124, 158 },    // Tolerance = 0.01
                                  {  9, 20, 35, 55, 78, 106, 139, 175 },    // Tolerance = 0.001
                                  { 11, 23, 40, 61, 87, 117, 152, 191 } };  // Tolerance = 0.0001


class CloudManager : public Manager {
public:
    CloudManager();
    virtual ~CloudManager();
    void newConfig(AtomixCloudConfig *cfg);
    void receiveCloudMapAndConfig(AtomixCloudConfig *config, harmap *inMap, bool generator = true);
    void update(double time) override final;
    
    size_t getColourSize();
    int getMaxLayer(double tolerance, int n_max, int divisor);
    int getMaxRadius(double tolerance, int n_max);
    bool hasVertices();
    bool hasBuffers();

    void printRecipes();
    void printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP);

protected:
    

private:
    void initManager() override;
    void receiveCloudMap(harmap *inMap);

    double createThreaded();
    double bakeOrbitalsThreaded();
    double cullToleranceThreaded();
    double expandPDVsToColours();
    double cullSliderThreaded();

    void clearForNext() override;
    void resetManager() override;
    
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

    size_t setColourCount();
    size_t setColourSize();
    int countMapRecipes(harmap *inMap);

    void printBuffer(fvec buf, std::string name);
    void printBuffer(uvec buf, std::string name);
    void printTimes();

    void cloudTest(int n_max);
    void cloudTestCSV();
    void radialMaxCSV(fvec &vecPDV, int n_max);
    void testThreadingInit(AtomixCloudConfig *config, harmap *inMap);

    void genVertexArray() override final {Manager::genVertexArray();}
    void genDataBuffer() override final {Manager::genDataBuffer();}
    void genColourBuffer() override final {Manager::genColourBuffer();}
    void genIndexBuffer() override final {Manager::genIndexBuffer();}

    AtomixCloudConfig cfg;

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
    uint numOrbitals = 0;
    int atomZ = 1;
    int max_n = 0;
    int opt_max_radius = 0;
    float cm_culled = 0;
    const int MAX_SHELLS = 8;

    size_t cm_pixels;
    std::mutex cm_proc_coarse;
    std::mutex cm_proc_fine;
    std::array<double, 4> cm_times = { 0.0, 0.0, 0.0, 0.0 };
    std::array<std::string, 4> cm_labels = { "Create():        ", "BakeOrbitals():  ", "CullTolerance(): ", "CullSlider():    " };

    int cloudResolution = 0;
    int cloudLayerDivisor = 0;
    double cloudTolerance = 0.05;

    uint printCounter = 0;
};

#endif
