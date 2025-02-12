/**
 * manager.hpp
 *
 *    Created on: Sep 23, 2024
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

#ifndef MANAGER_H
#define MANAGER_H

#include <QMutex>
#include <QMutexLocker>
#include <vector>
#include <algorithm>

#include "filehandler.hpp"


using glm::vec4;
using glm::ivec2;
using glm::ivec3;
using fvec = std::vector<float>;
using dvec = std::vector<double>;
using uvec = std::vector<uint>;
using vVec4 = std::vector<vec4>;
// using vVec2 = std::vector<vec2>;


class Manager {
    public:
        Manager(){};
        virtual ~Manager(){ resetManager(); };

        virtual double create() { return 0.0; };
        virtual void update(double time) {m_time = time;};

        uint clearUpdates();

        uint getVertexCount();
        uint getVertexSize();
        uint getVertexOffset() { return int(this->vertexOffset); };
        uint getDataCount();
        uint getDataSize();
        uint getDataOffset() { return int(this->dataOffset); };
        uint getColourCount();
        uint getColourSize();
        uint getColourOffset() { return int(this->colourOffset); };
        uint getIndexCount();
        uint getIndexSize();
        uint getIndexOffset() { return int(this->indexOffset); };
        
        const float* getVertexData();
        const float* getDataData();
        const float* getColourData();
        const uint* getIndexData();

        bool isCPU() { return this->mStatus.hasAny(em::CPU_RENDER); };

        void printIndices();
        void printVertices();

    protected:
        virtual void initManager() {};
        virtual void resetManager();
        virtual void clearForNext();

        virtual void genVertexArray();
        virtual void genDataBuffer();
        virtual void genColourBuffer();
        virtual void genIndexBuffer();
        
        int setVertexCount();
        int setVertexSize();
        int setDataCount();
        int setDataSize();
        int setColourCount();
        int setColourSize();
        int setIndexCount();
        int setIndexSize();

        BitFlag mStatus;
        QMutex mutex;
        
        vVec4 allVertices;
        dvec dataStaging;
        fvec allData;
        vVec4 allColours;
        uvec indicesStaging;
        uvec allIndices;
        
        uint64_t vertexCount = 0;
        uint64_t vertexSize = 0;
        uint64_t vertexOffset = 0;
        uint64_t dataCount = 0;
        uint64_t dataSize = 0;
        uint64_t dataOffset = 0;
        uint64_t colourCount = 0;
        uint64_t colourSize = 0;
        uint64_t colourOffset = 0;
        uint64_t indexCount = 0;
        uint64_t indexSize = 0;
        uint64_t indexOffset = 0;

        double deg_fac = 0;
        double m_time = 0;

        bool init = false;

        enum em {
            INIT =              1 << 0,     // Manager has been initialized
            VERT_READY =        1 << 1,     // Vertices generated and ready for VBO load
            DATA_READY =        1 << 2,     // Special data generated and ready for VBO load
            INDEX_READY =       1 << 3,     // Indices generated and ready for IBO load
            INDEX_GEN =         1 << 4,     // Data generated but not processed
            UPD_SHAD_V =        1 << 5,     // Update Vertex Shader
            UPD_SHAD_F =        1 << 6,     // Update Fragment Shader
            UPD_VBO =           1 << 7,     // Cloud VBO needs to be updated
            UPD_DATA =          1 << 8,     // Cloud RDPs need to be loaded into VBO #2
            UPD_IBO =           1 << 9,     // Cloud IBO needs to be updated
            UPD_IDXOFF =        1 << 10,    // Index Offset/Count need to be updated
            UPD_UNI_COLOUR =    1 << 11,    // [Wave] Colour Uniforms need to be updated
            UPD_UNI_MATHS =     1 << 12,    // [Wave] Maths Uniforms need to be updated
            UPD_PUSH_CONST =    1 << 13,    // Push Constants need to be updated
            UPD_MATRICES =      1 << 14,    // Needs initVecsAndMatrices() to reset position and view
            CPU_RENDER =        1 << 15,    // CPU rendering
            UPDATE_REQUIRED =   1 << 16,    // An update must execute on next render
        };

        const uint eUpdateFlags = uint(-1) << 5;
        const uint eInitFlags = em::UPD_VBO | em::UPD_IBO | em::UPD_UNI_COLOUR | em::UPD_UNI_MATHS | em::UPD_PUSH_CONST | em::UPD_MATRICES;
};

#endif
