/**
 * manager.hpp
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

#ifndef MANAGER_H
#define MANAGER_H

#include <QMutex>
#include <QMutexLocker>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

using vVec3 = std::vector<glm::vec3>;
using vVec2 = std::vector<glm::vec2>;
using fvec = std::vector<float>;
using dvec = std::vector<double>;
using uvec = std::vector<uint>;
using vec3 = glm::vec3;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using harmap = std::map<int, std::vector<glm::ivec3>>;


class Manager {
    public:
        Manager(){};
        virtual ~Manager(){ resetManager(); };
        virtual void newConfig(AtomixConfig *config);
        virtual void initManager() {};

        virtual double create() { return 0.0; };
        virtual void update(double time) {};

        uint clearUpdates();

        const uint getVertexCount();
        const uint getVertexSize();
        const uint getDataCount();
        const uint getDataSize();
        const uint getIndexCount();
        const uint getIndexSize();
        
        const float* getVertexData();
        const float* getDataData();
        const uint* getIndexData();

        std::string& getShaderVert();
        std::string& getShaderFrag();

        bool isCPU();

        void printIndices();
        void printVertices();

    protected:
        virtual void resetManager();
        virtual void clearForNext();

        virtual void genVertexArray();
        virtual void genDataBuffer();
        virtual void genIndexBuffer();
        
        int setVertexCount();
        int setVertexSize();
        int setDataCount();
        int setDataSize();
        int setIndexCount();
        int setIndexSize();

        AtomixConfig cfg;
        BitFlag mStatus;
        QMutex mutex;
        
        vVec3 allVertices;
        dvec dataStaging;
        fvec allData;
        uvec indicesStaging;
        uvec allIndices;
        
        uint vertexCount = 0;
        uint vertexSize = 0;
        uint dataCount = 0;
        uint dataSize = 0;
        uint indexCount = 0;
        uint indexSize = 0;

        double deg_fac = 0;

        enum em {
            INIT =              1 << 0,     // Manager has been initialized
            VERT_READY =        1 << 1,     // Vertices generated and ready for VBO load
            DATA_READY =        1 << 2,     // Special data generated and ready for VBO load
            INDEX_READY =       1 << 3,     // Indices generated and ready for EBO load
            DATA_GEN =          1 << 4,     // Data generated but not processed
            UPD_SHAD_V =        1 << 5,     // Update Vertex Shader
            UPD_SHAD_F =        1 << 6,     // Update Fragment Shader
            UPD_VBO =           1 << 7,     // Cloud VBO needs to be updated
            UPD_DATA =          1 << 8,     // Cloud RDPs need to be loaded into VBO #2
            UPD_EBO =           1 << 9,     // Cloud EBO needs to be updated
            UPD_UNI_COLOUR =    1 << 10,    // [Wave] Colour Uniforms need to be updated
            UPD_UNI_MATHS =     1 << 11,    // [Wave] Maths Uniforms need to be updated
            UPD_MATRICES =      1 << 12,    // Needs initVecsAndMatrices() to reset position and view
            UPDATE_REQUIRED =   1 << 13,    // An update must execute on next render
        };

        const uint eUpdateFlags = em::UPD_SHAD_V | em::UPD_SHAD_F | em::UPD_VBO | em::UPD_DATA | em::UPD_EBO | em::UPD_UNI_COLOUR | em::UPD_UNI_MATHS | em::UPD_MATRICES;
};

#endif