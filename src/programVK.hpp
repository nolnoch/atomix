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

typedef uint64_t VKuint64;
typedef uint32_t VKuint;
typedef uint32_t VKenum;
typedef float VKfloat;
typedef char VKchar;
typedef int VKint;
typedef int VKsizei;

enum class DataType : uint32_t {
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

enum class BufferType : unsigned int {
    VERTEX = 0,
    INDEX  = 1,
    UNIFORM = 2,
    DATA = 3
};

struct AtomixDevice {
    QVulkanWindow *window = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
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

struct BufferCreateInfo {
    std::string name;
    BufferType type;
    uint64_t count = 0;
    uint64_t size = 0;
    const void *data = nullptr;
    bool storeData = false;
    std::vector<DataType> dataTypes;
};

struct BufferUpdateInfo {
    std::string modelName;
    std::string bufferName;
    BufferType type;
    uint64_t count = 0;
    uint64_t size = 0;
    const void *data = nullptr;
};

struct OffsetInfo {
    VkDeviceSize offset = 0;
    VKuint vertShaderIndex = 0;
    VKuint fragShaderIndex = 0;
    VKuint topologyIndex = 0;
};

struct ModelCreateInfo {
    std::string name;
    std::vector<BufferCreateInfo *> vbos;
    BufferCreateInfo *ibo = nullptr;
    VkDeviceSize uboSize = 0;
    std::vector<std::string> vertShaders;
    std::vector<std::string> fragShaders;
    std::vector<VkPrimitiveTopology> topologies;
    std::vector<OffsetInfo> offsets;
};

struct PipelineLibrary {
    std::vector<VkPipeline> vertexInput;
    std::vector<VkPipeline> preRasterization;
    std::vector<VkPipeline> fragmentShader;
};

struct ModelPipelineInfo {
    std::vector<VkPipelineShaderStageCreateInfo> vsCreates;
    std::vector<VkPipelineShaderStageCreateInfo> fsCreates;
    VkPipelineVertexInputStateCreateInfo vboCreate;
    std::vector<VkPipelineInputAssemblyStateCreateInfo> iaCreates;
    std::vector<VkPipelineRasterizationStateCreateInfo> rsCreates;
    PipelineLibrary *pipeLib = nullptr;
};

struct GlobalPipelineInfo {
    VkDynamicState dynStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineViewportStateCreateInfo vp{};
    VkPipelineTessellationStateCreateInfo ts{};
    VkPipelineDynamicStateCreateInfo dyn{};
    VkPipelineMultisampleStateCreateInfo ms{};
    VkPipelineDepthStencilStateCreateInfo ds{};
    VkPipelineColorBlendStateCreateInfo cb{};
    VkPipelineColorBlendAttachmentState cbAtt{};
    VkPipelineLayoutCreateInfo pipeLayInfo{};
    VkPipelineLayout pipeLayout;
    bool init = false;
};

struct RenderInfo {
    VkPipeline pipeline;
    VkBuffer *firstVbo;
    VkBuffer *ibo;
    std::vector<VkDeviceSize> vboOffsets;
    VkDeviceSize iboOffset;
    uint64_t indexCount;
};

struct AttribInfo {
    std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
};

struct ShaderInfo {
    std::map<VKuint, VKuint> vertModulesToCreates;
    std::map<VKuint, VKuint> fragModulesToCreates;
    std::vector<VkShaderModule> vertModules;
    std::vector<VkShaderModule> fragModules;
};

struct ModelInfo {
    uint id = 0;
    std::string name;
    std::vector<VkBuffer> vbos;
    std::vector<VkDeviceMemory> vboMemory;
    VkBuffer ibo = nullptr;
    VkDeviceMemory iboMemory = nullptr;
    ShaderInfo *shaders = nullptr;
    AttribInfo *attributes = nullptr;
    ModelPipelineInfo *pipeInfo = nullptr;
    std::vector<RenderInfo *> renders;
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

    bool addShader(const std::string& fName, VKuint type);
    int addAllShaders(std::vector<std::string> *fList, VKuint type);
    bool compileShader(Shader *shader);
    int compileAllShaders();
    VkShaderModule createShaderModule(Shader *shader);
    void createShaderStages(ModelInfo *model);
    AttribInfo* defineModelAttributes(ModelCreateInfo *info);

    VKuint addModel(ModelCreateInfo &info);
    VKuint activateModel(const std::string& name);
    VKuint deactivateModel(const std::string& name);

    void createPipelineCache();
    void savePipelineToCache();
    void loadPipelineFromCache();

    bool init();
    void createCommandPool();
    void createCommandBuffers();
    void createRenderPass();
    void defineDescriptorSets();

    void pipelineModelSetup(ModelCreateInfo *info, ModelInfo *m);
    void pipelineGlobalSetup();
    void createPipeline(RenderInfo *render, ModelInfo *m, int vs, int fs, int ia, int rs);
    void genVertexInputPipeLib(ModelInfo *model, VkPipelineVertexInputStateCreateInfo &vbo, VkPipelineInputAssemblyStateCreateInfo &ia);
    void genPreRasterizationPipeLib(ModelInfo *model, VkPipelineShaderStageCreateInfo &vert, VkPipelineRasterizationStateCreateInfo &rs);
    void genFragmentShaderPipeLib(ModelInfo *model, VkPipelineShaderStageCreateInfo &frag);
    void genFragmentOutputPipeLib();
    void createPipeFromLibraries(RenderInfo *render, VkPipeline &vis, VkPipeline &pre, VkPipeline &frag);
    void updatePipeFromLibraries();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size);
    void stageAndCopyVertexBuffer(ModelInfo *model, VKuint idx, bool update, VKuint64 size, const void *data);
    void stageAndCopyIndexBuffer(ModelInfo *model, bool update, VKuint64 size, const void *data);
    void createPersistentUniformBuffer(VkDeviceSize bufferSize);
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memProperties);

    void updateRender(const std::string& modelName, OffsetInfo &info);
    void updateBuffer(BufferUpdateInfo &info);
    void updateUniformBuffer(uint32_t currentImage, uint32_t uboSize, const void *uboData);
    void updateClearColor(float r, float g, float b, float a);
    void updateSwapExtent(int x, int y);

    void render(VkCommandBuffer& cmdBuff, VkExtent2D &extent);
    void renderMacOS(VkCommandBuffer& cmdBuff, VkExtent2D &extent);

    Shader* getShaderFromName(const std::string& fileName);
    ModelInfo* getModelFromName(const std::string& modelName);
    VKuint getModelIdFromName(const std::string& name);
    std::vector<VKuint> getActiveModelsById();
    std::vector<std::string> getActiveModelsByName();

    bool isActive(const std::string& modelName);

    void printModel(ModelInfo *model);


private:
    const uint MAX_FRAMES_IN_FLIGHT = QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT;

    VkDevice p_dev = VK_NULL_HANDLE;
    VkPhysicalDevice p_phydev = VK_NULL_HANDLE;
    VkCommandPool p_cmdpool = VK_NULL_HANDLE;
    VkQueue p_queue = VK_NULL_HANDLE;
    VkCommandBuffer p_cmdbuff = VK_NULL_HANDLE;
    VkFramebuffer p_frames[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT] = { 0 };
    VkPipelineLayout p_pipeLayout = VK_NULL_HANDLE;
    VkPipelineCache p_pipeCache = VK_NULL_HANDLE;
    VkDescriptorSetLayout p_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool p_descPool = VK_NULL_HANDLE;
    VkRenderPass p_renderPass = VK_NULL_HANDLE;
    VkExtent2D p_swapExtent = { 0, 0 };
    VkClearValue p_clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkViewport p_viewport = { 0, 0, 0, 0 };
    VkRect2D p_scissor = { {0, 0}, {0, 0} };

    QVulkanDeviceFunctions *p_vdf = nullptr;
    QVulkanFunctions *p_vf = nullptr;
    QVulkanInstance *p_vi = nullptr;
    QVulkanWindow *p_vkw = nullptr;
    
    std::vector<Shader *> p_registeredShaders;
    std::vector<Shader *> p_compiledShaders;
    std::vector<ModelInfo *> p_models;
    std::vector<VKuint> p_activeModels;
    std::map<std::string, VKuint> p_mapModels;
    std::map<std::string, VKuint> p_mapBuffers;
    std::vector<const void *> p_allocatedBuffers;

    std::vector<VkDescriptorSet> p_descSets;
    std::vector<VkBuffer> p_uniformBuffers;
    std::vector<VkDeviceMemory> p_uniformMemory;
    std::vector<void *> uniformMappings;

    VkBuffer p_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory p_stagingMemory = VK_NULL_HANDLE;

    std::vector<VkPipeline> p_pipelines;
    GlobalPipelineInfo p_pipeInfo{};
    VkPipeline p_fragmentOutput = VK_NULL_HANDLE;
    
    VkResult err = VK_SUCCESS;
    
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

    std::array<VkFormat, 16> dataFormats = {
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
};

#endif /* PROGRAMVK_HPP_ */
