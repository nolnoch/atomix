/**
 * shaderobj.hpp
 *
 *    Created on: Apr 08, 2013
 *   Last Update: Oct 27, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 *
 *  This was created specifically for my Program class for easier use of
 *  GLSL shaders with the GLEW extension libraries.
 *
 *  This class stores each shader file as a Shader object with the data
 *  necessary to load, compile, attach, link, and use it in an automated
 *  framework.
 *
 *  Copyright 2013,2023,2024 Wade Burch (GPLv3)
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

#ifndef SHADEROBJ_HPP_
#define SHADEROBJ_HPP_

#include <string>
#include <cstdint>
#include <vector>
#include <fstream>
#include <sstream>

#ifndef USING_QOPENGL
    #define GL_VERTEX_SHADER 0x8B31
    #define GL_FRAGMENT_SHADER 0x8B30
    #define GL_GEOMETRY_SHADER 0x8DD9
    #define GL_COMPUTE_SHADER 0x91B9
#endif


class Shader {
public:
    Shader(std::string fName, uint type);
    virtual ~Shader();

    bool compile();
    void setId(unsigned int idAssigned);

    unsigned int getId();
    unsigned int getType();
    std::string& getName();
    std::string& getPath();
    const char* getSourceRaw();
    const uint* getSourceCompiled();
    size_t getLengthRaw();
    size_t getLengthCompiled();
    
    bool isValidFile();
    bool isValidCompile();

private:
    bool fileToString();

    unsigned int shaderId;
    unsigned int shaderType;
    std::string filePath;
    std::string fileName;
    std::string sourceStringRaw;
    std::vector<uint32_t> sourceBufferCompiled;
    
    bool validFile = false;
    bool validCompile = false;
};

#endif /* SHADER_HPP_ */
