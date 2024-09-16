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
    vec colour = vec(0.0f);

    for (int k = 1; k <= cloudLayerCount; k++) {
        pixelVertices.push_back(new gvec);
        //pixelColours.push_back(new gvec);
        pixelRDPs.push_back(new fvec);
        pixelIndices.push_back(new ivec);
        
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->resolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;
        float rdp = 0;
    
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
                //pixelColours[lv]->push_back(colour);
                pixelRDPs[lv]->push_back(rdp);
            }
        }
    }

    genVertexArray();
    //genColourArray();
    wavefuncNorms(MAX_SHELLS);
}

/*
void CloudManager::orbit1s() {
    double Z = 1.0;                         // effective nuclear charge for that orbital in that atom
    double n = 1.0;                         // principal quantum number

    for (int i = 0; i < vertexCount; i ++) {
        int layer = ceil((sqrt(8.0*(i/10.0)+1.0)-1.0)/2.0);
        double radius = layer * cloudLayerDelta;
        //double rho = Z * radius / n;
        int lv = layer - 1;

        /*  FF0000 -> FFFF00 -> 00FF00 -> 00FFFF -> 0000FF -> FF00FF -> FFFFFF  

        /*                    R1s * Y1s = 2Z^(3/2)e^(-rho/2) * (1/sqrt(4PI))                 
        //double wavefunction = 2.0 * pow(Z,(3.0/2.0)) * pow(M_E,(-1 * rho)) * sqrt(1.0/(4.0 * M_PI));
        //double rdp = wavefunction * wavefunction * 4.0 * M_PI * radius * radius;
        float rdp = static_cast<float>(std::clamp((4.0 * exp(-2.0 * radius) * radius * radius), 0.0, 1.0));

        if (rdp >= 0.01f){
            allColours[i] = vec(rdp, rdp, rdp);
            pixelIndices[lv]->push_back(i);
        }
    }

    genIndexBuffer();
}
*/

void CloudManager::orbit1s() {
    double Z = 1.0;                         // effective nuclear charge for that orbital in that atom
    double n = 1.0;                         // principal quantum number

    for (int k = 1; k <= cloudLayerCount; k++) {
        double radius = k * cloudLayerDelta;
        //double rho = Z * radius / n;
        int lv = k - 1;
        int steps = k * this->cloudOrbitDivisor;
        int fdn = ((2*lv*lv*lv)+(3*lv*lv)+lv)/6*(this->cloudOrbitDivisor*this->cloudOrbitDivisor);
        
        //double wavefunction = 2.0 * pow(Z,(3.0/2.0)) * pow(M_E,(-1 * rho)) * sqrt(1.0/(4.0 * M_PI));
        //double rdp = wavefunction * wavefunction * 4.0 * M_PI * radius * radius;
        double wavefunction = 4.0 * exp(-2.0 * radius) * radius * radius;
        
        float rdp = static_cast<float>(std::clamp((wavefunction * wavefunction * 4.0), 0.0, 1.0));

        //std::cout << rdp << "\n";

        if (rdp >= 0.1f) {
        
            for (int i = 0; i < steps; i++) {
                int base = i * steps;
                for (int j = 0; j < steps; j++) {
                    int inLayerIdx = base + j;
                    int inCloudIdx = fdn + inLayerIdx;

                    (*this->pixelColours[lv])[inLayerIdx] = vec(rdp, rdp, rdp);
                    pixelIndices[lv]->push_back(inCloudIdx);
                }
            }
        }
    }

    genColourArray();
    genIndexBuffer();
}

void CloudManager::genShell(int n, int l, int m_l) {
    double max_R = 0, max_rdp = 0, max_Y = 0, max_rdp2 = 0, max_Psi = 0;
    double r_maxR = 0, r_maxRDP = 0, r_maxY = 0, r_maxRDP2 = 0, r_maxPsi = 0;
    std::vector<double> rdp_spread;
    int fdn = 0;


    for (int k = 1; k <= cloudLayerCount; k++) {
        double radius = k * this->cloudLayerDelta;
        int steps = radius * this->resolution;
        double deg_fac = TWO_PI / steps;
        int lv = k - 1;
        double fac = (this->resolution / this->cloudOrbitDivisor);

        double R = wavefuncRadial(n, l, radius);
        double rdp = wavefuncRDP(R, radius, l);

        if (R > max_R) {
            max_R = R;
            r_maxR = radius;
        }
        if (rdp > max_rdp) {
            max_rdp = rdp;
            r_maxRDP = radius;
        }
        rdp_spread.push_back(rdp);

        if (rdp > 0.01f) {

            for (int i = 0; i < steps; i++) {
                int base = i * steps;
                double theta = i * deg_fac;
                for (int j = 0; j < steps; j++) {
                    double phi = j * deg_fac;
                    int inLayerIdx = base + j;
                    int inCloudIdx = fdn + inLayerIdx;

                    std::complex<double> Y = wavefuncAngular(l, m_l, theta, phi);
                    std::complex<double> Psi = wavefuncPsi(R, Y);
                    double rdp2 = wavefuncRDP2(Psi, radius, l);

                    /*
                    if (Y.real() > max_Y) {
                        max_Y = Y.real();
                        r_maxY = radius;
                    }
                    if (Psi.real() > max_Psi){
                        max_Psi = Psi.real();
                        r_maxPsi = radius;
                    }
                    */
                    if (rdp2 > max_rdp2) {
                        max_rdp2 = rdp2;
                        r_maxRDP2 = radius;
                    }
                    
                    float safeRDP = static_cast<float>(std::clamp((rdp2 * 40), 0.0, 1.0));

                    if (safeRDP >= 0.1f) {
                        (*this->pixelColours[lv])[inLayerIdx] += vec(safeRDP);
                        pixelIndices[lv]->push_back(inCloudIdx);
                    }
                    
                }
            }
        }

        fdn += (steps * steps);
    }

    std::cout << "R: " << max_R << " at r=" << r_maxR << "\n";
    std::cout << "RDP: " << max_rdp << " at r=" << r_maxRDP << "\n";
    //std::cout << "Y: " << max_Y << " at r=" << r_maxY << "\n";
    //std::cout << "Psi: " << max_Psi << " at r=" << r_maxPsi << "\n";
    std::cout << "RDP2: " << max_rdp2 << " at r=" << r_maxRDP2 << std::endl;

    std::cout << "\nRDP per r: \n";
    int q = 1;
    for (auto it : rdp_spread) {
        double r = q++ * cloudLayerDelta;
        std::string index = std::format("{:.1f}", r);
        std::cout << index << ", " << it << "\n";
    }
    std::cout << std::endl;

    genColourArray();
    genIndexBuffer();
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

void CloudManager::genIndexBuffer() {
    allIndices.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelIndices[i]->begin(), pixelIndices[i]->end(), std::back_inserter(this->allIndices));
    }

    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();
}

int CloudManager::getVertexSize() {
    return this->vertexSize;
}

int CloudManager::getColourSize() {
    return this->colourSize;
}

int CloudManager::getIndexCount() {
    return this->indexCount;
}

int CloudManager::getIndexSize() {
    return this->indexSize;
}

int CloudManager::setVertexCount() {
    return allVertices.size();
}

int CloudManager::setVertexSize() {
    int chunks = vertexCount ?: setVertexCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

const float* CloudManager::getVertexData() {
    assert(colourCount);
    return glm::value_ptr(allVertices.front());
}

int CloudManager::setColourCount() {
    return allColours.size();
}

int CloudManager::setColourSize() {
    int chunks = colourCount ?: setColourCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

const float* CloudManager::getColourData() {
    assert(colourCount == vertexCount);
    return glm::value_ptr(allColours.front());
}

int CloudManager::setIndexCount() {
    return allIndices.size();
}

int CloudManager::setIndexSize() {
    int chunks = indexCount ?: setIndexCount();
    int chunkSize = sizeof(uint);

    //std::cout << "allIndices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

const uint* CloudManager::getIndexData() {
    if (!indexCount)
        return 0;

    return &allIndices[0];
}

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
