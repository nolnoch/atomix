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


CloudManager::CloudManager(WaveConfig *cfg)
    : config(cfg) {
    newConfig(cfg);
    createCloud();
    wavefuncNorms(MAX_SHELLS);
}

CloudManager::~CloudManager() {
    resetManager();
}

void CloudManager::createCloud() {
    this->cloudOrbitCount = 30;
    this->cloudOrbitDivisor = 2;
    this->cloudLayerDelta = 1.0 / this->cloudOrbitDivisor;
    this->cloudLayerCount = this->cloudOrbitCount * this->cloudOrbitDivisor;
    vec pos = vec(0.0f);

    for (int k = 1; k <= cloudLayerCount; k++) {
        pixelVertices.push_back(new vVec3);
        
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->resolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;
    
        for (int i = 0; i < steps; i++) {
            double theta = i * deg_fac;
            for (int j = 0; j < steps; j++) {
                double phi = j * deg_fac;

                if (config->cpu) {
                    pos.x = radius * sin(phi) * sin(theta);
                    pos.y = radius * cos(phi);
                    pos.z = radius * sin(phi) * cos(theta);
                } else {
                    pos.x = (float) theta;
                    pos.y = (float) phi;
                    pos.z = (float) radius;
                }
                
                pixelVertices[lv]->push_back(pos);
                pixelRDPs.push_back(0.0);
            }
        }
    }

    genVertexArray();
}

double CloudManager::genOrbital(int n, int l, int m_l) {
    int fdn = 0;
    double max_rdp = 0;
    double correction = (l) ? 1.0 : 0.1;

    for (int k = 1; k <= cloudLayerCount; k++) {
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->resolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;
        double fac = (this->resolution / this->cloudOrbitDivisor);

        double orbNorm = this->norm_constY[DSQ(l, m_l)];
        double R = wavefuncRadial(n, l, radius);
        double rdp = wavefuncRDP(R, radius, l);

        if (rdp > 0.01f) {

            for (int i = 0; i < steps; i++) {
                int base = i * steps;
                double theta = i * deg_fac;
                std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
                for (int j = 0; j < steps; j++) {
                    double phi = j * deg_fac;
                    int inCloudIdx = fdn + base + j;
                    
                    // double orbLeg = wavefuncAngLeg(l, m_l, phi);
                    std::complex<double> Y = orbExp * orbNorm * wavefuncAngLeg(l, m_l, phi);
                    double rdp2 = wavefuncRDP2(R * Y, radius, l) * correction;

                    pixelRDPs[inCloudIdx] += rdp2;

                    if (rdp2 > max_rdp) {
                        max_rdp = rdp2;
                    }
                }
            }
        }
        fdn += (steps * steps);
    }

    return max_rdp;
}

double CloudManager::genOrbitalsThroughN(int n_max, double opt_maxRDP) {
    double max_val = opt_maxRDP;
    int idx = 0;
    
    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            for (int m_l = l; m_l >= -l; m_l--) {
                double maxRDP = genOrbital(n, l, m_l);
                if (maxRDP > max_val)
                    max_val = maxRDP;
                std::cout << std::setw(3) << idx++ << ") " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << " :: " << maxRDP << "\n";
            }
        }
    }
    std::cout << std::endl;

    return max_val;
}

double CloudManager::genOrbitalsOfN(int n, double opt_maxRDP) {
    double max_val = opt_maxRDP;
    int idx = 0;
    
    for (int l = n-1; l > 0; l--) {
        for (int m_l = l; m_l >= -l; m_l--) {
            double maxRDP = genOrbital(n, l, m_l);
            if (maxRDP > max_val)
                max_val = maxRDP;
            std::cout << std::setw(3) << idx++ << ") " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << " :: " << maxRDP << "\n";
        }
    }
    std::cout << std::endl;

    return max_val;
}

double CloudManager::genOrbitalExplicit(int n, int l, int m_l, double opt_maxRDP) {
    double max_val = opt_maxRDP;
    double maxRDP = genOrbital(n, l, m_l);

    if (maxRDP > max_val) {
        max_val = maxRDP;
    }
    
    std::cout << std::setw(3) << "1) " << n << "  " << l << " " << ((m_l >= 0) ? " " : "") << m_l << " :: " << maxRDP << "\n";
    
    return max_val;
}

void CloudManager::bakeOrbitalsForRender(double maxRDP) {
    genRDPs(maxRDP);
    genIndexBuffer();
}

void CloudManager::cloudTest(int n_max) {
    int idx = 0;

    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            for (int m_l = l; m_l >= -l; m_l--) {
                std::cout << idx++ << ": " << n << "  " << l << " " << (m_l >= 0 ? " " : "") << m_l << "\n";
            }
        }
    }
    std::cout << std::endl;
}

void CloudManager::updateCloud(double time) {
    this->update = true;
    //TODO implement for CPU updates over time
}

void CloudManager::newConfig(WaveConfig *cfg) {
    this->config = cfg;
    this->orbitCount = config->orbits;
    this->amplitude = config->amplitude;
    this->resolution = config->resolution;
    this->two_pi_L = TWO_PI / config->wavelength;
    this->two_pi_T = TWO_PI / config->period;
    //this->deg_fac = TWO_PI / this->resolution;
}

void CloudManager::newCloud() {
    resetManager();
    //createCloud();
}

uint CloudManager::selectCloud(int id, bool checked) {
    uint flag = id;

    if (checked)
        renderedOrbits |= flag;
    else
        renderedOrbits &= ~(flag);

    allIndices.clear();
    genIndexBuffer();

    return renderedOrbits;
}

int CloudManager::fact(int n) {
    int prod = n ?: 1;
    
    while (n > 2) {
        prod *= --n;
    }

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
        factor *= 4.0 * M_PI;
    }

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
    for (auto v : pixelVertices) {
        delete (v);
    }
    pixelVertices.clear();
    for (auto v : pixelColours) {
        delete (v);
    }
    pixelColours.clear();
    for (auto c : pixelIndices) {
        delete (c);
    }
    pixelIndices.clear();

    allVertices.clear();
    allColours.clear();
    allIndices.clear();

    this->vertexCount = 0;
    this->vertexSize = 0;
    this->colourCount = 0;
    this->colourSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;
    this->update = false;
}

/* 
 *  Generators
 */

void CloudManager::genVertexArray() {
    allVertices.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelVertices[i]->begin(), pixelVertices[i]->end(), std::back_inserter(this->allVertices));
    }

    this->vertexCount = setVertexCount();
    this->vertexSize = setVertexSize();
}

void CloudManager::genColourArray() {
    allColours.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelColours[i]->begin(), pixelColours[i]->end(), std::back_inserter(this->allColours));
    }

    this->colourCount = setColourCount();
    this->colourSize = setColourSize();
}

void CloudManager::genRDPs(double max_val) {
    int pixels = this->pixelRDPs.size();
    
    for (int pIdx = 0; pIdx < pixels; pIdx++) {
        double new_val = pixelRDPs[pIdx] / max_val;
        allRDPs.push_back(static_cast<float>(new_val));

        if (new_val > 0.06) {
            allIndices.push_back(pIdx);
        }
    }

    this->RDPCount = setRDPCount();
    this->RDPSize = setRDPSize();
}

void CloudManager::genIndexBuffer() {
    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();
}

/* 
 *  Getters -- Size
 */

int CloudManager::getVertexSize() {
    return this->vertexSize;
}

int CloudManager::getColourSize() {
    return this->colourSize;
}

int CloudManager::getRDPSize() {
    return this->RDPSize;
}

int CloudManager::getIndexSize() {
    return this->indexSize;
}

/* 
 *  Getters -- Count
 */

int CloudManager::getIndexCount() {
    return this->indexCount;
}

/* 
 *  Getters -- Data
 */

const float* CloudManager::getVertexData() {
    assert(vertexCount);
    return glm::value_ptr(allVertices.front());
}

const float* CloudManager::getColourData() {
    assert(colourCount == vertexCount);
    return glm::value_ptr(allColours.front());
}

const float* CloudManager::getRDPData() {
    assert(RDPCount);
    return &allRDPs[0];
}

const uint* CloudManager::getIndexData() {
    assert(indexCount);
    return &allIndices[0];
}

/* 
 *  Setters -- Size
 */

int CloudManager::setVertexSize() {
    int chunks = vertexCount ?: setVertexCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

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

int CloudManager::setIndexSize() {
    int chunks = indexCount ?: setIndexCount();
    int chunkSize = sizeof(uint);

    //std::cout << "allIndices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

/* 
 *  Setters -- Count
 */

int CloudManager::setVertexCount() {
    return allVertices.size();
}

int CloudManager::setColourCount() {
    return allColours.size();
}

int CloudManager::setRDPCount() {
    return allRDPs.size();
}

int CloudManager::setIndexCount() {
    return allIndices.size();
}

/* 
 *  Printers
 */

void CloudManager::printIndices() {
    for (auto v : this->allIndices) {
        std::cout << v << ", ";
    }
    std::cout << std::endl;
}

void CloudManager::printVertices() {
    for (auto v : this->allVertices) {
        std::cout << glm::to_string(v) << ", ";
    }
    std::cout << std::endl;
}
