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

uint Manager::clearUpdates() {
    uint flags = mStatus.intersection(eUpdateFlags);
    mStatus.clear(eUpdateFlags);
    return flags;
}

/*
 *  Generators
 */

void Manager::genVertexArray() {
    assert(mStatus.hasAll(em::VERT_READY));
    
    this->vertexCount = setVertexCount();
    this->vertexSize = setVertexSize();

    this->mStatus.set(em::UPD_VBO);
}

void Manager::genDataBuffer() {
    assert(mStatus.hasAll(em::DATA_READY));

    this->dataCount = setDataCount();
    this->dataSize = setDataSize();

    this->mStatus.set(em::UPD_DATA);
}

void Manager::genColourBuffer() {
    assert(mStatus.hasAll(em::DATA_READY));

    this->colourCount = setColourCount();
    this->colourSize = setColourSize();

    this->mStatus.set(em::UPD_DATA);
}

void Manager::genIndexBuffer() {
    assert(mStatus.hasAll(em::INDEX_READY));

    this->indexCount = setIndexCount();
    this->indexSize = setIndexSize();

    this->mStatus.set(em::UPD_IBO);
}

/*
 *  Getters -- Size
 */

uint Manager::getVertexSize() {
    assert(mStatus.hasAll(em::VERT_READY));
    return this->vertexSize;
}

uint Manager::getDataSize() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->dataSize;
}

uint Manager::getColourSize() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->colourSize;
}

uint Manager::getIndexSize() {
    assert(mStatus.hasAll(em::INDEX_READY));
    return this->indexSize;
}

/*
 *  Getters -- Count
 */

uint Manager::getVertexCount() {
    assert(mStatus.hasAll(em::VERT_READY));
    return this->vertexCount;
}

uint Manager::getDataCount() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->dataCount;
}

uint Manager::getColourCount() {
    assert(mStatus.hasAll(em::DATA_READY));
    return this->colourCount;
}

uint Manager::getIndexCount() {
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

const float* Manager::getColourData() {
    assert(mStatus.hasAll(em::DATA_READY));
    return glm::value_ptr(allColours.front());
}

const uint* Manager::getIndexData() {
    assert(mStatus.hasAll(em::INDEX_READY));
    return &allIndices[0];
}

/*
 *  Getters -- Misc.
 */

bool Manager::getCPU() {
    return mStatus.hasAll(em::CPU_RENDER);
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

int Manager::setColourSize() {
    int chunks = colourCount ?: setColourCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allColours has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
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

int Manager::setColourCount() {
    return allColours.size();
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
