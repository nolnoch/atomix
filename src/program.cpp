/**
 * program.cpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Oct 14, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2013,2023 Wade Burch (GPLv3)
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

#include "program.hpp"

using std::cout;
using std::endl;
using std::string;


/**
 * Default Constructor.
 */
Program::Program(QOpenGLFunctions_4_5_Core *funcPointer)
: qgf(funcPointer) {
}

/**
 * Default Destructor.
 */
Program::~Program() {
  if (samplers)
    delete samplers;
  if (vao)
    qgf->glDeleteVertexArrays(1, &vao);
  if (stage >= 2)
    qgf->glDeleteProgram(programId);
}

/**
 * Associate a shader source file with the program as a Shader object.
 * This will populate the Shader with its string-parsed source, but init()
 * must still be called to compile and attach the shader to the program.
 *
 * This function will return 0 upon error and automatically remove the
 * failed shader from the program's list of Shaders.
 * @param fName - string representation of the shader filename
 * @param type - GLEW-defined constant, one of: GL_VERTEX_SHADER,
 *               GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
 * @return 1 on success or 0 on error
 */
int Program::addShader(string fName, int type) {
  int validFile;

  string fileLoc = SHADERS + fName;

  this->shaders.push_back(Shader(fileLoc, type));
  validFile = this->shaders.back().isValid();

  if (!validFile) {
    this->shaders.pop_back();
    std::cout << "Failed to add shader source." << std::endl;
  } else
    this->stage = 1;

  return validFile;
}

/**
 * Shortcut for adding one shader.vert and one shader.frag.
 */
void Program::addDefaultShaders() {
  this->addShader("shader.vert", GL_VERTEX_SHADER);
  this->addShader("shader.frag", GL_FRAGMENT_SHADER);
  // this->addShader("shader.geom", GL_GEOMETRY_SHADER);
}

/**
 * Generates a sampler uniform bind target for use in the GLSL shader code.
 */
void Program::addSampler(string sName) {
  SamplerInfo info;
  GLuint sample;

  if (!samplers)
    this->samplers = new std::vector<SamplerInfo>();

  qgf->glGenSamplers(1, &sample);

  info.samplerID = sample;
  info.samplerName = sName;

  this->samplers->push_back(info);
}

/**
 * Initializes the program. Then initializes, loads, compiles, and attaches
 * all shaders associated with the Program object.
 */
void Program::init() {
  int numShaders = this->shaders.size();

  if (!numShaders || !stage) {
    cout << "No shader files associated with program. Aborting..." << endl;
    return;
  }

  // Init program.
  this->programId = qgf->glCreateProgram();

  for (int i = 0; i < numShaders; i++) {
    // Init shader
    Shader& shad = this->shaders[i];
    shad.setId(qgf->glCreateShader(shad.type()));

    // Load shader sources.
    const GLchar *shaderSource = shad.source().c_str();
    qgf->glShaderSource(shad.id(), 1, &shaderSource, NULL);

    // Compile shader from source.
    qgf->glCompileShader(shad.id());

    // Display any problems with shader compilation.
    displayLogShader(shad.id());

    // Attach compiled shader to program.
    qgf->glAttachShader(this->programId, shad.id());
  }

  this->stage = 2;
}

/**
 * Attribute binding (for use with Vertex Array/Buffer Objects) must happen
 * after initialization of the program but is only recognized on the next
 * linking. Linking may skip this step, but warnings will be given if done
 * out of order.
 * @param location - explicitly specify the integer binding target
 * @param name - string representation of the GLSL attribute name
 */
void Program::bindAttribute(int location, string name) {
  if (stage < 2) {
    cout << "Invalid binding. Must init first." << endl;
    return;
  } else if (stage >= 4) {
    cout << "This attribute binding will not take effect until next linking."
         << endl;
  }

  // Bind explicit attribute locations before linking.
  qgf->glBindAttribLocation(this->programId, location, name.c_str());

  this->stage = 3;
}

/**
 * Completes the linking of the program with all attached shaders.
 * Automatically validates the program and displays the info log if the
 * info log is not empty.
 *
 * Note: Some cards print only errors while some print a success statement.
 * @return GLEW_OK on success or an error code on failure.
 */
GLint Program::linkAndValidate() {
  if (stage < 2) {
    cout << "Invalid linking. Must init (and bind attributes) first." << endl;
    return 0;
  }

  GLint programValid;
  // Link the compiled and attached program to this code.
  qgf->glLinkProgram(programId);
  if (programId == GL_INVALID_VALUE)
    exit(-1);

  // Verify program compilation and linkage.
  qgf->glValidateProgram(programId);
  qgf->glGetProgramiv(programId, GL_VALIDATE_STATUS, &programValid);
  if (!programValid)
    displayLogProgram();

  this->stage = programValid ? 5 : 4;

  return programValid;
}

/**
 * A sequence-protected wrapper for glUseProgram().  This completely preempts
 * the OpenGL graphics pipeline for any shader functions implemented.
 */
void Program::enable() {
  if (stage < 5) {
    if (stage < 4)
      cout << "Program not ready to enable: must link before use." << endl;
    else
      cout << "Program not ready to enable: linked but not valid." << endl;

    return;
  }

  qgf->glUseProgram(this->programId);
}

/**
 * Set the current program to NULL and resume normal OpenGL (direct-mode)
 * operation.
 */
void Program::disable() {
  qgf->glUseProgram(0);
}

void Program::initVAO() {
  qgf->glGenVertexArrays(1, &vao);
}

void Program::bindVAO() {
  qgf->glBindVertexArray(vao);
}

void Program::clearVAO() {
  qgf->glBindVertexArray(0);
}

void Program::bindVBO(uint bufSize, const GLfloat *buf, uint mode) {
  qgf->glGenBuffers(1, &this->vbo);
  qgf->glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  qgf->glBufferData(GL_ARRAY_BUFFER, bufSize, buf, mode);
}

void Program::attributePointer(uint idx, uint count, uint stride, const void *offset) {
  attribs.push_back(idx);
  qgf->glVertexAttribPointer(idx, count, GL_FLOAT, GL_FALSE, stride, offset);
}

void Program::enableAttributes() {
  for (auto i: attribs)
    qgf->glEnableVertexAttribArray(i);
}

void Program::updateVBO(uint offset, uint bufSize, const GLfloat *buf) {
  qgf->glBufferSubData(GL_ARRAY_BUFFER, offset, bufSize, buf);
  displayLogProgram();
}

void Program::clearVBO() {
  qgf->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Program::bindEBO(uint bufSize, const GLuint *buf) {
  qgf->glGenBuffers(1, &this->ebo);
  qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
  qgf->glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufSize, buf, GL_STATIC_DRAW);
}

void Program::clearEBO() {
  qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Program::beginRender() {
  enable();
  bindVAO();
  qgf->glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
}

void Program::endRender() {
  clearVAO();
  disable();
}

void Program::clearBuffers() {
  clearVBO();
  clearEBO();
}

/**
 * A quick wrapper for single, non-referenced uniform values.
 * @param type - GL_FLOAT or GL_INT
 * @param name - string representation of the GLSL uniform name
 * @param n - uniform value
 */
void Program::setUniform(int type, string name, float n) {
  GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

  if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
  } else if (type == GL_INT) {
        qgf->glUniform1i(loc, static_cast<int>(n));
  }
}

/**
 * A quick wrapper for array of referenced uniform values.
 * @param count - number of values in the array
 * @param type - GL_FLOAT or GL_INT
 * @param name - string representation of the GLSL uniform name
 * @param n - pointer to the array of values
 */
void Program::setUniformv(int count, int type, string name, const float *n) {
  GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

  if (type == GL_FLOAT) {
    switch (count) {
      case 1:
        qgf->glUniform1fv(loc, 1, n);
        break;
      case 2:
        qgf->glUniform2fv(loc, 1, n);
        break;
      case 3:
        qgf->glUniform3fv(loc, 1, n);
        break;
      case 4:
        qgf->glUniform4fv(loc, 1, n);
        break;
      default:
        break;
    }
  } else if (type == GL_INT) {
    switch (count) {
      case 1:
        qgf->glUniform1iv(loc, 1, reinterpret_cast<const int *>(n));
        break;
      case 2:
        qgf->glUniform2iv(loc, 1, reinterpret_cast<const int *>(n));
        break;
      case 3:
        qgf->glUniform3iv(loc, 1, reinterpret_cast<const int *>(n));
        break;
      case 4:
        qgf->glUniform4iv(loc, 1, reinterpret_cast<const int *>(n));
        break;
      default:
        break;
    }
  }
}

/**
 * A quick wrapper for passing matrices to GLSL uniforms.
 * @param size - width of the square matrix
 * @param name - string representation of the GLSL uniform name
 * @param m - pointer to the first matrix value
 */
void Program::setUniformMatrix(int size, string name, float *m) {
  GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

  if (size == 4) {
    qgf->glUniformMatrix4fv(loc, 1, GL_FALSE, m);
  } else if (size == 3) {
    qgf->glUniformMatrix3fv(loc, 1, GL_FALSE, m);
  }
}

/**
 * Accessor function for the GLenum program ID.
 * @return the program ID
 */
GLuint Program::getProgramId() {
  return this->programId;
}

/**
 * Displays the info log for this program.
 */
void Program::displayLogProgram() {
  GLsizei logLength;
  qgf->glGetProgramiv(this->programId, GL_INFO_LOG_LENGTH, &logLength);

  GLsizei MAXLENGTH = 1 << 30;
  GLchar *logBuffer = new GLchar[MAXLENGTH];
  qgf->glGetProgramInfoLog(this->programId, MAXLENGTH, &logLength, logBuffer);
  if (strlen(logBuffer)) {
    cout << "************ Begin Program Log ************" << endl;
    cout << logBuffer << endl;
    cout << "************* End Program Log *************" << endl;
  }
  delete[] logBuffer;
}

/**
 * Displays the info log for the specified shader.
 * @param shader - the shader to be evaluated
 */
void Program::displayLogShader(GLenum shader) {
  GLsizei logLength;
  qgf->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

  GLsizei MAXLENGTH = 1 << 30;
  GLchar *logBuffer = new GLchar[MAXLENGTH];
  qgf->glGetShaderInfoLog(shader, MAXLENGTH, &logLength, logBuffer);
  if (strlen(logBuffer)) {
    cout << "************ Begin Shader Log ************" << endl;
    cout << logBuffer << endl;
    cout << "************* End Shader Log *************" << endl;
  }
  delete[] logBuffer;
}
