/**
 * programVK.hpp
 *
 *    Created on: Oct 21, 2013
 *   Last Update: Oct 21, 2024
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
 *  Copyright 2013, 2023, 2024 Wade Burch (GPLv3)
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

#ifndef PROGRAMVK_HPP_
#define PROGRAMVK_HPP_

#include <QVulkanDeviceFunctions>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shaderobj.hpp"
#include "global.hpp"

typedef unsigned int VKuint;
typedef unsigned int VKenum;
typedef float VKfloat;
typedef char VKchar;
typedef int VKint;
typedef int VKsizei;

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30

struct AtomixDevice {
    QVulkanInstance *instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
};
Q_DECLARE_METATYPE(AtomixDevice);

struct ProgBufInfo {
    std::string name;
    uint bufSize;
    void *data;
    uint bufId;
    uint binding;
    uint locationCount;
    uint *types;
    uint *sizes;
};

struct ProgUniInfo {
    std::string name;
    uint location;
    uint binding;
};

/**
 * Stores pairings of generated GLSL samplers and their uniform names.
 */
struct SamplerInfo {
    VKuint samplerID;         /**< Generated sampler ID */
    std::string samplerName;  /**< Uniform name as string */
};


/**
 * Class representing an OpenGL shader program. Simplifies the initialization
 * and management of all sources and bindings.
 */
class ProgramVK {
public:
    ProgramVK(AtomixDevice *atomixDevice);
    virtual ~ProgramVK();

    int addShader(std::string fName, VKuint type);
    int addAllShaders(std::vector<std::string> *fList, VKuint type);
    void addDefaultShaders();
    void addSampler(std::string sName);

    void addBufferConfig(ProgBufInfo &info);

    void init();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createVertexBuffer(std::string name);
    void createIndexBuffer(std::string name);

    void bindAttribute(int location, std::string name);
    
    void attachShader(std::string& name);
    VKint linkAndValidate();
    void detachShaders();
    void detachDelete();
    void enable();
    void disable();

    void initVAO();
    void bindVAO();
    void clearVAO();

    VKuint bindVBO(std::string name, uint bufCount, uint bufSize, const VKfloat *buf, uint mode);
    void setAttributePointerFormat(VKuint attrIdx, VKuint binding, VKuint count, VKenum type, VKuint offset, VKuint step = 0);
    void setAttributeBuffer(VKuint binding, VKuint vboIdx, VKsizei stride);
    void enableAttribute(VKuint idx);
    void enableAttributes();
    void disableAttributes();
    void updateVBO(uint offset, uint bufCount, uint bufSize, const VKfloat *buf);
    void updateVBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const VKfloat *buf);
    void resizeVBONamed(std::string name, uint bufCount, uint bufSize, const VKfloat *buf, uint mode);
    void clearVBO();

    VKuint bindEBO(std::string name, uint bufCount, uint bufSize, const VKuint *buf, uint mode);
    void updateEBO(uint offset, uint bufCount, uint bufSize, const VKuint *buf);
    void updateEBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const VKuint *buf);
    void resizeEBONamed(std::string name, uint bufCount, uint bufSize, const VKuint *buf, uint mode);
    void clearEBO();

    void assignFragColour();

    void beginRender();
    void endRender();
    
    void clearBuffers();
    void deleteBuffer(std::string name);
    
    bool hasBuffer(std::string name);

    void setUniform(int type, std::string name, double n);
    void setUniform(int type, std::string name, float n);
    void setUniform(int type, std::string name, int n);
    void setUniform(int type, std::string name, uint n);
    void setUniformv(int count, int size, int type, std::string name, const float *n);
    void setUniformMatrix(int size, std::string name, float *m);
    //void setTexture(int samplerIdx, TexInfo& texInfo);

    VKuint getProgramVKId();
    VKuint getFirstVBOId();
    VKuint getLastVBOId();
    VKuint getSize(std::string name);
    VKuint getShaderIdFromName(std::string& fileName);

    void displayLogProgramVK();
    void displayLogShader(VKenum shader);

private:
    std::vector<SamplerInfo> *samplers = nullptr;
    std::vector<Shader> registeredShaders;
    std::map<std::string, VKuint> compiledShaders;
    std::vector<VKuint> attachedShaders;
    std::vector<VKuint> attribs;

    std::deque<ProgBufInfo *> buffers;
    std::deque<ProgUniInfo *> uniforms;

    // std::map<std::string, glm::uvec3> buffers;

    VkDevice p_dev = VK_NULL_HANDLE;
    VkPhysicalDevice p_pdev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *p_vdf = nullptr;
    QVulkanFunctions *p_vf = nullptr;
    QVulkanInstance *p_vi = nullptr;
    VkResult err;
    
    bool enabled = false;
    
    int stage = 0;
};

#endif /* PROGRAM_HPP_ */
