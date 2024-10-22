/**
 * shaderobj.hpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Sep 9, 2024
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
#include <fstream>
#include <sstream>
#include <QOpenGLFunctions_4_5_Core>
#include <QVulkanFunctions>

class Shader {
 public:
  Shader(std::string fName, int type, QOpenGLFunctions_4_5_Core *funcPointer);
  virtual ~Shader();

  void setId(GLenum idAssigned);

  GLenum id();
  GLuint type();
  std::string& name();
  std::string& path();
  std::string& source();
  int isValid();

 private:
  GLenum shaderId;
  GLuint shaderType;
  std::string filePath;
  std::string fileName;
  std::string sourceString;
  int valid;
  QOpenGLFunctions_4_5_Core *qgf = nullptr;

  void fileToString();
};

#endif /* SHADER_HPP_ */
