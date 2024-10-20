/**
 * program.cpp
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
int Program::addShader(string fName, GLuint type) {
    int validFile;
    string fileLoc;

    if (fName.find('/') == std::string::npos) {
        fileLoc = string(ROOT_DIR) + string(SHADERS) + fName;
    } else {
        fileLoc = fName;
    }

    this->registeredShaders.push_back(Shader(fileLoc, type, qgf));
    validFile = this->registeredShaders.back().isValid();

    if (!validFile) {
        this->registeredShaders.pop_back();
        std::cout << "Failed to add shader source." << std::endl;
    } else
        this->stage = 1;

    return validFile;
}

/**
 * Associate N shader source files with the program as Shader objects.
 * This will populate the Shaders with their string-parsed sources, but init()
 * must still be called to compile and attach the shader(s) to the program.
 *
 * This function will return 0 upon error and automatically remove the
 * failed shader(s) from the program's list of Shaders.
 * @param fName - string representation of the shader filename
 * @param type - GLEW-defined constant, one of: GL_VERTEX_SHADER,
 *               GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
 * @return 1 on success or 0 on error
 */
int Program::addAllShaders(std::vector<std::string> *fList, GLuint type) {
    int errors = fList->size();
    GLuint shID, shIdx = 0;
    
    for (auto &fName : *fList) {
        uint validFile = 0;
        string fileLoc;
        
        if (fName.find('/') == std::string::npos) {
            fileLoc = string(ROOT_DIR) + string(SHADERS) + fName;
        } else {
            fileLoc = fName;
        }
        
        registeredShaders.push_back(Shader(fileLoc, type, qgf));
        validFile = registeredShaders.back().isValid();
    
        if (validFile) {
            errors--;
        } else {
            registeredShaders.pop_back();
            std::cout << "Failed to add shader source." << std::endl;
        }
    }
    
    if (!errors)
        this->stage = 1;

    return errors;
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
 * Initializes the program. Then initializes, loads, and compiles all shaders
 * associated with the Program object.
 */
void Program::init() {
    int numShaders = this->registeredShaders.size();

    if (!numShaders || !stage) {
        cout << "No shader files associated with program. Aborting..." << endl;
        return;
    }

    // Init program.
    this->programId = qgf->glCreateProgram();

    for (int i = 0; i < numShaders; i++) {
        // Init shader
        Shader *shad = &(this->registeredShaders[i]);
        GLuint id = qgf->glCreateShader(shad->type());
        shad->setId(id);

        // Load shader sources.
        const GLchar *shaderSource = shad->source().c_str();
        qgf->glShaderSource(id, 1, &shaderSource, NULL);

        // Compile shader from source.
        qgf->glCompileShader(id);

        // Display any problems with shader compilation.
        displayLogShader(id);

        // Add to compiledShaders map
        compiledShaders[shad->name()] = id;
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

GLuint Program::getShaderIdFromName(std::string& fileName) {
    assert(stage >= 2);
    assert(compiledShaders.count(fileName));
    
    return compiledShaders[fileName];
}

void Program::attachShader(std::string& name) {
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
GLint Program::linkAndValidate() {
    if (stage < 2) {
        cout << "Invalid linking. Must init (and bind attributes) first." << endl;
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
        displayLogProgram();

    this->stage = programValid ? 5 : 4;

    return programValid;
}

/**
 * @brief Detach only attached program shaders.
 * This should be done after successful link and validate.
 */
void Program::detachShaders() {
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
void Program::detachDelete() {
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
    enabled = true;
}

/**
 * Set the current program to NULL and resume normal OpenGL (direct-mode)
 * operation.
 */
void Program::disable() {
    qgf->glUseProgram(0);
    enabled = false;
}

void Program::initVAO() {
    qgf->glGenVertexArrays(1, &this->vao);
}

void Program::bindVAO() {
    qgf->glBindVertexArray(this->vao);
}

void Program::clearVAO() {
    qgf->glBindVertexArray(0);
}

GLuint Program::bindVBO(std::string name, uint bufCount, uint bufSize, const GLfloat *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ARRAY_BUFFER);
    GLuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void Program::setAttributePointerFormat(GLuint attrIdx, GLuint binding, GLuint count, GLenum type, GLuint offset, GLuint step) {
    if (std::find(attribs.begin(), attribs.end(), attrIdx) != attribs.end())
        attribs.push_back(attrIdx);
    //qgf->glVertexAttribPointer(idx, count, GL_FLOAT, GL_FALSE, stride, offset);
    qgf->glVertexArrayAttribFormat(this->vao, attrIdx, count, type, GL_FALSE, offset);
    qgf->glVertexArrayAttribBinding(this->vao, attrIdx, binding);
    qgf->glVertexArrayBindingDivisor(this->vao, attrIdx, step);
}

void Program::setAttributeBuffer(GLuint binding, GLuint vboIdx, GLsizei stride) {
    qgf->glVertexArrayVertexBuffer(this->vao, binding, vboIdx, 0, stride);
}

void Program::enableAttribute(GLuint idx) {
    qgf->glEnableVertexArrayAttrib(this->vao, idx);
}

void Program::enableAttributes() {
    for (auto a : attribs)
        qgf->glEnableVertexArrayAttrib(this->vao, a);
}

void Program::disableAttributes() {
    for (auto a : attribs)
        qgf->glDisableVertexArrayAttrib(this->vao, a);
}

void Program::updateVBO(uint offset, uint bufCount, uint bufSize, const GLfloat *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }
    
    qgf->glBufferSubData(GL_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgram();
}

void Program::updateVBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const GLfloat *buf) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgram();
}

void Program::resizeVBONamed(std::string name, uint bufCount, uint bufSize, const GLfloat *buf, uint mode) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgram();
}

void Program::clearVBO() {
    qgf->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint Program::bindEBO(std::string name, uint bufCount, uint bufSize, const GLuint *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ELEMENT_ARRAY_BUFFER);
    GLuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void Program::updateEBO(uint offset, uint bufCount, uint bufSize, const GLuint *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }

    qgf->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgram();
}

void Program::updateEBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const GLuint *buf) {
    GLuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgram();
}

void Program::resizeEBONamed(std::string name, uint bufCount, uint bufSize, const GLuint *buf, uint mode) {
    GLuint bufId = buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgram();
}

void Program::clearEBO() {
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Program::assignFragColour() {
    qgf->glBindFragDataLocation(this->programId, GL_COLOR_ATTACHMENT0, "FragColour");
}

void Program::beginRender() {
    enable();
    bindVAO();
    // qgf->glBindBuffer(GL_ARRAY_BUFFER, this->vbo.back());
    // qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo.back());
}

void Program::endRender() {
    clearVAO();
    disable();
}

void Program::clearBuffers() {
    clearVBO();
    clearEBO();
}

void Program::deleteBuffer(std::string name) {
    assert(!enabled);

    GLuint bufId = this->buffers[name].y;

    qgf->glDeleteBuffers(1, &bufId);
    this->buffers.erase(name);
}

bool Program::hasBuffer(std::string name) {
    return (buffers.find(name) != buffers.end());
}

/**
 * A quick wrapper for single, non-referenced uniform values.
 * @param type - GL_FLOAT or GL_INT
 * @param name - string representation of the GLSL uniform name
 * @param n - uniform value
 */
void Program::setUniform(int type, string name, double n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());
    float m = static_cast<float>(n);

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        cout << "Uniform failure: double to float" << endl;
}

void Program::setUniform(int type, string name, float n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        cout << "Uniform failure: float to float" << endl;
}

void Program::setUniform(int type, string name, int n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_INT) {
        qgf->glUniform1i(loc, n);
    } else
        cout << "Uniform failure: int to int" << endl;
}

void Program::setUniform(int type, string name, uint n) {
    GLint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_UNSIGNED_INT) {
        qgf->glUniform1ui(loc, n);
    } else
        cout << "Uniform failure: uint to uint" << endl;
}

/**
 * A quick wrapper for array of referenced uniform values.
 * @param count - number of vectors in the array
 * @param size - number of values per vector
 * @param type - GL_FLOAT or GL_INT
 * @param name - string representation of the GLSL uniform name
 * @param n - pointer to the array of values
 */
void Program::setUniformv(int count, int size, int type, string name, const float *n) {
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

uint Program::getSize(std::string name) {
    return this->buffers[name].x;
}

/**
 * Displays the info log for this program.
 */
void Program::displayLogProgram() {
    GLsizei logLength;
    qgf->glGetProgramiv(this->programId, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength) {
        cout << "Program Info Log content available." << endl;

        GLsizei MAXLENGTH = 1 << 30;
        GLchar *logBuffer = new GLchar[logLength];
        qgf->glGetProgramInfoLog(this->programId, MAXLENGTH, &logLength, logBuffer);
        if (strlen(logBuffer)) {
            cout << "************ Begin Program Log ************" << "\n";
            cout << logBuffer << "\n";
            cout << "************* End Program Log *************" << endl;
        }
        delete[] logBuffer;
    }
}

/**
 * Displays the info log for the specified shader.
 * @param shader - the shader to be evaluated
 */
void Program::displayLogShader(GLenum shader) {
    GLint success;
    qgf->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
        cout << "Shader compile failure for shader #" << shader << endl;
    
    GLsizei logLength;
    qgf->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    GLsizei MAXLENGTH = 1 << 30;
    GLchar *logBuffer = new GLchar[MAXLENGTH];
    qgf->glGetShaderInfoLog(shader, MAXLENGTH, &logLength, logBuffer);
    if (strlen(logBuffer)) {
        cout << "************ Begin Shader Log ************" << "\n";
        cout << logBuffer << "\n";
        cout << "************* End Shader Log *************" << endl;
    }
    delete[] logBuffer;
}
