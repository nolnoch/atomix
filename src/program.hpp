/**
 * program.hpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Oct 14, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 *
 *  This class was created to automate (and generally make easier) the use
 *  of GLSL programs through the GLEW extension libraries.
 *
 *  Notes:
 *
 *    You may call addShader for every file you want to attach to each
 *    program, and all will be processed and used for that program's
 *    lifespan.  Create multiple programs to separate your shaders for
 *    modular loading and use.
 *
 *    The construction and usage of the program is managed in sequence-
 *    protected stages. Error messages will be printed if functions are
 *    called out of order.  The correct order is:
 *
 *          addShader()         // as many as you need
 *          init()              // called once
 *          [bindAttribute()]   // only if you wish, for VAO/VBOs
 *          linkAndValidate()   // must be run before using program
 *          addSampler()        // called after program is linked for safety
 *          enable()            // to actually use
 *          disable()           // when you're done
 *
 *    At the moment, the samplers may only be specified when calling
 *    setTexure() by remembering the order in which you added them with
 *    addSampler().
 *
 *  This is a work in progress and will be continually improved as I use it.
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

#ifndef PROGRAM_HPP_
#define PROGRAM_HPP_

#include <iostream>
#include <vector>
#include <deque>
#include "shaderobj.hpp"
#include "global.hpp"


/**
 * Stores pairings of generated GLSL samplers and their uniform names.
 */
typedef struct {
  GLuint samplerID;         /**< Generated sampler ID */
  std::string samplerName;  /**< Uniform name as string */
} SamplerInfo;


/**
 * Class representing an OpenGL shader program. Simplifies the initialization
 * and management of all sources and bindings.
 */
class Program {
    public:
        Program(QOpenGLFunctions_4_5_Core *funcPointer);
        virtual ~Program();

        int addShader(std::string fName, GLuint type);
        int addAllShaders(std::vector<std::string> *fList, GLuint type);
        void addDefaultShaders();
        void addSampler(std::string sName);

        void init();
        void bindAttribute(int location, std::string name);
        GLuint getShaderIdFromName(std::string& fileName);
        void attachShader(std::string& name);
        GLint linkAndValidate();
        void detachShaders();
        void detachDelete();
        void enable();
        void disable();

        void initVAO();
        void bindVAO();
        void clearVAO();

        GLuint bindVBO(uint bufSize, const GLfloat *buf, uint mode);
        void setAttributePointerFormat(GLuint attrIdx, GLuint binding, GLuint count, GLenum type, GLuint offset, GLuint step = 0);
        void setAttributeBuffer(GLuint binding, GLuint vboIdx, GLsizei stride);
        void enableAttribute(GLuint idx);
        void enableAttributes();
        void disableAttributes();
        void updateVBO(uint offset, uint bufSize, const GLfloat *buf);
        void clearVBO();

        void bindEBO(uint bufSize, const GLuint *buf, uint mode);
        void updateEBO(uint offset, uint bufSize, const GLuint *buf);
        void clearEBO();

        void assignFragColour();

        void beginRender();
        void endRender();
        
        void clearBuffers();
        void deleteBuffers();
        void addBuffers();

        void setUniform(int type, std::string name, double n);
        void setUniform(int type, std::string name, float n);
        void setUniform(int type, std::string name, int n);
        void setUniform(int type, std::string name, uint n);
        void setUniformv(int count, int size, int type, std::string name, const float *n);
        void setUniformMatrix(int size, std::string name, float *m);
        //void setTexture(int samplerIdx, TexInfo& texInfo);

        GLuint getProgramId();

        void displayLogProgram();
        void displayLogShader(GLenum shader);

    private:
        QOpenGLFunctions_4_5_Core *qgf = nullptr;
        std::vector<SamplerInfo> *samplers = nullptr;
        std::vector<Shader> registeredShaders;
        std::vector<GLuint> attribs;
        std::map<std::string, GLuint> compiledShaders;
        std::vector<GLuint> attachedShaders;

        GLuint programId = 0;
        GLuint vao;
        std::deque<GLuint> vbo;
        std::deque<GLuint> ebo;
        
        bool enabled = false;
        
        int stage = 0;
};

#endif /* PROGRAM_HPP_ */
