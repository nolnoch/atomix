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
#include <QVulkanWindow>
#include <iostream>
#include <vector>
#include <deque>
#include <map>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
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

enum class DataType : unsigned int {
    FLOAT           = 0,
    FLOAT_VEC2      = 1,
    FLOAT_VEC3      = 2,
    FLOAT_VEC4      = 3,
    INT             = 4,
    INT_VEC2        = 5,
    INT_VEC3        = 6,
    INT_VEC4        = 7,
    UINT            = 8,
    UINT_VEC2       = 9,
    UINT_VEC3       = 10,
    UINT_VEC4       = 11,
    DOUBLE          = 12,
    DOUBLE_VEC2     = 13,
    DOUBLE_VEC3     = 14,
    DOUBLE_VEC4     = 15
};

std::array<VkFormat, 16> DataFormats = {
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R64_SFLOAT,
    VK_FORMAT_R64G64_SFLOAT,
    VK_FORMAT_R64G64B64_SFLOAT,
    VK_FORMAT_R64G64B64A64_SFLOAT
};

enum class BufferType : unsigned int {
    VERTEX = 0,
    INDEX  = 1,
    UNIFORM = 2
};

struct AtomixDevice {
    QVulkanWindow *window = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
};

struct ModelCreateInfo {
    std::string name;
    uint vboCount = 0;
    BufferCreateInfo *vbos = nullptr;
    BufferCreateInfo *ibo = nullptr;
    BufferCreateInfo *ubo = nullptr;
    std::string vertShader;
    std::string fragShader;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
};

struct ModelInfo {
    uint id = 0;
    std::vector<BufferInfo *> vbos;
    BufferInfo *ibo = nullptr;
    BufferInfo *ubo = nullptr;
    ShaderInfo *shaders = nullptr;
    AttribInfo *attributes = nullptr;
    ModelPipelineInfo *pipe = nullptr;
};

struct BufferCreateInfo {
    std::string name;
    uint id = 0;
    BufferType type;
    uint binding = 0;
    uint size = 0;
    void *data = nullptr;
    bool storeData = false;
    std::vector<DataType> dataTypes;
};

struct BufferInfo {
    uint id = 0;
    BufferType type;
    uint binding = 0;
    uint size = 0;
    void *data = nullptr;
    std::vector<DataType> dataTypes;
};

struct AttribInfo {
    std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
};

struct DescInfo {
    std::string name;
    uint location = 0;
    uint binding = 0;
};

struct ShaderInfo {
    Shader *vertShader = nullptr;
    Shader *fragShader = nullptr;
    Shader *geomShader = nullptr;
    Shader *computeShader = nullptr;
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkShaderModule geomModule = VK_NULL_HANDLE;
    VkShaderModule computeModule = VK_NULL_HANDLE;
};

struct ProgUniInfo {
    std::string name;
    uint location = 0;
    uint binding = 0;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
};

struct SwapChainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct ModelPipelineInfo {
    std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBindings;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vbo{};
    VkPipelineInputAssemblyStateCreateInfo ia{};
    VkPipelineRasterizationStateCreateInfo rs{};
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
};

struct PipelineInfo {
    VkDynamicState dynStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineViewportStateCreateInfo vp{};
    VkPipelineDynamicStateCreateInfo dyn{};
    VkPipelineMultisampleStateCreateInfo ms{};
    VkPipelineDepthStencilStateCreateInfo ds{};
    VkPipelineColorBlendStateCreateInfo cb{};
    VkPipelineColorBlendAttachmentState cbAtt{};
    VkPipelineLayoutCreateInfo pipeLayInfo{};
    VkPipelineLayout pipeLayout;
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
    ProgramVK();
    virtual ~ProgramVK();
    void cleanup();

    void setInstance(AtomixDevice *atomixDevice);

    bool addShader(std::string fName, VKuint type);
    int addAllShaders(std::vector<std::string> *fList, VKuint type);
    bool compileShader(Shader *shader);
    int compileAllShaders();
    void bindShader(std::string name);

    BufferInfo* addBuffer(BufferCreateInfo &info);
    void addModel(ModelCreateInfo &info);

    bool init();
    void createSwapChain();
    void createCommandPool();
    void createCommandBuffers();
    void createRenderPass();
    void createShaderStages(ModelInfo *model);

    void pipelineModelSetup(ModelInfo *model);
    void assemblePipeline();
    void createPipeline(ModelInfo *model);

    void defineModelAttributes(ModelInfo *model);
    void defineDescriptorSets(ModelInfo *model);
    void lockModelAttributes(ModelInfo *model);

    void createVertexBuffer(BufferInfo *buf);
    void createIndexBuffer(BufferInfo *buf);
    void createPersistentUniformBuffer(BufferInfo *buf);
    void createDescriptorSet();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    SwapChainSupportInfo querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    BufferInfo* getBufferInfo(std::string name);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

    void recordCommandBuffer();

    void updateVBO(std::string name, VKuint *data);
    void updateIBO(VKuint *data);
    void updateUBO(VKuint *data);
    void render();

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
    Shader* getShaderFromName(std::string& fileName);

    void displayLogProgramVK();
    void displayLogShader(VKenum shader);

    void printModel(ModelInfo *model);


private:
    void addSampler(std::string sName);

    std::vector<SamplerInfo> *samplers = nullptr;
    std::vector<Shader *> boundShaders;
    std::vector<VKuint> attribs;

    const uint MAX_FRAMES_IN_FLIGHT = QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT;

    VkDevice p_dev = VK_NULL_HANDLE;
    VkPhysicalDevice p_phydev = VK_NULL_HANDLE;
    VkCommandPool p_cmdpool = VK_NULL_HANDLE;
    VkQueue p_queue = VK_NULL_HANDLE;
    VkCommandBuffer p_cmdbuff = VK_NULL_HANDLE;
    VkFramebuffer p_frames[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT] = { 0 };
    VkPipeline p_pipe = VK_NULL_HANDLE;
    VkPipelineLayout p_pipeLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout p_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool p_descPool = VK_NULL_HANDLE;
    VkRenderPass p_renderPass = VK_NULL_HANDLE;
    VkExtent2D p_swapExtent = { 0, 0 };
    VkClearValue p_clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};;

    VkShaderModule p_shaderVert = VK_NULL_HANDLE;
    VkShaderModule p_shaderFrag = VK_NULL_HANDLE;
    VkShaderModule p_shaderGeom = VK_NULL_HANDLE;
    VkShaderModule p_shaderComp = VK_NULL_HANDLE;

    PipelineInfo p_pipeInfo;

    VkViewport p_viewport = { 0, 0, 0, 0 };
    VkRect2D p_scissor = { {0, 0}, {0, 0} };

    QVulkanDeviceFunctions *p_vdf = nullptr;
    QVulkanFunctions *p_vf = nullptr;
    QVulkanInstance *p_vi = nullptr;
    QVulkanWindow *p_vkw = nullptr;
    
    std::vector<Shader *> registeredShaders;
    std::vector<Shader *> compiledShaders;
    std::vector<BufferInfo *> p_buffers;
    std::vector<ModelInfo *> p_models;
    std::map<std::string, VKuint> p_mapBuf;
    std::map<std::string, VKuint> p_mapModel;
    std::vector<VKuint> p_activeModels;
    std::vector<void *> p_allocatedBuffers;

    std::vector<VkDescriptorSet> p_descSets;
    std::vector<VkBuffer> p_uniformBuffers;
    std::vector<VkDeviceMemory> p_uniformMemory;
    std::vector<void *> uniformMappings;

    // std::vector<VkBuffer> p_stagingBuffers;
    // std::vector<VkDeviceMemory> p_stagingMemory;
    std::vector<VkBuffer> p_vertexBuffers;
    std::vector<VkDeviceMemory> p_vertexMemory;
    VkBuffer p_stagingBuffer, p_indexBuffer;
    VkDeviceMemory p_stagingMemory, p_indexMemory;
    
    VkResult err;
    
    bool enabled = false;
    int stage = 0;

    std::array<unsigned int, 16> dataSizes = {
        sizeof(float),
        sizeof(float) * 2,
        sizeof(float) * 3,
        sizeof(float) * 4,
        sizeof(int),
        sizeof(int) * 2,
        sizeof(int) * 3,
        sizeof(int) * 4,
        sizeof(uint),
        sizeof(uint) * 2,
        sizeof(uint) * 3,
        sizeof(uint) * 4,
        sizeof(double),
        sizeof(double) * 2,
        sizeof(double) * 3,
        sizeof(double) * 4
    };
};

#endif /* PROGRAMVK_HPP_ */
