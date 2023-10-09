/**
 * program.hpp
 *
 *    Created on: Apr 8, 2013
 *   Last Update: Apr 21, 2013
 *  Orig. Author: Wade Burch (nolnoch@cs.utexas.edu)
 *  Contributors: [none]
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
 *  Feel free to share, expand, and modify as you see fit with attribution
 *  to the original author (me) and any who have added since.
 *
 *  -Wade Burch
 */

#ifndef PROGRAM_HPP_
#define PROGRAM_HPP_

#include <iostream>
#include <vector>

#include "shaderobj.hpp"


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

  int addShader(std::string fName, int type);
  void addDefaultShaders();
  void addSampler(std::string sName);

  void init();
  void bindAttribute(int location, std::string name);
  GLint linkAndValidate();
  void enable();
  void disable();

  void setUniform(int type, std::string name, float n);
  void setUniformv(int count, int type, std::string name, const float *n);
  void setUniformMatrix(int size, std::string name, float *m);
  //void setTexture(int samplerIdx, TexInfo& texInfo);

  GLuint getProgramId();

  void displayLogProgram();
  void displayLogShader(GLenum shader);

 private:
  QOpenGLFunctions_4_5_Core *qgf = nullptr;
  GLuint programId;
  std::vector<Shader> shaders;
  std::vector<SamplerInfo> *samplers;
  int stage;
};

#endif /* PROGRAM_HPP_ */
