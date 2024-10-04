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
        
        virtual void clearForNext();

        const uint getVertexCount();
        const uint getVertexSize();
        const uint getIndexCount();
        const uint getIndexSize();
        
        const float* getVertexData();
        const uint* getIndexData();

        std::string& getShaderVert();
        std::string& getShaderFrag();

        BitFlag& getUpdates();

        void printIndices();
        void printVertices();

    protected:
        virtual void resetManager();

        virtual void genVertexArray();
        virtual void genIndexBuffer();
        
        int setVertexCount();
        int setVertexSize();
        int setIndexCount();
        int setIndexSize();

        AtomixConfig cfg;
        BitFlag mStatus;
        
        vVec3 allVertices;
        uvec allIndices;
        
        uint vertexCount = 0;
        uint vertexSize = 0;
        uint indexCount = 0;
        uint indexSize = 0;

        double deg_fac = 0;
        
        bool update = false;
        bool active = false;

        enum em {
            INIT =          1,          // Manager has been initialized
            NEW_CFG =       2,          // AtomixCfg has been changed
            VERT_READY =    2 << 1,     // Vertices generate and ready for VBO load
            DATA_READY =    2 << 2,     // Special data generated and ready for buffer load
            IDX_READY =     2 << 3,     // Indices generated and ready for EBO load
            UPDATE_VBO =    2 << 11,    // VBO needs to be updated
            UPDATE_DATA =   2 << 12,    // Data needs to be loaded into VBO #2
            UPDATE_EBO =    2 << 13,    // EBO needs to be updated
            INCR_VBO =      2 << 4,     // New VBO is larger than previous
            INCR_DATA =     2 << 5,     // New Data is larger than previous
            INCR_EBO =      2 << 6      // New EBO is larger than previous
        };
};

#endif