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


class Manager {
    public:
        Manager(){};
        virtual ~Manager(){ resetManager(); };
        
        void newConfig(AtomixConfig *config);

        const uint getVertexSize();
        const uint getIndexSize();
        const uint getIndexCount();
        const float* getVertexData();
        const uint* getIndexData();

        std::string& getShaderVert();
        std::string& getShaderFrag();

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
        
        vVec3 allVertices;
        uvec allIndices;
        
        uint vertexCount = 0;
        uint vertexSize = 0;
        uint indexCount = 0;
        uint indexSize = 0;
        
        bool update = false;
        bool active = false;
};

#endif