/**
 * cloudmanager.cpp
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

#include "cloudmanager.hpp"


CloudManager::CloudManager(AtomixConfig *cfg) {
    newConfig(cfg);
    this->cfg.vert = "gpu_sphere_test.vert";
    stageFlags |= flagStages::INIT;
}

CloudManager::~CloudManager() {
    resetManager();
}

void CloudManager::createCloud() {
    assert(stageFlags & flagStages::INIT);

    this->cloudOrbitCount = 150;
    this->cloudOrbitDivisor = 1;
    this->cloudResolution = 12;
    this->cloudLayerDelta = 1.0 / this->cloudOrbitDivisor;
    this->cloudLayerCount = this->cloudOrbitCount * this->cloudOrbitDivisor;
    vec3 pos = vec3(0.0f);

    for (int k = 1; k <= cloudLayerCount; k++) {
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->cloudResolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;

        for (int i = 0; i < steps; i++) {
            double theta = i * deg_fac;
            for (int j = 0; j < steps; j++) {
                double phi = j * deg_fac;

                if (cfg.cpu) {
                    pos.x = radius * sin(phi) * sin(theta);
                    pos.y = radius * cos(phi);
                    pos.z = radius * sin(phi) * cos(theta);
                } else {
                    pos.x = (float) theta;
                    pos.y = (float) phi;
                    pos.z = (float) radius;
                }

                allVertices.push_back(pos);
                allRDPs.push_back(0.0);
                rdpStaging.push_back(0.0);

                pixelCount++;
            }
        }
    }

    for (int i = 0; i < MAX_SHELLS; i++) {
        shellRDPMaxima.push_back(0.0);
        shellRDPMaximaCum.push_back(0.0);
    }

    wavefuncNorms(MAX_SHELLS);
    genVertexArray();
    stageFlags |= flagStages::VERTICES;
}

double CloudManager::genOrbital(int n, int l, int m_l) {
    assert(stageFlags & flagStages::VERTICES);
    int fdn = 0;
    double max_rdp = 0;

    for (int k = 1; k <= cloudLayerCount; k++) {
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->cloudResolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;
        double fac = (cfg.resolution / this->cloudOrbitDivisor);

        double orbNorm = this->norm_constY[DSQ(l, m_l)];
        double R = wavefuncRadial(n, l, radius);
        double rdp = wavefuncRDP(R, radius, l);

        if (rdp > 0.00001) {

            for (int i = 0; i < steps; i++) {
                int base = i * steps;
                double theta = i * deg_fac;
                std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
                for (int j = 0; j < steps; j++) {
                    double phi = j * deg_fac;
                    int inCloudIdx = fdn + base + j;
                    double orbLeg = wavefuncAngLeg(l, m_l, phi);

                    std::complex<double> Y = orbExp * orbNorm * orbLeg;
                    double rdp2 = wavefuncRDP2(R * Y, radius, l);

                    if (rdp2 > max_rdp) {
                        max_rdp = rdp2;
                        max_r = radius;
                        max_theta = theta;
                        max_phi = phi;
                    }

                    rdpStaging[inCloudIdx] += rdp2;
                }
            }
        }
        fdn += (steps * steps);
    }

    if (max_rdp > shellRDPMaxima[n-1])
        shellRDPMaxima[n-1] = max_rdp;

    return max_rdp;
}

void CloudManager::genOrbitalsThroughN(int n_max) {
    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            for (int m_l = l; m_l >= -l; m_l--) {
                cloudOrbitals[n].push_back(ivec2(l, m_l));
            }
        }
        activeShells.insert(n);
    }
    stageFlags |= flagStages::RECIPES;
}

void CloudManager::genOrbitalsOfN(int n) {
    for (int l = n-1; l >= 0; l--) {
        for (int m_l = l; m_l >= -l; m_l--) {
            cloudOrbitals[n].push_back(ivec2(l, m_l));
        }
    }
    activeShells.insert(n);
    stageFlags |= flagStages::RECIPES;
}

void CloudManager::genOrbitalExplicit(int n, int l, int m_l) {
    cloudOrbitals[n].push_back(ivec2(l, m_l));
    activeShells.insert(n);
    stageFlags |= flagStages::RECIPES;
}

int CloudManager::bakeOrbitalsForRender() {
    int nr = cloudOrbitals.size();
    if (nr) {
        std::cout << nr << " recipe(s) loaded. Begin processing..." << std::endl;
    } else {
        std::cout << "No recipes loaded. Aborting." << std::endl;
        return A_ERR;
    }
    if (!(stageFlags & flagStages::VERTICES)) {
        std::cout << ">> Vertices not created. Pre-empting for cloud creation." << std::endl;
        createCloud();
        std::cout << "Resume processing..." << std::endl;
    }

    // Iterate through stored recipes, grouped by N
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            double maxRDP = genOrbital(key, v.x, v.y);
            printMaxRDP(key, v.x, v.y, maxRDP);
        }
        // For each N: Get max element from accumulated rdpStaging values
        shellRDPMaximaCum[key - 1] = *std::max_element(rdpStaging.begin(), rdpStaging.end());

        // For each N: Normalize accumulated pixelRDPs, add to allRDPs, register indices, and reset pixelRDP vector
        for (uint p = 0; p < this->pixelCount; p++) {
            double new_val = rdpStaging[p] / shellRDPMaximaCum[key - 1];
            if (new_val > this->cloudTolerance) {
                allRDPs[p] += static_cast<float>(new_val);
                allIndices.push_back(p);
            }
            rdpStaging[p] = 0.0;
        }
    }
    // End: check actual value of accumulated allRDPs
    this->allRDPMaximum = *std::max_element(allRDPs.begin(), allRDPs.end());
    std::cout << "Cumulative Max RDP for allRDPs was: " << this->allRDPMaximum << std::endl;

    // End: Clamp all accumulated values in allRDPs to [0,1]
    std::transform(allRDPs.cbegin(), allRDPs.cend(), allRDPs.begin(), [=](auto f){ return std::clamp(f, 0.0f, 1.0f); });
    // std::transform(allRDPs.cbegin(), allRDPs.cend(), allRDPs.begin(), [=, this](auto f){ return f / this->allRDPMaximum; });

    // End: Reset state for next orbital calculation(s)
    std::fill(rdpStaging.begin(), rdpStaging.end(), 0.0);
    this->orbitalIdx = 0;
    cloudOrbitals.clear();

    genRDPs();
    stageFlags |= flagStages::VBO;
    genIndexBuffer();
    stageFlags |= flagStages::EBO;
    return flagExit::A_OKAY;
}

void CloudManager::cloudTest(int n_max) {
    int idx = 0;

    std::vector<int> cloudMap;

    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            std::cout << std::setw(3) << idx++ << ")   (" << n << " , " << l << ")\n        ";
            for (int m_l = l; m_l >= -l; m_l--) {
                std::cout << m_l << ((m_l == -l) ? "" : ", ");
                cloudMap.push_back( ((n<<2)*(n<<2)) + ((l<<1) * (l<<1)) + m_l );
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    for (const auto &i : cloudMap) {
        std::cout << i << ",";
    }
    std::cout << std::endl;

    std::sort(cloudMap.begin(), cloudMap.end());
    auto it = std::adjacent_find(cloudMap.begin(), cloudMap.end());
    std::cout << "Duplicates " << ((it == cloudMap.end()) ? "NOT found!" : "found.") << std::endl;
}

void CloudManager::updateCloud(double time) {
    assert(stageFlags & (flagStages::VERTICES | flagStages::VBO | flagStages::EBO));
    this->update = true;
    //TODO implement for CPU updates over time
}

void CloudManager::newCloud() {
    resetManager();
    //createCloud();
}

int64_t CloudManager::fact(int n) {
    int64_t prod = n ?: 1;
    int orig = n;

    while (n > 2) {
        prod *= --n;
    }

    /* if (orig && prod > std::numeric_limits<int>::max()) {
        std::cout << orig << "! was too large for [int] type." << std::endl;
        orig = 0;
    } */

    return prod;
}

double CloudManager::wavefuncRadial(int n, int l, double r) {
    double rho = (2.0 * atomZ * r) / n;
    int nl1 = n - l - 1;
    int l21 = 2 * l + 1;

    double laguerre = std::assoc_laguerre(nl1, l21, rho);
    double expFunc = exp(-rho/2.0);
    double rhol = pow(rho, l);

    return laguerre * rhol * expFunc * this->norm_constR[DSQ(n, l)];
}

std::complex<double> CloudManager::wavefuncAngular(int l, int m_l, double theta, double phi) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;

    double legendre = std::assoc_legendre(l, abs(m_l), cos(phi));
    ibase *= m_l * theta;
    std::complex<double> expFunc = exp(ibase);

    return expFunc * legendre * this->norm_constY[DSQ(l, m_l)];
}

std::complex<double> CloudManager::wavefuncAngExp(int m_l, double theta) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;
    return exp(ibase * (m_l * theta));
}

double CloudManager::wavefuncAngLeg(int l, int m_l, double phi) {
    return std::assoc_legendre(l, abs(m_l), cos(phi));
}

std::complex<double> CloudManager::wavefuncPsi(double radial, std::complex<double> angular) {
    return radial * angular;
}

double CloudManager::wavefuncRDP(double R, double r, int l) {
    double factor = r * r;

    if (!l) {
        factor *= 4.0 * M_PI;
    }

    return R * R * factor;
}

double CloudManager::wavefuncRDP2(std::complex<double> Psi, double r, int l) {
    double factor = r * r;

    if (!l) {
        factor *= 0.4 * M_PI;
    }

    /* std::complex<double> term1 = std::conj(Psi);
    std::complex<double> term2 = term1 * Psi;
    double term3 = term2.real(); */

    return (std::conj(Psi) * Psi).real() * factor;
}

double CloudManager::wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi) {
    std::complex<double> R = wavefuncRadial(n, l, r);
    std::complex<double> Y = wavefuncAngular(l, m_l, theta, phi);
    std::complex<double> Psi = R * Y;

    std::complex<double> term = std::conj(Psi) * Psi;

    return term.real();
}

void CloudManager::wavefuncNorms(int max_n) {
    int max_l = max_n - 1;

    for (int n = max_n; n > 0; n--) {
        double rho_r = (2.0 * this->atomZ) / n;
        for (int l = n-1; l >= 0; l--) {
            int nl1 = n - l - 1;
            int key = DSQ(n, l);

            double value = pow(rho_r, (3.0/2.0)) * sqrt(fact(nl1) / (2.0 * n * fact(n+l)));
            this->norm_constR[key] = value;
        }
    }
    for (int l = max_l; l >= 0; l--) {
        int l21 = 2 * l + 1;
        for (int m_l = -l; m_l <= l; m_l++) {
            int magM = abs(m_l);
            int key = DSQ(l, m_l);

            double value = sqrt((l21 / (4.0 * M_PI)) * (fact(l - magM) / static_cast<double>(fact(l + magM))));
            this->norm_constY[key] = value;
        }
    }
}

void CloudManager::RDPtoColours() {
    // genRDPs();
    // TODO implement RDP-to-Colour conversion for CPU


    /*
    for (auto f : max_rads) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;
    for (auto f : max_RDPs) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;

    std::vector<float>::iterator fIt = std::max_element(allRDPs.begin(), allRDPs.end());
    int idx = std::distance(allRDPs.begin(), fIt);
    std::cout << "Max value " << *fIt << " at index " << idx << std::endl;
    */
}

void CloudManager::resetManager() {
    for (auto v : pixelColours) {
        delete (v);
    }
    pixelColours.clear();

    allVertices.clear();
    allColours.clear();
    allIndices.clear();
    allRDPs.clear();
    rdpStaging.clear();
    shellRDPMaxima.clear();
    shellRDPMaximaCum.clear();
    norm_constR.clear();
    norm_constY.clear();
    activeShells.clear();
    cloudOrbitals.clear();

    this->pixelCount = 0;
    this->vertexCount = 0;
    this->vertexSize = 0;
    this->colourCount = 0;
    this->colourSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;
    this->update = false;
    this->active = false;
    this->atomZ = 1;
    this->RDPCount = 0;
    this->RDPSize = 0;
    this->orbitalIdx = 0;
    this->cloudOrbitCount = 0;
    this->cloudOrbitDivisor = 0;
    this->cloudLayerCount = 0;
    this->cloudLayerDelta = 0.0;
    this->cloudResolution = 0;
    this->stageFlags = 0;
}

/*
 *  Generators
 */

void CloudManager::genColourArray() {
    allColours.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelColours[i]->begin(), pixelColours[i]->end(), std::back_inserter(this->allColours));
    }

    this->colourCount = setColourCount();
    this->colourSize = setColourSize();
}

void CloudManager::genRDPs() {
    this->RDPCount = setRDPCount();
    this->RDPSize = setRDPSize();
}

/*
 *  Getters -- Size
 */

uint CloudManager::getColourSize() {
    return this->colourSize;
}

uint CloudManager::getRDPSize() {
    return this->RDPSize;
}

/*
 *  Getters -- Count
 */

/*
 *  Getters -- Data
 */

const float* CloudManager::getRDPData() {
    assert(RDPCount);
    return &allRDPs[0];
}

/*
 *  Setters -- Size
 */

int CloudManager::setColourSize() {
    int chunks = colourCount ?: setColourCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

int CloudManager::setRDPSize() {
    int chunks = RDPCount ?: setRDPCount();
    int chunkSize = sizeof(float);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

/*
 *  Setters -- Count
 */

int CloudManager::setColourCount() {
    return allColours.size();
}

int CloudManager::setRDPCount() {
    return allRDPs.size();
}

/*
 *  Printers
 */

void CloudManager::printMaxRDP(const int &n, const int &l, const int &m_l, const double &maxRDP) {
    std::cout << std::setw(3) << ++(this->orbitalIdx) << ")  " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << " :: " << std::setw(9) << maxRDP\
     << " at (" << this->max_r << ", " << this->max_theta << ", " << this->max_phi << ")\n";
}

void CloudManager::printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP) {
    std::cout << n << "," << l << "," << m_l << "," << maxRDP << "\n";
}
