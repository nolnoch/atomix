/**
 * shaderobj.cpp
 *
 *      Created on: Apr 08, 2013
 *     Last Update: Oct 27, 2024
 *    Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *    Copyright 2013,2023,2024 Wade Burch (GPLv3)
 * 
 *    This file is part of atomix.
 * 
 *    atomix is free software: you can redistribute it and/or modify it under the
 *    terms of the GNU General Public License as published by the Free Software 
 *    Foundation, either version 3 of the License, or (at your option) any later 
 *    version.
 * 
 *    atomix is distributed in the hope that it will be useful, but WITHOUT ANY 
 *    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License along with 
 *    atomix. If not, see <https://www.gnu.org/licenses/>.
 */

#include "shaderobj.hpp"
#include <iostream>

#ifdef USING_QVULKAN
    #include <glslang/Public/ShaderLang.h>
    #include <glslang/Public/ResourceLimits.h>
    #include <glslang/SPIRV/GlslangToSpv.h>
#endif


/**
 * Primary Constructor.
 */
Shader::Shader(std::string fName, uint type)
: filePath(fName),
    shaderId(0),
    shaderType(type),
    sourceStringRaw("")
{
    this->fileToString();
}

/**
 * Default Destructor.
 */
Shader::~Shader() {
    // Nothing to do here (yet).
}

/**
 * Assigns the ID given by shader init.
 * @param idAssigned - the init-provided ID
 */
void Shader::setId(unsigned int idAssigned) {
    shaderId = idAssigned;
}

#ifdef USING_QVULKAN
/**
 * Compile the shader from source code.
 * 
 * Compiles the shader using glslang. This function will return false if the
 * shader fails to compile or link, and will output the error log to the
 * console. If the shader compiles successfully, the SPIR-V code will be
 * stored in the sourceBufferCompiled member variable and the validCompile
 * flag will be set to true.
 * 
 * @return true if the shader compiles and links successfully, false otherwise
 */
bool Shader::compile() {
    const char *shaderSource = this->getSourceRaw();
    int shaderLength = this->getLengthRaw();
    EShLanguage stage = (this->shaderType == GL_VERTEX_SHADER) ? EShLangVertex : EShLangFragment;
    EShMessages messages = EShMsgDefault;
    
    // Open the glslang library
    glslang::InitializeProcess();

    // Create the shader
    glslang::TShader shader(stage);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    shader.setStringsWithLengths(&shaderSource, &shaderLength, 1);
    shader.setEntryPoint("main");
    shader.setSourceEntryPoint("main");
    const TBuiltInResource *builtinResource = GetDefaultResources();
    if (!shader.parse(builtinResource, 450, ECoreProfile, false, false, messages)) {
        std::cout << "Failed to parse shader: " << this->filePath << std::endl;
        std::cout << "Error log: " << std::endl;
        std::cout << shader.getInfoLog() << std::endl;
        std::cout << shader.getInfoDebugLog() << std::endl;
        return false;
    }
    
    // Link the shader
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cout << "Failed to link shader: " << this->filePath << std::endl;
        std::cout << "Error log: " << std::endl;
        std::cout << program.getInfoLog() << std::endl;
        std::cout << program.getInfoDebugLog() << std::endl;
        return false;
    }

    // Generate SPIR-V
    glslang::GlslangToSpv(*program.getIntermediate(stage), this->sourceBufferCompiled);
    
    // Close the glslang library
    glslang::FinalizeProcess();
    
    this->validCompile = true;
    return true;
}
#endif

/**
 * Accessor function for the assigned ID.
 * @return the init-provided ID
 */
unsigned int Shader::getId() {
    return this->shaderId;
}

/**
 * Accessor function for the shader type.
 * @return the shader type
 */
unsigned int Shader::getType() {
    return this->shaderType;
}

/**
 * Accessor function for the shader filename as a string.
 * @return the string representation of the shader filename
 */
std::string& Shader::getName() {
    return this->fileName;
}

/**
 * Accessor function for the shader file path as a string.
 * @return the string representation of the shader file path
 */
std::string& Shader::getPath() {
    return this->filePath;
}

/**
 * Accessor function for the shader source parsed into a string.
 * @return the string representation of the shader source
 */
const char* Shader::getSourceRaw() {
    return this->sourceStringRaw.c_str();
}

const uint* Shader::getSourceCompiled() {
    return this->sourceBufferCompiled.data();
}

size_t Shader::getLengthRaw() {
    return this->sourceStringRaw.size();
}

size_t Shader::getLengthCompiled() {
    return this->sourceBufferCompiled.size();
}

/**
 * Used by Program object to check valid loading of the shader file.
 * @return true if successful or false if invalid file
 */
bool Shader::isValidFile() {
    return this->validFile;
}

/**
 * Used by Program object to check valid compilation of the shader.
 * @return true if successful or false if invalid compile
 */
bool Shader::isValidCompile() {
    return this->validCompile;
}

/**
 * Converts the shader source file to a string for loading by the program.
 */
bool Shader::fileToString() {
    std::fstream shaderFile(this->filePath.c_str(), std::ios::in);

    if (shaderFile.is_open()) {
        std::ostringstream buffer;
        buffer << shaderFile.rdbuf();
        this->sourceStringRaw = std::string(buffer.str());
        this->validFile = true;

        std::size_t nameStart = this->filePath.find_last_of('/') + 1;
        this->fileName = this->filePath.substr(nameStart);
    } else {
        this->validFile = false;
        this->fileName = "invalid";
    }

    return this->validFile;
}

