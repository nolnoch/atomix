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
#include <chrono>


CloudManager::CloudManager(AtomixConfig *cfg, harmap &inMap, int numRecipes) {
    newConfig(cfg);
    receiveCloudMap(&inMap, numRecipes);
    mStatus.set(em::INIT);
}

CloudManager::~CloudManager() {
    resetManager();
}

void CloudManager::newConfig(AtomixConfig *config) {
    Manager::newConfig(config);

    this->cloudLayerDivisor = cfg.cloudLayDivisor;
    this->cloudResolution = cfg.cloudResolution;
    this->cloudTolerance = cfg.cloudTolerance;
    this->deg_fac = TWO_PI / this->cloudResolution;
    this->cfg.vert = "gpu_harmonics.vert";
}

void CloudManager::initManager() {
    auto begin = std::chrono::high_resolution_clock::now(), end = std::chrono::high_resolution_clock::now();
    double times[4] = { 0.0, 0.0, 0.0, 0.0 };

    begin = std::chrono::high_resolution_clock::now();
    create();
    end = std::chrono::high_resolution_clock::now();
    times[0] = std::chrono::duration<double, std::milli>(end - begin).count();
    begin = std::chrono::high_resolution_clock::now();
    bakeOrbitalsForRender();
    // bakeOrbitalsForRenderAlt();
    end = std::chrono::high_resolution_clock::now();
    times[1] = std::chrono::duration<double, std::milli>(end - begin).count();
    begin = std::chrono::high_resolution_clock::now();
    cullPDVs();
    // cullPDVsAlt();
    end = std::chrono::high_resolution_clock::now();
    times[2] = std::chrono::duration<double, std::milli>(end - begin).count();
    begin = std::chrono::high_resolution_clock::now();
    cullIndices();
    // cullIndicesAlt();
    end = std::chrono::high_resolution_clock::now();
    times[3] = std::chrono::duration<double, std::milli>(end - begin).count();

    std::cout << "Functions took:\n";
    for (auto t : times) {
        std::cout << t << " ms\n";
    }
    std::cout << std::endl;
}

void CloudManager::create() {
    QMutexLocker lock(&mutex);
    assert(mStatus.hasFirstNotLast(em::INIT, em::VERT_READY));

    this->max_n = cloudOrbitals.rbegin()->first;
    int opt_max_radius = getMaxRadius(this->cloudTolerance, this->max_n, this->cloudLayerDivisor);
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    double deg_fac_local = this->deg_fac;

    this->pixelCount = opt_max_radius * theta_max_local * phi_max_local;
    allVertices.reserve(pixelCount);
    vec3 pos = vec3(0.0f);
    // allVertices.insert(allVertices.end(), pixelCount, pos);

    for (int k = 1; k <= opt_max_radius; k++) {
        // double radius = ((5 * pow(2.0, ((40.0 + k) / 40.0))) / (log(2.0))) - (10.0 / log(2.0));  --  Scaling layer divisor equation
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;
        int lv = k - 1;

        for (int i = 0; i < theta_max_local; i++) {
            double theta = i * deg_fac_local;
            for (int j = 0; j < phi_max_local; j++) {
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
            }
        }
    }

    /* QtConcurrent::blockingMap(allVertices, [=, this](glm::vec3 &pos){
        int i = &pos - &this->allVertices.at(0);
        int layer_size = theta_max_local * phi_max_local;
        int layer = (i / layer_size) + 1;
        int layer_pos = i % layer_size;
        float theta = (layer_pos / phi_max_local) * deg_fac_local;
        float phi = (layer_pos % phi_max_local) * deg_fac_local;
        float radius = static_cast<float>(layer) / this->cloudLayerDivisor;

        pos = glm::vec3(theta, phi, radius);
    }); */

    pdvStaging.insert(pdvStaging.end(), pixelCount, 0.0);
    allData.insert(allData.end(), pixelCount, 0.0f);
    indicesStaging.reserve(pixelCount);

    for (int i = 0; i < MAX_SHELLS; i++) {
        pdvMaximaN.push_back(0.0);
        pdvMaximaL.push_back(0.0);
        pdvMaximaCum.push_back(0.0);
    }
    wavefuncNorms(MAX_SHELLS);

    mStatus.advance(em::INIT, em::VERT_READY);
    genVertexArray();
}

void CloudManager::genOrbital(int n, int l, int m_l, double weight) {
    double deg_fac_local = this->deg_fac;
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    int divSciExp = std::abs(floor(log10(this->cloudTolerance)));
    int opt_max_radius = cm_maxRadius[divSciExp - 1][max_n - 1] * this->cloudLayerDivisor;
    double orbNorm = this->norm_constY[DSQ(l, m_l)];

    for (int k = 1; k <= opt_max_radius; k++) {
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;
        double R = wavefuncRadial(n, l, radius);
        for (int i = 0; i < theta_max_local; i++) {
            double theta = i * deg_fac_local;
            std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
            for (int j = 0; j < phi_max_local; j++) {
                double phi = j * deg_fac_local;
                int localCount = ((k-1) * theta_max_local * phi_max_local) + (i * phi_max_local) + j;

                double orbLeg = wavefuncAngLeg(l, m_l, phi);
                std::complex<double> Y = orbExp * orbNorm * orbLeg;
                double pdv = wavefuncPDV(R * Y, radius, l);

                pdvStaging[localCount] += (pdv * weight);
            }
        }
    }
}

void CloudManager::genOrbitalAlt(int n, int l, int m_l, double weight) {
    for (uint64_t i = 0; i < pixelCount; i++) {
        double theta = allVertices[i].x;
        double phi = allVertices[i].y;
        double radius = allVertices[i].z;

        double pdv = wavefuncPsi2(n, l, m_l, radius, theta, phi);
        pdvStaging[i] += (pdv * weight);
    }
}

double CloudManager::genOrbitalAlt2(double &item) {
    std::ptrdiff_t i = &item - &this->pdvStaging.at(0);
    double final_pdv = 0;
    double theta = allVertices[i].x;
    double phi = allVertices[i].y;
    double radius = allVertices[i].z;

    for (auto const &[key, val] : cloudOrbitals) {
        int n = key;
        for (auto const &v : val) {
            int l = v.x;
            int m_l = v.y;
            double weight = v.z / static_cast<double>(this->numOrbitals);

            double pdv = wavefuncPsi2(n, l, m_l, radius, theta, phi);
            final_pdv += pdv * weight;
        }
    }

    return final_pdv;
}

void CloudManager::bakeOrbitalsForRender() {
    assert(mStatus.hasNone(em::INIT) && mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_GEN));

    // Iterate through stored recipes, grouped by N
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            genOrbitalAlt(key, v.x, v.y, (v.z / static_cast<double>(numOrbitals)));
        }
    }

    // End: check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(pdvStaging.begin(), pdvStaging.end());
    mStatus.set(em::DATA_GEN);
}

void CloudManager::bakeOrbitalsForRenderAlt() {
    assert(mStatus.hasNone(em::INIT) && mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_GEN));

    QtConcurrent::blockingMap(pdvStaging, [this](double &item){
        std::ptrdiff_t i = &item - &this->pdvStaging.at(0);
        double final_pdv = 0;
        double theta = allVertices[i].x;
        double phi = allVertices[i].y;
        double radius = allVertices[i].z;
        double totalRecipes = static_cast<double>(this->numOrbitals);

        for (auto const &[key, val] : cloudOrbitals) {
            int n = key;
            for (auto const &v : val) {
                int l = v.x;
                int m_l = v.y;
                double weight = v.z / totalRecipes;

                double pdv = wavefuncPsi2(n, l, m_l, radius, theta, phi);
                final_pdv += pdv * weight;
            }
        }
        item = final_pdv;
    });

    // End: check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(pdvStaging.begin(), pdvStaging.end());
    mStatus.set(em::DATA_GEN);
}

void CloudManager::cullPDVs() {
    assert(mStatus.hasFirstNotLast(em::DATA_GEN, em::DATA_READY));

    std::fill(allData.begin(), allData.end(), 0.0);
    indicesStaging.clear();

    for (uint p = 0; p < this->pixelCount; p++) {
        double new_val = pdvStaging[p] / this->allPDVMaximum;
        if (new_val > this->cloudTolerance) {
            allData[p] = static_cast<float>(new_val);
            indicesStaging.push_back(p);
        }
    }
    allIndices.reserve(indicesStaging.size());

    mStatus.set(em::DATA_READY);
    genDataBuffer();
}

void CloudManager::cullPDVsAlt() {
    assert(mStatus.hasFirstNotLast(em::DATA_GEN, em::DATA_READY));

    std::fill(allData.begin(), allData.end(), 0.0);
    indicesStaging.clear();

    /* The joys of debugging segfaults and exceptions in asynchronous, non-static, Filter and Map functions called via lambdas. */

    QtConcurrent::blockingMap(pdvStaging, [this](const double &item){
        std::ptrdiff_t index = &item - &this->pdvStaging.at(0);
        double new_val = item / this->allPDVMaximum;
        if (new_val > this->cloudTolerance) {
            allData[index] = static_cast<float>(new_val);
        }
    });

    for (uint p = 0; p < this->pixelCount; p++) {
        if (allData[p]) {
            indicesStaging.push_back(p);
        }
    }
    allIndices.reserve(indicesStaging.size());

    mStatus.set(em::DATA_READY);
    genDataBuffer();
}

void CloudManager::cullIndices() {
    assert(mStatus.hasNone(em::INDEX_READY));
    allIndices.clear();

    if (!cm_culled) {
        std::copy(indicesStaging.cbegin(), indicesStaging.cend(), std::back_inserter(allIndices));
    } else {
        uint layer_size = cloudResolution * (cloudResolution >> 1);
        uint culled_size = layer_size * cm_culled;
        uint idxEnd = indicesStaging.size();

        for (uint i = 0; i < idxEnd; i++) {
            if ((indicesStaging[i] % layer_size) < culled_size) {
                continue;
            } else {
                allIndices.push_back(indicesStaging[i]);
            }
        }
    }

    mStatus.set(em::INDEX_READY);
    genIndexBuffer();
}

void CloudManager::cullIndicesAlt() {
    assert(mStatus.hasNone(em::INDEX_READY));
    allIndices.clear();
    // QFuture<uint> newIndices;

    if (!cm_culled) {
        std::copy(indicesStaging.cbegin(), indicesStaging.cend(), std::back_inserter(allIndices));
    } else {
        // newIndices = QtConcurrent::filtered(indicesStaging, filterIndices);
        // newIndices = QtConcurrent::blockingFiltered(indicesStaging, [this](uint item){ return this->filterIndices(item); });
        // allIndices = QtConcurrent::blockingFiltered(indicesStaging, [this](uint item){ return this->filterIndices(item); });
        allIndices = QtConcurrent::blockingFiltered(indicesStaging, [this](const uint &item){
            uint layer_size = cloudResolution * (cloudResolution >> 1);
            uint culled_size = layer_size * cm_culled;
            return ((item % layer_size) < culled_size);
        });
    }
    // QList<uint> listAllIndices = newIndices.results();
    // std::copy(listAllIndices.begin(), listAllIndices.end(), std::back_inserter(allIndices));

    mStatus.set(em::INDEX_READY);
    genIndexBuffer();
}

bool CloudManager::filterPDVs(double item) {
    double new_val = item / this->allPDVMaximum;
    bool captured = (new_val > this->cloudTolerance);

    if (captured) {
        std::ptrdiff_t idx = std::distance(&this->pdvStaging[0], &item);
        // std::ptrdiff_t idx = &item - &this->pdvStaging.at(0);
        allData[idx] = static_cast<float>(new_val);
        indicesStaging.push_back(idx);
    }

    return captured;
}

bool CloudManager::filterIndices(const uint &item) {
    uint layer_size = cloudResolution * (cloudResolution >> 1);
    uint culled_size = layer_size * cm_culled;

    return ((item % layer_size) < culled_size);
}

void CloudManager::update(double time) {
    assert(mStatus.hasAll(em::VERT_READY | em::UPD_VBO | em::UPD_EBO));
    //TODO implement for CPU updates over time
}

void CloudManager::receiveCloudMap(harmap *inMap, int numRecipes) {
    this->cloudOrbitals = *inMap;
    this->numOrbitals = numRecipes;
    this->max_n = cloudOrbitals.rbegin()->first;
}

void CloudManager::receiveCloudMapAndConfig(AtomixConfig *config, harmap *inMap, int numRecipes) {
    // Check for relevant config changes OR for recipes to require larger radius
    bool newMap = cloudOrbitals != (*inMap);
    bool newDivisor = (this->cloudLayerDivisor != config->cloudLayDivisor);
    bool newResolution = (this->cloudResolution != config->cloudResolution);
    bool newTolerance = (this->cloudTolerance != config->cloudTolerance);
    bool higherMaxN = (mStatus.hasAny(em::VERT_READY)) && (inMap->rbegin()->first > this->max_n);
    bool configChanged = (newDivisor || newResolution || newTolerance);
    bool newVerticesRequired = (newDivisor || newResolution || higherMaxN);

    // Resest or clear if necessary
    if (newVerticesRequired) {
        this->resetManager();
    } else if (newMap) {
        this->clearForNext();
    }

    // Update config and map
    if (configChanged) {
        this->newConfig(config);
    }
    // Mark for vecsAndMatrices update if map (orbital) has changed
    if (newMap) {
        this->receiveCloudMap(inMap, numRecipes);
        mStatus.set(em::UPD_MATRICES);
    }

    // Regen vertices if necessary
    if (newVerticesRequired) {
        mStatus.clear(em::VERT_READY);
        this->create();
    }
    // Regen RDPS if necessary
    if (newVerticesRequired || newMap) {
        mStatus.clear(em::DATA_GEN | em::DATA_READY);
        this->bakeOrbitalsForRender();
    }
    // Regen indices if necessary
    if (newVerticesRequired || newMap || newTolerance) {
        mStatus.clear(em::INDEX_READY);
        this->cullPDVs();
        this->cullIndices();
    }
}

void CloudManager::receiveCulling(float pct) {
    uint flags = 0;

    this->mStatus.clear(em::INDEX_READY);
    this->indexCount = 0;
    this->indexSize = 0;
    this->cm_culled = pct;

    this->cullIndices();
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
                    double max_pdv = 0;
                    int steps_local = this->cloudResolution;
                    double deg_fac_local = this->deg_fac;
                    double orbNorm = this->norm_constY[DSQ(l, m_l)];
                    double R = wavefuncRadial(n, l, k);

                    for (int i = 0; i < steps_local; i++) {
                        double theta = i * deg_fac_local;
                        std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
                        for (int j = 0; j < steps_local; j++) {
                            double phi = j * deg_fac_local;

                            double orbLeg = wavefuncAngLeg(l, m_l, phi);
                            std::complex<double> Y = orbExp * orbNorm * orbLeg;
                            double pdv = wavefuncPDV(R * Y, k, l);

                            if (pdv > max_pdv) {
                                max_pdv = pdv;
                            }
                        }
                    }

                    std::cout << "," << max_pdv;
                }
                std::cout << "\n";
            }

        }
    }
    std::cout << std::endl;

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

double CloudManager::wavefuncPDV(std::complex<double> Psi, double r, int l) {
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
    double factor = r * r;
    
    double R = wavefuncRadial(n, l, r);
    std::complex<double> Y = wavefuncAngular(l, m_l, theta, phi);
    std::complex<double> Psi = R * Y;

    return (std::conj(Psi) * Psi).real() * factor;
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

/* void CloudManager::RDPtoColours() {
    // genRDPs();
    // TODO implement RDP-to-Colour conversion for CPU



    for (auto f : max_rads) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;
    for (auto f : max_RDPs) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;

    std::vector<float>::iterator fIt = std::max_element(allData.begin(), allData.end());
    int idx = std::distance(allData.begin(), fIt);
    std::cout << "Max value " << *fIt << " at index " << idx << std::endl;

} */

/* void CloudManager::resetUpdates() {
    mStatus.reset();
} */

void CloudManager::clearForNext() {
    mStatus.reset();

    std::fill(pdvMaximaN.begin(), pdvMaximaN.end(), 0.0);
    std::fill(pdvMaximaL.begin(), pdvMaximaL.end(), 0.0);
    std::fill(pdvMaximaCum.begin(), pdvMaximaCum.end(), 0.0);
    std::fill(pdvStaging.begin(), pdvStaging.end(), 0.0);
    cloudOrbitals.clear();
    this->orbitalIdx = 0;
    this->allPDVMaximum = 0;
    this->atomZ = 1;
    mStatus.setTo(em::VERT_READY);
}

void CloudManager::resetManager() {
    Manager::resetManager();

    /* for (auto v : pixelColours) {
        delete (v);
    }
    this->pixelColours.clear();
    this->allColours.clear(); */

    this->pdvStaging.clear();
    this->norm_constR.clear();
    this->norm_constY.clear();

    this->pixelCount = 0;
    this->colourCount = 0;
    this->colourSize = 0;
    this->orbitalIdx = 0;
    this->allPDVMaximum = 0;
}

/*
 *  Generators
 */

/* void CloudManager::genColourArray() {
    allColours.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelColours[i]->begin(), pixelColours[i]->end(), std::back_inserter(this->allColours));
    }

    this->colourCount = setColourCount();
    this->colourSize = setColourSize();
} */

/*
 *  Getters -- Size
 */

const uint CloudManager::getColourSize() {
    return this->colourSize;
}

const uint CloudManager::getMaxRadius(double tolerance, int n_max, int divisor) {
    int divSciExp = std::abs(floor(log10(tolerance)));
    return cm_maxRadius[divSciExp - 1][n_max - 1] * divisor;
}

/*
 *  Getters -- Count
 */

/*
 *  Getters -- Data
 */

bool CloudManager::hasVertices() {
    return (this->mStatus.hasAny(em::VERT_READY));
}

bool CloudManager::hasBuffers() {
    return (this->mStatus.hasAny(em::UPD_EBO));
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

/*
 *  Setters -- Count
 */

int CloudManager::setColourCount() {
    return allColours.size();
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
