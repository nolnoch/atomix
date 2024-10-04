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
    allData.clear();
    allIndices.clear();

    this->vertexCount = 0;
    this->vertexSize = 0;
    this->dataCount = 0;
    this->dataSize = 0;
    this->indexCount = 0;
    this->indexSize = 0;

    mStatus.setTo(em::INIT);
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

    this->mStatus.set(em::VERT_READY);
}

void Manager::genDataBuffer() {
    this->dataCount = setDataCount();
    this->dataSize = setDataSize();

    this->mStatus.set(em::DATA_READY);
}

void Manager::genIndexBuffer() {
    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();

    this->mStatus.set(em::INDEX_READY);
}

/*
 *  Getters -- Size
 */

const uint Manager::getVertexSize() {
    assert(mStatus.hasAll(em::VERT_READY));
    return this->vertexSize;
}

const uint Manager::getDataSize() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->dataSize;
}

const uint Manager::getIndexSize() {
    assert(mStatus.hasAll(em::INDEX_READY));
    return this->indexSize;
}

/*
 *  Getters -- Count
 */

const uint Manager::getVertexCount() {
    assert(mStatus.hasAll(em::VERT_READY));
    return this->vertexCount;
}

const uint Manager::getDataCount() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->dataCount;
}

const uint Manager::getIndexCount() {
    assert(mStatus.hasAll(em::INDEX_READY));
    return this->indexCount;
}

/*
 *  Getters -- Data
 */

const float* Manager::getVertexData() {
    assert(mStatus.hasAll(em::VERT_READY));
    return glm::value_ptr(allVertices.front());
}

const float* Manager::getDataData() {
    assert(mStatus.hasAll(em::DATA_READY));
    return &allData[0];
}

const uint* Manager::getIndexData() {
    assert(mStatus.hasAll(em::INDEX_READY));
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

int Manager::setDataSize() {
    int chunks = dataCount ?: setDataCount();
    int chunkSize = sizeof(float);

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

int Manager::setDataCount() {
    return allData.size();
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
