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

using namespace std;


/**
 * Primary Constructor.
 */
Shader::Shader(string fName, int type, QOpenGLFunctions_4_5_Core *funcPointer)
: filePath(fName),
  shaderId(0),
  shaderType(type),
  sourceString(""),
  valid(0),
  qgf(funcPointer) {
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
void Shader::setId(GLenum idAssigned) {
  shaderId = idAssigned;
}

/**
 * Accessor function for the assigned ID.
 * @return the init-provided ID
 */
GLenum Shader::id() {
  return this->shaderId;
}

/**
 * Accessor function for the shader type.
 * @return the shader type
 */
GLuint Shader::type() {
  return this->shaderType;
}

/**
 * Accessor function for the shader filename as a string.
 * @return the string representation of the shader filename
 */
string& Shader::name() {
  return this->fileName;
}

/**
 * Accessor function for the shader file path as a string.
 * @return the string representation of the shader file path
 */
string& Shader::path() {
  return this->filePath;
}

/**
 * Accessor function for the shader source parsed into a string.
 * @return the string representation of the shader source
 */
string& Shader::source() {
  return this->sourceString;
}

/**
 * Used by Program object to check valid loading of the shader file.
 * @return 1 if successful or 0 if invalid file
 */
int Shader::isValid() {
  return this->valid;
}

/**
 * Converts the shader source file to a string for loading by the program.
 */
void Shader::fileToString() {
  fstream shaderFile(this->filePath.c_str(), ios::in);

  if (shaderFile.is_open()) {
    ostringstream buffer;
    buffer << shaderFile.rdbuf();
    this->sourceString = string(buffer.str());
    this->valid = 1;
  }
}

