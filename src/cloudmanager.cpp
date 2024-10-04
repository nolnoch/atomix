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

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>


CloudManager::CloudManager(AtomixConfig *cfg, harmap &inMap, int numRecipes) {
    newConfig(cfg);
    receiveCloudMap(inMap, numRecipes);
    mStatus.set(em::INIT);
}

CloudManager::~CloudManager() {
    resetManager();
}

void CloudManager::newConfig(AtomixConfig *config) {
    Manager::newConfig(config);

    this->cloudMaxRadius = 250;
    this->cloudLayerDivisor = cfg.cloudLayDivisor;
    this->cloudResolution = cfg.cloudResolution;
    this->cloudLayerCount = this->cloudMaxRadius * this->cloudLayerDivisor;
    this->cloudTolerance = cfg.cloudTolerance;
    this->deg_fac = TWO_PI / this->cloudResolution;
    this->cfg.vert = "gpu_harmonics.vert";
}

void CloudManager::initManager() {
    createCloud();
    bakeOrbitalsForRender();
    cullRDPs();
}

void CloudManager::createCloud() {
    this->max_n = cloudOrbitals.rbegin()->first;
    int divSciExp = std::abs(floor(log10(this->cloudTolerance)));
    int opt_max_radius = cm_maxRadius[divSciExp - 1][max_n - 1] * this->cloudLayerDivisor;
    int steps_local = this->cloudResolution;
    double deg_fac_local = this->deg_fac;
    vec3 pos = vec3(0.0f);

    for (int k = 1; k <= opt_max_radius; k++) {
        // double radius = ((5 * pow(2.0, ((40.0 + k) / 40.0))) / (log(2.0))) - (10.0 / log(2.0));  --  Scaling layer divisor equation
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;
        int lv = k - 1;

        for (int i = 0; i < steps_local; i++) {
            double theta = i * deg_fac_local;
            for (int j = 0; j < steps_local; j++) {
                double phi = j * deg_fac_local;

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
        shellRDPMaximaN.push_back(0.0);
        shellRDPMaximaL.push_back(0.0);
        shellRDPMaximaCum.push_back(0.0);
    }

    wavefuncNorms(MAX_SHELLS);
    genVertexArray();

    if (this->vertexCount > this->oldVERSize) {
        mStatus.set(em::INCR_VBO);
    }
    mStatus.set(em::VERT_READY);
}

void CloudManager::clearForNext() {
    mStatus.reset();

    std::fill(shellRDPMaximaN.begin(), shellRDPMaximaN.end(), 0.0);
    std::fill(shellRDPMaximaL.begin(), shellRDPMaximaL.end(), 0.0);
    std::fill(shellRDPMaximaCum.begin(), shellRDPMaximaCum.end(), 0.0);
    std::fill(allRDPs.begin(), allRDPs.end(), 0.0f);
    std::fill(rdpStaging.begin(), rdpStaging.end(), 0.0);
    allIndices.clear();
    cloudOrbitals.clear();
    this->orbitalIdx = 0;
    this->allRDPMaximum = 0;
    this->update = false;
    this->active = false;
    this->atomZ = 1;
    mStatus.setTo(em::INIT | em::VERT_READY);
}

void CloudManager::genOrbital(int n, int l, int m_l, double weight) {
    assert(mStatus.hasAny(em::VERT_READY));
    double max_rdp = 0;
    int pixelCount = 0;
    double deg_fac_local = this->deg_fac;
    int steps_local = this->cloudResolution;
    int divSciExp = std::abs(floor(log10(this->cloudTolerance)));
    int opt_max_radius = cm_maxRadius[divSciExp - 1][max_n - 1] * this->cloudLayerDivisor;

    for (int k = 1; k <= opt_max_radius; k++) {
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;
        double orbNorm = this->norm_constY[DSQ(l, m_l)];
        double R = wavefuncRadial(n, l, radius);
        double rdp = wavefuncRDP(R, radius, l);

        for (int i = 0; i < steps_local; i++) {
            double theta = i * deg_fac_local;
            std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
            for (int j = 0; j < steps_local; j++) {
                double phi = j * deg_fac_local;

                double orbLeg = wavefuncAngLeg(l, m_l, phi);
                std::complex<double> Y = orbExp * orbNorm * orbLeg;
                double rdp2 = wavefuncRDP2(R * Y, radius, l);

                if (rdp2 > max_rdp) {
                    max_rdp = rdp2;
                }
                rdpStaging[pixelCount++] += rdp2 * weight;

            }
        }
    }

    if (max_rdp > shellRDPMaximaN[n-1])
        shellRDPMaximaN[n-1] = max_rdp;
    if (max_rdp > shellRDPMaximaL[l])
        shellRDPMaximaL[l] = max_rdp;
}

/* void CloudManager::genOrbitalsThroughN(int n_max) {
    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            for (int m_l = l; m_l >= -l; m_l--) {
                cloudOrbitals[n].push_back(ivec2(l, m_l));
            }
        }
    }
    mStatus.set(em::NEW_CFG);
}

void CloudManager::genOrbitalsOfN(int n) {
    for (int l = n-1; l >= 0; l--) {
        for (int m_l = l; m_l >= -l; m_l--) {
            cloudOrbitals[n].push_back(ivec2(l, m_l));
        }
    }
    mStatus.set(em::NEW_CFG);
}

void CloudManager::genOrbitalsOfL(int n, int l) {

    for (int m_l = l; m_l >= -l; m_l--) {
        cloudOrbitals[n].push_back(ivec2(l, m_l));
    }
    mStatus.set(em::NEW_CFG);
}

void CloudManager::genOrbitalExplicit(int n, int l, int m_l) {
    cloudOrbitals[n].push_back(ivec2(l, m_l));
    mStatus.set(em::NEW_CFG);
} */

void CloudManager::bakeOrbitalsForRender() {
    assert(mStatus.hasAll(em::VERT_READY));

    if (numOrbitals) {
        std::cout << numOrbitals << " recipe(s) loaded. Begin processing..." << std::endl;
    } else {
        std::cout << "No recipes loaded. Aborting." << std::endl;
        return;
    }
    /* if (!(mStatus.hasAny(em::VERT_READY))) {
        std::cout << ">> Vertices not created. Pre-empting for cloud creation." << std::endl;
        createCloud();
        std::cout << "Resume processing..." << std::endl;
    } */

    // Iterate through stored recipes, grouped by N
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            genOrbital(key, v.x, v.y, (v.z / static_cast<double>(numOrbitals)));
            printMaxRDP(key, v.x, v.y, 0);
        }
        // For each N: Get max element from accumulated rdpStaging values
        // double thisMax = *std::max_element(rdpStaging.begin(), rdpStaging.end());

        // For each N: Normalize accumulated pixelRDPs, add to allRDPs, register indices, and reset pixelRDP vector
        /* for (uint p = 0; p < this->pixelCount; p++) {
            double new_val = rdpStaging[p];
            new_val /= thisMax;
            if (new_val > this->cloudTolerance) {
                allRDPs[p] += static_cast<float>(new_val);
                allIndices.push_back(p);
            }
            // rdpStaging[p] = 0.0;
        } */
    }
    mStatus.set(em::DATA_READY);    
}

void CloudManager::cullRDPs() {
    std::fill(allRDPs.begin(), allRDPs.end(), 0.0);
    allIndices.clear();

    // End: check actual value of accumulated allRDPs
    this->allRDPMaximum = *std::max_element(rdpStaging.begin(), rdpStaging.end());
    // std::cout << "Cumulative Max RDP for rdpStaging was: " << this->allRDPMaximum << std::endl;

    // End: Clamp all accumulated values in allRDPs to [0,1] or normalize by peak accumulated value
    // std::transform(allRDPs.cbegin(), allRDPs.cend(), allRDPs.begin(), [=](auto f){ return std::clamp(f, 0.0f, 1.0f); });
    // std::transform(allRDPs.cbegin(), allRDPs.cend(), allRDPs.begin(), [=, this](auto f){ return f / this->allRDPMaximum; });

    for (uint p = 0; p < this->pixelCount; p++) {
        double new_val = rdpStaging[p] / this->allRDPMaximum;
        if (new_val > this->cloudTolerance) {
            allRDPs[p] = static_cast<float>(new_val);
            allIndices.push_back(p);
        }
    }

    genRDPs();
    genIndexBuffer();

    mStatus.set(em::DATA_READY | em::IDX_READY);
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

void CloudManager::cloudTestCSV() {
    for (int n = 1; n <= 8; n++) {
        for (int l = 0; l <= n-1; l++) {
            for (int m_l = 0; m_l <= l; m_l++) {
                std::cout << n << l << m_l;

                for (int k = 1; k <= 250; k++) {
                    double max_rdp = 0;
                    int steps_local = this->cloudResolution;
                    double deg_fac_local = this->deg_fac;
                    double orbNorm = this->norm_constY[DSQ(l, m_l)];
                    double R = wavefuncRadial(n, l, k);
                    double rdp = wavefuncRDP(R, k, l);

                    for (int i = 0; i < steps_local; i++) {
                        double theta = i * deg_fac_local;
                        std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
                        for (int j = 0; j < steps_local; j++) {
                            double phi = j * deg_fac_local;

                            double orbLeg = wavefuncAngLeg(l, m_l, phi);
                            std::complex<double> Y = orbExp * orbNorm * orbLeg;
                            double rdp2 = wavefuncRDP2(R * Y, k, l);

                            if (rdp2 > max_rdp) {
                                max_rdp = rdp2;
                            }
                        }
                    }

                    std::cout << "," << max_rdp;
                }
                std::cout << "\n";
            }
            
        }
    }
    std::cout << std::endl;

}

void CloudManager::updateCloud(double time) {
    assert(mStatus.hasAll(em::VERT_READY | em::UPDATE_VBO | em::UPDATE_EBO));
    this->update = true;
    //TODO implement for CPU updates over time
}

void CloudManager::receiveCloudMap(harmap &inMap, int numRecipes) {
    this->cloudOrbitals = inMap;
    this->numOrbitals = numRecipes;
    mStatus.set(em::NEW_CFG);
}

uint CloudManager::receiveCloudMapAndConfig(AtomixConfig *config, harmap &inMap, int numRecipes) {
    uint flags = 0;
    
    // Check for relevant config changes OR for recipes to require larger radius
    bool newMap = cloudOrbitals != inMap;
    bool newDivisor = (this->cloudLayerDivisor != config->cloudLayDivisor);
    bool newResolution = (this->cloudResolution != config->cloudResolution);
    bool newTolerance = (this->cloudTolerance != config->cloudTolerance);
    bool higherMaxN = (mStatus.hasAny(em::VERT_READY)) && (inMap.rbegin()->first > this->max_n);
    bool newVerticesRequired = (newDivisor || newResolution || higherMaxN);

    // Resest or clear if necessary
    if (newVerticesRequired) {
        this->resetManager();
    } else if (newMap) {
        this->clearForNext();
    }
    
    // Update config and map
    this->newConfig(config);
    this->receiveCloudMap(inMap, numRecipes);

    // Regen vertices if necessary
    if (newVerticesRequired) {
        this->createCloud();
        flags |= em::UPDATE_VBO;
    }
    // Regen RDPS if necessary
    if (newVerticesRequired || newMap) {
        this->bakeOrbitalsForRender();
        flags |= em::UPDATE_DATA;
    }
    // Regen indices if necessary
    if (newVerticesRequired || newMap || newTolerance) {
        this->cullRDPs();
        flags |= em::UPDATE_EBO;
    }

    return flags;
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

    /* if (!l) {
        factor *= 4.0 * M_PI;
    } */

    return R * R * factor;
}

double CloudManager::wavefuncRDP2(std::complex<double> Psi, double r, int l) {
    double factor = r * r;

    /* if (!l) {
        factor *= 0.4 * M_PI;
    } */

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

/* void CloudManager::resetUpdates() {
    mStatus.reset();
} */

void CloudManager::resetManager() {
    Manager::resetManager();
    
    for (auto v : pixelColours) {
        delete (v);
    }
    this->pixelColours.clear();

    this->allColours.clear();
    this->allRDPs.clear();
    this->rdpStaging.clear();
    this->shellRDPMaximaN.clear();
    this->shellRDPMaximaL.clear();
    this->shellRDPMaximaCum.clear();
    this->norm_constR.clear();
    this->norm_constY.clear();
    this->cloudOrbitals.clear();

    this->pixelCount = 0;
    this->colourCount = 0;
    this->colourSize = 0;
    this->atomZ = 1;
    this->RDPCount = 0;
    this->RDPSize = 0;
    this->orbitalIdx = 0;
    this->cloudLayerCount = 0;
    this->cloudResolution = 0;
    this->allRDPMaximum = 0;
    this->numOrbitals = 0;
    this->cloudTolerance = 0.01;
    this->max_r = 0;
    this->max_theta = 0;
    this->max_phi = 0;
    this->mStatus.reset();
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

const uint CloudManager::getColourSize() {
    return this->colourSize;
}

const uint CloudManager::getRDPCount() {
    return this->RDPCount;
}

const uint CloudManager::getRDPSize() {
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

bool CloudManager::hasVertices() {
    return (this->mStatus.hasAny(em::VERT_READY));
}

bool CloudManager::hasBuffers() {
    return (this->mStatus.hasAny(em::UPDATE_EBO));
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
    // std::cout << std::setw(3) << ++(this->orbitalIdx) << ")  " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << " :: " << std::setw(9) << maxRDP\
    //  << " at (" << this->max_r << ", " << this->max_theta << ", " << this->max_phi << ")\n";

    std::cout << std::setw(3) << ++(this->orbitalIdx) << ")  " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << "\n";
}

void CloudManager::printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP) {
    std::cout << n << "," << l << "," << m_l << "," << maxRDP << "\n";
}

void CloudManager::printBuffer(fvec buf, std::string name) {
    std::ofstream outfile(name);

    if (outfile.is_open()) {
        for (const auto& f : buf) {
            outfile << f;
        }
        outfile << std::endl;
        outfile.close();
    } else {
        std::cout << "Failed to open file." << std::endl;
    }
}

void CloudManager::printBuffer(uvec buf, std::string name) {
    std::ofstream outfile(name);

    if (outfile.is_open()) {
        for (const auto& u : buf) {
            outfile << u;
        }
        outfile << std::endl;
        outfile.close();
    } else {
        std::cout << "Failed to open file." << std::endl;
    }
}
