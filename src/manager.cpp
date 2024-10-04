/**
 * manager.cpp
 *
 *    Created on: Sep 23, 2024
 *   Last Update: Sep 23, 2024
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

#include "manager.hpp"


void Manager::newConfig(AtomixConfig *config) {
    this->cfg.waves = config->waves;
    this->cfg.amplitude = config->amplitude;
    this->cfg.period = config->period;
    this->cfg.wavelength = config->wavelength;
    this->cfg.resolution = config->resolution;
    this->cfg.parallel = config->parallel;
    this->cfg.superposition = config->superposition;
    this->cfg.cpu = config->cpu;
    this->cfg.sphere = config->sphere;
    this->cfg.vert = config->vert;
    this->cfg.frag = config->frag;

    this->cfg.cloudLayDivisor = config->cloudLayDivisor;
    this->cfg.cloudResolution = config->cloudResolution;
    this->cfg.cloudTolerance = config->cloudTolerance;
}

void Manager::resetManager() {
    allVertices.clear();
    allIndices.clear();

    this->vertexCount = 0;
    this->vertexSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;
}

void Manager::clearForNext() {
    this->resetManager();
}

/*
 *  Generators
 */

void Manager::genVertexArray() {
    this->vertexCount = setVertexCount();
    this->vertexSize = setVertexSize();

    // std::cout << "Vertex Array generation complete." << std::endl;
}

void Manager::genIndexBuffer() {
    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();

    // std::cout << "Index Buffer generation complete." << std::endl;
}

/*
 *  Getters -- Size
 */

const uint Manager::getVertexSize() {
    return this->vertexSize;
}

const uint Manager::getIndexSize() {
    return this->indexSize;
}

/*
 *  Getters -- Count
 */

const uint Manager::getVertexCount() {
    return this->vertexCount;
}

const uint Manager::getIndexCount() {
    return this->indexCount;
}

/*
 *  Getters -- Data
 */

const float* Manager::getVertexData() {
    assert(vertexCount);
    return glm::value_ptr(allVertices.front());
}

const uint* Manager::getIndexData() {
    assert(indexCount);
    return &allIndices[0];
}

std::string& Manager::getShaderVert() {
    return this->cfg.vert;
}

std::string& Manager::getShaderFrag() {
    return this->cfg.frag;    
}

bool Manager::isCPU() {
    return cfg.cpu;
}

/*
 *  Setters -- Size
 */

int Manager::setVertexSize() {
    int chunks = vertexCount ?: setVertexCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

int Manager::setIndexSize() {
    int chunks = indexCount ?: setIndexCount();
    int chunkSize = sizeof(uint);

    //std::cout << "allIndices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

/*
 *  Setters -- Count
 */

int Manager::setVertexCount() {
    return allVertices.size();
}

int Manager::setIndexCount() {
    return allIndices.size();
}

/*
 *  Printers
 */

void Manager::printIndices() {
    for (const auto &v : this->allIndices) {
        std::cout << v << ", ";
    }
    std::cout << std::endl;
}

void Manager::printVertices() {
    for (const auto &v : this->allVertices) {
        std::cout << glm::to_string(v) << ", ";
    }
    std::cout << std::endl;
}
