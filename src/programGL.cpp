/**
 * programGL.cpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Sep 23, 2024
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

#include "programGL.hpp"


/**
 * Default Constructor.
 */
ProgramGL::ProgramGL(QOpenGLFunctions_4_5_Core *funcPointer)
: qgf(funcPointer) {
}

/**
 * Default Destructor.
 */
ProgramGL::~ProgramGL() {
    delete samplers;
    if (vao) {
        for (auto &m : buffers) {
            qgf->glDeleteBuffers(1, &m.second.y);
        }
        qgf->glDeleteVertexArrays(1, &vao);
    }
    if (stage >= 2) {
        //for (auto sh : shaders)
        //  delete (&sh);
        qgf->glDeleteProgram(programId);
    }
}

/**
 * Associate a shader source file with the program as a Shader object.
 * This will populate the Shader with its std::string-parsed source, but init()
 * must still be called to compile and attach the shader to the program.
 *
 * This function will return 0 upon error and automatically remove the
 * failed shader from the program's list of Shaders.
 * @param fName - std::string representation of the shader filename
 * @param type - GLEW-defined constant, one of: GL_VERTEX_SHADER,
 *               GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
 * @return 1 on success or 0 on error
 */
int ProgramGL::addShader(std::string fName, GLuint type) {
    int validFile;
    std::string fileLoc;

    if (fName.find('/') == std::string::npos) {
        fileLoc = std::string(ROOT_DIR) + std::string(SHADERS) + fName;
    } else {
        fileLoc = fName;
    }

    this->registeredShaders.push_back(Shader(fileLoc, type));
    validFile = this->registeredShaders.back().isValidFile();

    if (!validFile) {
        this->registeredShaders.pop_back();
        std::cout << "Failed to add shader source: " << fName << std::endl;
    } else
        this->stage = 1;

    return validFile;
}

/**
 * Associate N shader source files with the program as Shader objects.
 * This will populate the Shaders with their std::string-parsed sources, but init()
 * must still be called to compile and attach the shader(s) to the program.
 *
 * This function will return 0 upon error and automatically remove the
 * failed shader(s) from the program's list of Shaders.
 * @param fName - std::string representation of the shader filename
 * @param type - GLEW-defined constant, one of: GL_VERTEX_SHADER,
 *               GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
 * @return 1 on success or 0 on error
 */
int ProgramGL::addAllShaders(std::vector<std::string> *fList, GLuint type) {
    int errors = fList->size();
    GLuint shID, shIdx = 0;
    
    for (auto &fName : *fList) {
        uint validFile = 0;
        std::string fileLoc;
        
        if (fName.find('/') == std::string::npos) {
            fileLoc = std::string(ROOT_DIR) + std::string(SHADERS) + fName;
        } else {
            fileLoc = fName;
        }
        
        registeredShaders.push_back(Shader(fileLoc, type));
        validFile = registeredShaders.back().isValidFile();
    
        if (validFile) {
            errors--;
        } else {
            registeredShaders.pop_back();
            std::cout << "Failed to add shader source: " << fName << std::endl;
        }
    }
    
    if (!errors)
        this->stage = 1;

    return errors;
}

/**
 * Shortcut for adding one shader.vert and one shader.frag.
 */
void ProgramGL::addDefaultShaders() {
    this->addShader("shader.vert", GL_VERTEX_SHADER);
    this->addShader("shader.frag", GL_FRAGMENT_SHADER);
    // this->addShader("shader.geom", GL_GEOMETRY_SHADER);
}

/**
 * Generates a sampler uniform bind target for use in the GLSL shader code.
 */
void ProgramGL::addSampler(std::string sName) {
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
 * Initializes the program. Then initializes, loads, and compiles all shaders
 * associated with the ProgramGL object.
 */
void ProgramGL::init() {
    int numShaders = this->registeredShaders.size();

    if (!numShaders || !stage) {
        std::cout << "No shader files associated with program. Aborting..." << std::endl;
        return;
    }

    // Init program.
    this->programId = qgf->glCreateProgram();

    for (int i = 0; i < numShaders; i++) {
        // Init shader
        Shader *shad = &(this->registeredShaders[i]);
        GLuint id = qgf->glCreateShader(shad->getType());
        shad->setId(id);

        // Load shader sources.
        const GLchar *shaderSource = shad->getSourceRaw();
        qgf->glShaderSource(id, 1, &shaderSource, nullptr);

        // Compile shader from source.
        qgf->glCompileShader(id);

        // Display any problems with shader compilation.
        displayLogShader(id);

        // Add to compiledShaders map
        compiledShaders[shad->getName()] = id;
    }

    this->stage = 2;
}

/**
 * Attribute binding (for use with Vertex Array/Buffer Objects) must happen
 * after initialization of the program but is only recognized on the next
 * linking. Linking may skip this step, but warnings will be given if done
 * out of order.
 * @param location - explicitly specify the integer binding target
 * @param name - std::string representation of the GLSL attribute name
 */
void ProgramGL::bindAttribute(int location, std::string name) {
    if (stage < 2) {
        std::cout << "Invalid binding. Must init first." << std::endl;
        return;
    } else if (stage >= 4) {
        std::cout << "This attribute binding will not take effect until next linking."
            << std::endl;
    }

    // Bind explicit attribute locations before linking.
    qgf->glBindAttribLocation(this->programId, location, name.c_str());

    this->stage = 3;
}

GLuint ProgramGL::getShaderIdFromName(std::string& fileName) {
    assert(stage >= 2);
    assert(compiledShaders.count(fileName));
    
    return compiledShaders[fileName];
}

void ProgramGL::attachShader(std::string& name) {
    GLuint shID = getShaderIdFromName(name);
    assert(shID);

    // Attach compiled shader to program.
    qgf->glAttachShader(this->programId, shID);
    attachedShaders.push_back(shID);
}

/**
 * Completes the linking of the program with all attached shaders.
 * Automatically validates the program and displays the info log if the
 * info log is not empty.
 *
 * @return GLEW_OK on success or an error code on failure.
 */
GLint ProgramGL::linkAndValidate() {
    if (stage < 2) {
        std::cout << "Invalid linking. Must init (and bind attributes) first." << std::endl;
        return 0;
    }

    GLint programValid = 0;
    
    // Link the compiled and attached program to this code.
    qgf->glLinkProgram(programId);
    if (programId == GL_INVALID_VALUE)
        exit(-1);

    // Verify program compilation and linkage.
    qgf->glValidateProgram(programId);
    qgf->glGetProgramiv(programId, GL_VALIDATE_STATUS, &programValid);
    if (!programValid)
        displayLogProgramGL();

    this->stage = programValid ? 5 : 4;

    return programValid;
}

/**
 * @brief Detach only attached program shaders.
 * This should be done after successful link and validate.
 */
void ProgramGL::detachShaders() {
    assert(this->stage >= 5);

    if (!attachedShaders.size())
        return;

    for (auto id : attachedShaders) {
        qgf->glDetachShader(programId, id);
    }
    attachedShaders.clear();
}

/**
 * @brief Detach and delete all program shaders.
 * This should be done after successful link and validate.
 */
void ProgramGL::detachDelete() {
    assert(this->stage >= 5);

    if (!attachedShaders.size())
        return;

    for (auto id : attachedShaders) {
        qgf->glDetachShader(programId, id);
        qgf->glDeleteShader(id);
    }
    attachedShaders.clear();
}

/**
 * A sequence-protected wrapper for glUseProgramGL().  This completely preempts
 * the OpenGL graphics pipeline for any shader functions implemented.
 */
void ProgramGL::enable() {
    if (stage < 5) {
        if (stage < 4)
        std::cout << "ProgramGL not ready to enable: must link before use." << std::endl;
        else
        std::cout << "ProgramGL not ready to enable: linked but not valid." << std::endl;

        return;
    }

    qgf->glUseProgram(this->programId);
    enabled = true;
}

/**
 * Set the current program to NULL and resume normal OpenGL (direct-mode)
 * operation.
 */
void ProgramGL::disable() {
    qgf->glUseProgram(0);
    enabled = false;
}

void ProgramGL::initVAO() {
    qgf->glGenVertexArrays(1, &this->vao);
}

void ProgramGL::bindVAO() {
    qgf->glBindVertexArray(this->vao);
}

void ProgramGL::clearVAO() {
    qgf->glBindVertexArray(0);
}

GLuint ProgramGL::bindVBO(std::string name, uint bufCount, uint bufSize, const GLfloat *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ARRAY_BUFFER);
    GLuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void ProgramGL::setAttributePointerFormat(GLuint attrIdx, GLuint binding, GLuint count, GLenum type, GLuint offset, GLuint step) {
    if (std::find(attribs.begin(), attribs.end(), attrIdx) != attribs.end())
        attribs.push_back(attrIdx);
    //qgf->glVertexAttribPointer(idx, count, GL_FLOAT, GL_FALSE, stride, offset);
    qgf->glVertexArrayAttribFormat(this->vao, attrIdx, count, type, GL_FALSE, offset);
    qgf->glVertexArrayAttribBinding(this->vao, attrIdx, binding);
    qgf->glVertexArrayBindingDivisor(this->vao, attrIdx, step);
}

void ProgramGL::setAttributeBuffer(GLuint binding, GLuint vboIdx, GLsizei stride) {
    qgf->glVertexArrayVertexBuffer(this->vao, binding, vboIdx, 0, stride);
}

void ProgramGL::enableAttribute(GLuint idx) {
    qgf->glEnableVertexArrayAttrib(this->vao, idx);
}

void ProgramGL::enableAttributes() {
    for (auto a : attribs)
        qgf->glEnableVertexArrayAttrib(this->vao, a);
}

void ProgramGL::disableAttributes() {
    for (auto a : attribs)
        qgf->glDisableVertexArrayAttrib(this->vao, a);
}

void ProgramGL::updateVBO(uint offset, uint bufCount, uint bufSize, const GLfloat *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }
    
    qgf->glBufferSubData(GL_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgramGL();
}

void ProgramGL::updateVBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const GLfloat *buf) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgramGL();
}

void ProgramGL::resizeVBONamed(std::string name, uint bufCount, uint bufSize, const GLfloat *buf, uint mode) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgramGL();
}

void ProgramGL::clearVBO() {
    qgf->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint ProgramGL::bindEBO(std::string name, uint bufCount, uint bufSize, const GLuint *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ELEMENT_ARRAY_BUFFER);
    GLuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void ProgramGL::updateEBO(uint offset, uint bufCount, uint bufSize, const GLuint *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }

    qgf->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgramGL();
}

void ProgramGL::updateEBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const GLuint *buf) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgramGL();
}

void ProgramGL::resizeEBONamed(std::string name, uint bufCount, uint bufSize, const GLuint *buf, uint mode) {
    GLuint bufId = buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgramGL();
}

void ProgramGL::clearEBO() {
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ProgramGL::assignFragColour() {
    qgf->glBindFragDataLocation(this->programId, GL_COLOR_ATTACHMENT0, "FragColour");
}

void ProgramGL::beginRender() {
    enable();
    bindVAO();
    // qgf->glBindBuffer(GL_ARRAY_BUFFER, this->vbo.back());
    // qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo.back());
}

void ProgramGL::endRender() {
    clearVAO();
    disable();
}

void ProgramGL::clearBuffers() {
    clearVBO();
    clearEBO();
}

void ProgramGL::deleteBuffer(std::string name) {
    assert(!enabled);

    GLuint bufId = this->buffers[name].y;

    qgf->glDeleteBuffers(1, &bufId);
    this->buffers.erase(name);
}

bool ProgramGL::hasBuffer(std::string name) {
    return (buffers.find(name) != buffers.end());
}

/**
 * A quick wrapper for single, non-referenced uniform values.
 * @param type - GL_FLOAT or GL_INT
 * @param name - std::string representation of the GLSL uniform name
 * @param n - uniform value
 */
void ProgramGL::setUniform(int type, std::string name, double n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());
    float m = static_cast<float>(n);

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        std::cout << "Uniform failure: double to float" << std::endl;
}

void ProgramGL::setUniform(int type, std::string name, float n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        std::cout << "Uniform failure: float to float" << std::endl;
}

void ProgramGL::setUniform(int type, std::string name, int n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_INT) {
        qgf->glUniform1i(loc, n);
    } else
        std::cout << "Uniform failure: int to int" << std::endl;
}

void ProgramGL::setUniform(int type, std::string name, uint n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_UNSIGNED_INT) {
        qgf->glUniform1ui(loc, n);
    } else
        std::cout << "Uniform failure: uint to uint" << std::endl;
}

/**
 * A quick wrapper for array of referenced uniform values.
 * @param count - number of vectors in the array
 * @param size - number of values per vector
 * @param type - GL_FLOAT or GL_INT
 * @param name - std::string representation of the GLSL uniform name
 * @param n - pointer to the array of values
 */
void ProgramGL::setUniformv(int count, int size, int type, std::string name, const float *n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_FLOAT) {
        switch (size) {
        case 1:
            qgf->glUniform1fv(loc, count, n);
            break;
        case 2:
            qgf->glUniform2fv(loc, count, n);
            break;
        case 3:
            qgf->glUniform3fv(loc, count, n);
            break;
        case 4:
            qgf->glUniform4fv(loc, count, n);
            break;
        default:
            break;
        }
    } else if (type == GL_INT) {
        switch (size) {
        case 1:
            qgf->glUniform1iv(loc, count, reinterpret_cast<const int *>(n));
            break;
        case 2:
            qgf->glUniform2iv(loc, count, reinterpret_cast<const int *>(n));
            break;
        case 3:
            qgf->glUniform3iv(loc, count, reinterpret_cast<const int *>(n));
            break;
        case 4:
            qgf->glUniform4iv(loc, count, reinterpret_cast<const int *>(n));
            break;
        default:
            break;
        }
    } else if (type == GL_UNSIGNED_INT) {
        switch (size) {
        case 1:
            qgf->glUniform1uiv(loc, count, reinterpret_cast<const uint *>(n));
            break;
        case 2:
            qgf->glUniform2uiv(loc, count, reinterpret_cast<const uint *>(n));
            break;
        case 3:
            qgf->glUniform3uiv(loc, count, reinterpret_cast<const uint *>(n));
            break;
        case 4:
            qgf->glUniform4uiv(loc, count, reinterpret_cast<const uint *>(n));
            break;
        default:
            break;
        }
    }
}

/**
 * A quick wrapper for passing matrices to GLSL uniforms.
 * @param size - width of the square matrix
 * @param name - std::string representation of the GLSL uniform name
 * @param m - pointer to the first matrix value
 */
void ProgramGL::setUniformMatrix(int size, std::string name, float *m) {
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
GLuint ProgramGL::getProgramGLId() {
    return this->programId;
}

uint ProgramGL::getSize(std::string name) {
    return this->buffers[name].x;
}

/**
 * Displays the info log for this program.
 */
void ProgramGL::displayLogProgramGL() {
    GLsizei logLength;
    qgf->glGetProgramiv(this->programId, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength) {
        std::cout << "ProgramGL Info Log content available." << std::endl;

        GLsizei MAXLENGTH = 1 << 30;
        GLchar *logBuffer = new GLchar[logLength];
        qgf->glGetProgramInfoLog(this->programId, MAXLENGTH, &logLength, logBuffer);
        if (std::string(logBuffer).length()) {
            std::cout << "************ Begin ProgramGL Log ************" << "\n";
            std::cout << logBuffer << "\n";
            std::cout << "************* End ProgramGL Log *************" << std::endl;
        }
        delete[] logBuffer;
    }
}

/**
 * Displays the info log for the specified shader.
 * @param shader - the shader to be evaluated
 */
void ProgramGL::displayLogShader(GLenum shader) {
    GLint success;
    qgf->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
        std::cout << "Shader compile failure for shader #" << shader << std::endl;
    
    GLsizei logLength;
    qgf->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    GLsizei MAXLENGTH = 1 << 30;
    GLchar *logBuffer = new GLchar[MAXLENGTH];
    qgf->glGetShaderInfoLog(shader, MAXLENGTH, &logLength, logBuffer);
    if (std::string(logBuffer).length()) {
        std::cout << "************ Begin Shader Log ************" << "\n";
        std::cout << logBuffer << "\n";
        std::cout << "************* End Shader Log *************" << std::endl;
    }
    delete[] logBuffer;
}
