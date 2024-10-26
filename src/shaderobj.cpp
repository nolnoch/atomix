/**
 * shaderobj.cpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
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

#include "shaderobj.hpp"


/**
 * Primary Constructor.
 */
Shader::Shader(std::string fName, int type)
: filePath(fName),
  shaderId(0),
  shaderType(type),
  sourceString(""),
  valid(0)
{
  this->fileToString();
  std::size_t nameStart = fName.find_last_of('/') + 1;
  this->fileName = fName.substr(nameStart);
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

/**
 * Accessor function for the assigned ID.
 * @return the init-provided ID
 */
unsigned int Shader::id() {
  return this->shaderId;
}

/**
 * Accessor function for the shader type.
 * @return the shader type
 */
unsigned int Shader::type() {
  return this->shaderType;
}

/**
 * Accessor function for the shader filename as a string.
 * @return the string representation of the shader filename
 */
std::string& Shader::name() {
  return this->fileName;
}

/**
 * Accessor function for the shader file path as a string.
 * @return the string representation of the shader file path
 */
std::string& Shader::path() {
  return this->filePath;
}

/**
 * Accessor function for the shader source parsed into a string.
 * @return the string representation of the shader source
 */
std::string& Shader::source() {
  return this->sourceString;
}

/**
 * Used by Program object to check valid loading of the shader file.
 * @return 1 if successful or 0 if invalid file
 */
bool Shader::isValid() {
  return this->valid;
}

/**
 * Converts the shader source file to a string for loading by the program.
 */
void Shader::fileToString() {
  std::fstream shaderFile(this->filePath.c_str(), std::ios::in);

  if (shaderFile.is_open()) {
    std::ostringstream buffer;
    buffer << shaderFile.rdbuf();
    this->sourceString = std::string(buffer.str());
    this->valid = true;
  }
}

