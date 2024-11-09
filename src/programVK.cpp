/**
 * programVK.cpp
 *
 *    Created on: Oct 21, 2013
 *   Last Update: Oct 21, 2024
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

#include "programVK.hpp"


/**
 * Default Constructor.
 */
ProgramVK::ProgramVK() {
}

/**
 * Default Destructor.
 */
ProgramVK::~ProgramVK() {
    cleanup();
}

void ProgramVK::cleanup() {
    // destruct models
    for (auto &model : p_models) {
        // vbos
        for (auto &buf : model->vbos) {
            this->p_vdf->vkDestroyBuffer(this->p_dev, buf, nullptr);
        }
        for (auto &buf : model->vboMemory) {
            this->p_vdf->vkFreeMemory(this->p_dev, buf, nullptr);
        }
        
        // ibo
        this->p_vdf->vkDestroyBuffer(this->p_dev, model->ibo, nullptr);
        this->p_vdf->vkFreeMemory(this->p_dev, model->iboMemory, nullptr);
        
        // shaders
        for (auto &shader : model->shaders->vertModules) {
            this->p_vdf->vkDestroyShaderModule(this->p_dev, shader, nullptr);
        }
        for (auto &shader : model->shaders->fragModules) {
            this->p_vdf->vkDestroyShaderModule(this->p_dev, shader, nullptr);
        }
        delete model->shaders;

        // attributes
        delete model->attributes;

        // pipeline libraries
        if (p_libEnabled) {
            for (auto &pipe : model->pipeInfo->pipeLib->vertexInput) {
                this->p_vdf->vkDestroyPipeline(this->p_dev, pipe, nullptr);
            }
            for (auto &pipe : model->pipeInfo->pipeLib->preRasterization) {
                this->p_vdf->vkDestroyPipeline(this->p_dev, pipe, nullptr);
            }
            for (auto &pipe : model->pipeInfo->pipeLib->fragmentShader) {
                this->p_vdf->vkDestroyPipeline(this->p_dev, pipe, nullptr);
            }
            delete model->pipeInfo->pipeLib;
        }
        delete model->pipeInfo;
        
        // pipelines
        for (auto &render : model->renders) {
            this->p_vdf->vkDestroyPipeline(this->p_dev, render->pipeline, nullptr);
            delete render;
        }
        
        delete model;
    }

    // descriptor sets
    this->p_vdf->vkDestroyDescriptorSetLayout(this->p_dev, p_descSetLayout, nullptr);
    this->p_vdf->vkDestroyDescriptorPool(this->p_dev, p_descPool, nullptr);

    // destruct uniform buffers
    for (int i = 0; i < p_uniformBuffers.size(); i++) {
        this->p_vdf->vkDestroyBuffer(this->p_dev, p_uniformBuffers[i], nullptr);
        this->p_vdf->vkFreeMemory(this->p_dev, p_uniformMemory[i], nullptr);
    }
    uniformMappings.clear();

    // global pipeline objects
    this->p_vdf->vkDestroyPipeline(this->p_dev, p_fragmentOutput, nullptr);
    this->p_vdf->vkDestroyPipelineCache(this->p_dev, p_pipeCache, nullptr);
    this->p_vdf->vkDestroyPipelineLayout(this->p_dev, p_pipeLayout, nullptr);

    // shader objects
    for (auto &shader : p_registeredShaders) {
        delete shader;
    }  
}

void ProgramVK::setInstance(AtomixDevice *atomixDevice) {
    p_dev = atomixDevice->device;
    p_phydev = atomixDevice->physicalDevice;
    p_vkw = atomixDevice->window;
    p_vi = p_vkw->vulkanInstance();
    p_vdf = p_vi->deviceFunctions(p_dev);
    p_vf = p_vi->functions();
}

/**
 * Associate a shader source file with the program as a Shader object.
 * This will populate the Shader with its std::string-parsed source, but init()
 * must still be called to compile and attach the shader to the program.
 *
 * This function will return 0 upon error and automatically remove the
 * failed shader from the program's list of Shaders.
 * @param fName - std::string representation of the shader filename
 * @param type - GLEW-defined constant, one of: GL_*_SHADER, where * is
 *               VERTEX, FRAGMENT, GEOMETRY, or COMPUTE
 * @return true on success or false on error
 */
bool ProgramVK::addShader(const std::string &fName, VKuint type) {
    bool validFile = false;
    std::string fileLoc;
    
    if (fName.find('/') == std::string::npos) {
        fileLoc = atomixFiles.shaders() + fName;
    } else {
        fileLoc = fName;
    }

    Shader *shader = new Shader(fileLoc, type);

    for (auto &s : this->p_registeredShaders) {
        if (s->getName() == shader->getName()) {
            std::cout << "Shader already registered: " << fName << std::endl;
            delete shader;
            return false;
        }
    }

    if (!shader->isValidFile()) {
        delete shader;
        std::cout << "Failed to add shader source: " << fName << std::endl;
    } else {
        this->p_registeredShaders.push_back(shader);
        validFile = true;
        this->stage = 1;
    }

    return validFile;
}


/**
 * Associate N shader source files with the program as Shader objects.
 * This will populate the Shaders with their std::string-parsed sources, but init()
 * must still be called to compile and attach the shader(s) to the program.
 *
 * @param fList - vector of std::strings representing the shader filenames
 * @param type - GLEW-defined constant, one of: GL_*_SHADER, where * is
 *               VERTEX, FRAGMENT, GEOMETRY, or COMPUTE
 * @return 0 on success or the number of errors on failure
 */
int ProgramVK::addAllShaders(std::vector<std::string> *fList, VKuint type) {
    int errors = fList->size();
    VKuint shID, shIdx = 0;
    
    for (auto &fName : *fList) {
        bool validFile = addShader(fName, type);
    
        if (validFile) {
            errors--;
        } else {
            std::cout << "Failed to add shader source." << std::endl;
        }
    }

    return errors;
}

bool ProgramVK::compileShader(Shader *shader) {
    bool compiled = shader->compile(VK_SPIRV_VERSION);

    if (compiled) {
        this->p_compiledShaders.push_back(shader);
    } else {
        std::cout << "Failed to compile shader. Deleting shader..." << std::endl;
        delete shader;
    }

    return compiled;
}

/**
 * Compile all shaders that have been added with addShader() or
 * addAllShaders().  This function will return the number of errors
 * encountered during compilation.
 *
 * @return number of errors, or 0 if all shaders compiled successfully
 */
int ProgramVK::compileAllShaders() {
    int errors = this->p_registeredShaders.size();

    /* auto it = this->p_registeredShaders.begin();
    while (it != this->p_registeredShaders.end()) {
        if (this->compileShader(*it)) {
            errors--;
            it++;
        } else {
            it = this->p_registeredShaders.erase(it);
        }
    } */

    std::erase_if(this->p_registeredShaders, [this](Shader *s) { return !this->compileShader(s); });

    errors -= this->p_registeredShaders.size();

    return errors;
}

VkShaderModule ProgramVK::createShaderModule(Shader *shader) {
    if (std::find(this->p_compiledShaders.cbegin(), this->p_compiledShaders.cend(), shader) == this->p_compiledShaders.cend()) {
        throw std::runtime_error("Shader not compiled: " + shader->getName());
    }

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = shader->getLengthCompiled() * sizeof(uint32_t);
    moduleCreateInfo.pCode = shader->getSourceCompiled();

    VkShaderModule shaderModule;
    err = p_vdf->vkCreateShaderModule(p_dev, &moduleCreateInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module: " + std::to_string(err));
    }

    return shaderModule;
}

void ProgramVK::createShaderStages(ModelInfo *m) {
    VKuint i = 0;

    for (auto &module : m->shaders->vertModules) {
        VkPipelineShaderStageCreateInfo shaderStage{};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStage.module = module;
        shaderStage.pName = "main";
        m->pipeInfo->vsCreates.push_back(shaderStage);
        m->shaders->vertModulesToCreates[i] = i++;
    }

    for (auto &module : m->shaders->fragModules) {
        VkPipelineShaderStageCreateInfo shaderStage{};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStage.module = module;
        shaderStage.pName = "main";
        m->pipeInfo->fsCreates.push_back(shaderStage);
    }
}

VKuint ProgramVK::addModel(ModelCreateInfo &info) {
    ModelInfo *model;
    VKuint idx = p_models.size();

    printInfo(&info);

    const auto [it, success] = p_mapModels.insert({info.name, idx});

    // Check for existing model and add if it doesn't exist
    if (!success) {
        std::cout << "Model already exists. Updating model " << info.name << "..." << std::endl;
        model = p_models[it->second];

        // TODO: Handle old model data
        return -1;
    } else {
        model = new ModelInfo{};
        model->id = idx;
        model->name = info.name;
        p_models.push_back(model);
        p_mapModels[info.name] = model->id;
    }

    // Shaders: Modules
    model->shaders = new ShaderInfo{};
    for (auto &vert : info.vertShaders) {
        Shader *v = getShaderFromName(vert);
        model->shaders->vertModules.push_back(createShaderModule(v));
        model->shaders->vertShaders.push_back(v);
    }
    for (auto &frag : info.fragShaders) {
        Shader *f = getShaderFromName(frag);
        model->shaders->fragModules.push_back(createShaderModule(f));
        model->shaders->fragShaders.push_back(f);
    }
    // Shaders: Stages
    model->pipeInfo = new ModelPipelineInfo{};
    this->createShaderStages(model);

    // Buffers: VBO
    for (auto &vbo : info.vbos) {
        model->vbos.push_back(VkBuffer{});
        model->vboMemory.push_back(VkDeviceMemory{});
        this->stageAndCopyVertexBuffer(model, model->vbos.size() - 1, false, vbo->size, vbo->data);
        p_mapBuffers[vbo->name] = model->vbos.size() - 1;
    }
    // Buffers: IBO
    this->stageAndCopyIndexBuffer(model, false, info.ibo->size, info.ibo->data);
    p_mapBuffers[info.ibo->name] = model->id;
    // Buffers: UBO
    this->createPersistentUniformBuffer(info.uboSize);

    // Attributes -- Bindings/Locations
    model->attributes = defineModelAttributes(&info);
    this->pipelineModelSetup(&info, model);

    // Pipeline Global Setup
    if (!this->p_pipeInfo.init) {
        this->pipelineGlobalSetup();
    }
    
    if (p_libEnabled) {
        // Pipeline Libraries
        model->pipeInfo->pipeLib = new PipelineLibrary{};
        for (auto &ia : model->pipeInfo->iaCreates) {
            this->genVertexInputPipeLib(model, model->pipeInfo->vboCreate, ia);
        }
        for (auto &vert : model->pipeInfo->vsCreates) {
            this->genPreRasterizationPipeLib(model, vert, model->pipeInfo->rsCreate);
        }
        for (auto &frag : model->pipeInfo->fsCreates) {
            this->genFragmentShaderPipeLib(model, frag);
        }
        if (!this->p_fragmentOutput) {
            this->genFragmentOutputPipeLib();
        }
    }

    std::vector<VKuint> indexCount;
    for (int i = 0; i < info.offsets.size(); i++) {
        VKuint end = 0;
        if (i < info.offsets.size() - 1) {
            end = info.offsets[i+1].offset;
        } else {
            end = info.ibo->count;
        }
        indexCount.push_back(end - info.offsets[i].offset);
    }

    // Populate Renders
    int i = 0;
    for (auto &off : info.offsets) {
        model->renders.push_back(new RenderInfo{});
        RenderInfo *render = model->renders.back();
        render->firstVbo = model->vbos.data();
        render->ibo = &model->ibo;
        render->vboOffsets.resize(model->vbos.size(), 0);
        render->pushConst = { info.pushConstantsSize, info.pushConstantsData };
        render->iboOffset = off.offset;
        render->indexCount = indexCount[i++];
        
        // Not all platforms support libraries
        if (p_libEnabled) {
            // Final Pipelines for Renders (Libraries)
            this->createPipeFromLibraries(render,
                                            model->pipeInfo->pipeLib->vertexInput[off.topologyIndex],
                                            model->pipeInfo->pipeLib->preRasterization[off.vertShaderIndex],
                                            model->pipeInfo->pipeLib->fragmentShader[off.fragShaderIndex]);
        } else {
            // Final Pipelines for Renders (Full PSO)
            this->createPipeline(render, model, off.vertShaderIndex, off.fragShaderIndex, off.topologyIndex);
        }
    }

    printModel(model);

    return model->id;
}

VKuint ProgramVK::activateModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    p_activeModels.push_back(id);
    
    return id;
}

VKuint ProgramVK::deactivateModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);

    if (!std::erase(p_activeModels, id)) {
        std::cout << "Model not found and not removed from active models: " << name << std::endl;
        id = -1;
    }

    return id;
}

void ProgramVK::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCreateInfo.flags = 0;
    pipelineCacheCreateInfo.pNext = nullptr;
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData = nullptr;

    err = p_vdf->vkCreatePipelineCache(p_dev, &pipelineCacheCreateInfo, nullptr, &p_pipeCache);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline cache: " + std::to_string(err));
    }
}

void ProgramVK::savePipelineToCache() {
    std::cout << "Saving pipeline to cache..." << std::endl;

    err = p_vdf->vkGetPipelineCacheData(p_dev, p_pipeCache, nullptr, nullptr);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to get pipeline cache data: " + std::to_string(err));
    }
}

void ProgramVK::loadPipelineFromCache() {
    std::cout << "Loading pipeline from cache..." << std::endl;

}

/**
 * Initializes the program. Then initializes, loads, and compiles all shaders
 * associated with the ProgramVK object.
 */
bool ProgramVK::init() {
    int numShaders = this->p_registeredShaders.size();

    if (!numShaders || !stage) {
        std::cout << "No shader files associated with program. Aborting..." << std::endl;
        return false;
    }

    // Link Command Pool, Queue, and Render Pass to Qt defaults
    this->p_cmdpool = this->p_vkw->graphicsCommandPool();
    this->p_queue = this->p_vkw->graphicsQueue();
    this->p_renderPass = this->p_vkw->defaultRenderPass();

    // Process registered shaders
    compileAllShaders();

    // Init pipeline cache and global setup
    createPipelineCache();

    this->stage = 2;

    return true;
}

QueueFamilyIndices ProgramVK::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    this->p_vf->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    this->p_vf->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            break;
        }
        i++;
    }

    return indices;
}

void ProgramVK::createRenderPass() {
    VkAttachmentDescription2 colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    colorAttachment.format = this->p_vkw->colorFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference2 colorAttachmentRef{};
    colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription2 subpass{};
    subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo2 renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if ((this->p_vdf->vkCreateRenderPass2(this->p_dev, &renderPassInfo, nullptr, &this->p_renderPass)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void ProgramVK::pipelineModelSetup(ModelCreateInfo *info, ModelInfo *m) {
    // For each topology
    for (auto &topology : info->topologies) {
        // Input Assembly
        m->pipeInfo->iaCreates.push_back({});
        VkPipelineInputAssemblyStateCreateInfo *ia = &m->pipeInfo->iaCreates.back();
        ia->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia->topology = topology;
        ia->primitiveRestartEnable = VK_FALSE;
        ia->flags = 0;
        ia->pNext = nullptr;
    }

    // Rasterization
    m->pipeInfo->rsCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m->pipeInfo->rsCreate.pNext = nullptr;
    m->pipeInfo->rsCreate.flags = 0;
    m->pipeInfo->rsCreate.polygonMode = VK_POLYGON_MODE_FILL;
    m->pipeInfo->rsCreate.cullMode = VK_CULL_MODE_BACK_BIT;  // VK_CULL_MODE_BACK_BIT or VK_CULL_MODE_NONE
    m->pipeInfo->rsCreate.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m->pipeInfo->rsCreate.lineWidth = 1.0f;
    m->pipeInfo->rsCreate.rasterizerDiscardEnable = VK_FALSE;
    m->pipeInfo->rsCreate.depthClampEnable = VK_FALSE;
    m->pipeInfo->rsCreate.depthBiasEnable = VK_FALSE;
    m->pipeInfo->rsCreate.depthBiasConstantFactor = 0.0f;
    m->pipeInfo->rsCreate.depthBiasClamp = 0.0f;
    m->pipeInfo->rsCreate.depthBiasSlopeFactor = 0.0f;

    // Vertex Input State
    m->pipeInfo->vboCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m->pipeInfo->vboCreate.vertexBindingDescriptionCount = m->attributes->bindings.size();
    m->pipeInfo->vboCreate.vertexAttributeDescriptionCount = m->attributes->attributes.size();
    m->pipeInfo->vboCreate.pVertexBindingDescriptions = m->attributes->bindings.data();
    m->pipeInfo->vboCreate.pVertexAttributeDescriptions = m->attributes->attributes.data();
    m->pipeInfo->vboCreate.flags = 0;
    m->pipeInfo->vboCreate.pNext = nullptr;

    // Push Constants
    bool hasPushConstants = info->pushConstantsSize >= 0;
    if (hasPushConstants) {
        m->pipeInfo->pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        m->pipeInfo->pushConstants.offset = 0;
        m->pipeInfo->pushConstants.size = info->pushConstantsSize;
    }

    // Dscription Set Layout
    memset(&this->p_pipeInfo.pipeLayInfo, 0, sizeof(this->p_pipeInfo.pipeLayInfo));
    this->p_pipeInfo.pipeLayInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    this->p_pipeInfo.pipeLayInfo.setLayoutCount = 1;
    this->p_pipeInfo.pipeLayInfo.pSetLayouts = &p_descSetLayout;
    this->p_pipeInfo.pipeLayInfo.pushConstantRangeCount = hasPushConstants ? 1 : 0;
    this->p_pipeInfo.pipeLayInfo.pPushConstantRanges = hasPushConstants ? &m->pipeInfo->pushConstants : nullptr;

    if (this->p_vdf->vkCreatePipelineLayout(this->p_dev, &this->p_pipeInfo.pipeLayInfo, nullptr, &this->p_pipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void ProgramVK::pipelineGlobalSetup() {
    // Viewport and Scissor
    memset(&this->p_pipeInfo.vp, 0, sizeof(this->p_pipeInfo.vp));
    this->p_pipeInfo.vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    this->p_pipeInfo.vp.viewportCount = 1;
    this->p_pipeInfo.vp.scissorCount = 1;

    // Tessellation
    memset(&this->p_pipeInfo.ts, 0, sizeof(this->p_pipeInfo.ts));
    this->p_pipeInfo.ts.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    this->p_pipeInfo.ts.patchControlPoints = 0;
    this->p_pipeInfo.ts.flags = 0;

    // Dynamic State (in place of above viewport and scissor)
    memset(&this->p_pipeInfo.dyn, 0, sizeof(this->p_pipeInfo.dyn));
    this->p_pipeInfo.dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    this->p_pipeInfo.dyn.dynamicStateCount = sizeof(this->p_pipeInfo.dynStates) / sizeof(VkDynamicState);
    this->p_pipeInfo.dyn.pDynamicStates = this->p_pipeInfo.dynStates;

    // Multisampling
    memset(&this->p_pipeInfo.ms, 0, sizeof(this->p_pipeInfo.ms));
    this->p_pipeInfo.ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    this->p_pipeInfo.ms.rasterizationSamples = this->p_vkw->sampleCountFlagBits();
    this->p_pipeInfo.ms.sampleShadingEnable = VK_TRUE;
    this->p_pipeInfo.ms.minSampleShading = 0.3f;
    this->p_pipeInfo.ms.pSampleMask = nullptr;
    this->p_pipeInfo.ms.alphaToCoverageEnable = VK_FALSE;
    this->p_pipeInfo.ms.alphaToOneEnable = VK_FALSE;

    // Depth Stencil
    memset(&this->p_pipeInfo.ds, 0, sizeof(this->p_pipeInfo.ds));
    this->p_pipeInfo.ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    this->p_pipeInfo.ds.depthTestEnable = VK_TRUE;
    this->p_pipeInfo.ds.depthWriteEnable = VK_TRUE;
    this->p_pipeInfo.ds.depthCompareOp = VK_COMPARE_OP_LESS;
    this->p_pipeInfo.ds.depthBoundsTestEnable = VK_FALSE;
    this->p_pipeInfo.ds.minDepthBounds = 0.0f;
    this->p_pipeInfo.ds.maxDepthBounds = 1.0f;
    this->p_pipeInfo.ds.stencilTestEnable = VK_FALSE;
    this->p_pipeInfo.ds.front = {};
    this->p_pipeInfo.ds.back = {};
    this->p_pipeInfo.ds.flags = 0;

    // Color Blending
    memset(&this->p_pipeInfo.cb, 0, sizeof(this->p_pipeInfo.cb));
    memset(&this->p_pipeInfo.cbAtt, 0, sizeof(this->p_pipeInfo.cbAtt));
    this->p_pipeInfo.cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    this->p_pipeInfo.cbAtt.blendEnable = VK_TRUE;                                           // VK_TRUE                                  VK_FALSE
    this->p_pipeInfo.cbAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;                 // VK_BLEND_FACTOR_SRC_ALPHA                VK_BLEND_FACTOR_ONE
    this->p_pipeInfo.cbAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;       // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      VK_BLEND_FACTOR_ZERO
    this->p_pipeInfo.cbAtt.colorBlendOp = VK_BLEND_OP_ADD;                                  // VK_BLEND_OP_ADD                          VK_BLEND_OP_ADD
    this->p_pipeInfo.cbAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                       // VK_BLEND_FACTOR_ONE                      VK_BLEND_FACTOR_ONE
    this->p_pipeInfo.cbAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;                      // VK_BLEND_FACTOR_ZERO                     VK_BLEND_FACTOR_ZERO
    this->p_pipeInfo.cbAtt.alphaBlendOp = VK_BLEND_OP_ADD;                                  // VK_BLEND_OP_ADD                          VK_BLEND_OP_ADD

    this->p_pipeInfo.cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    this->p_pipeInfo.cb.attachmentCount = 1;
    this->p_pipeInfo.cb.pAttachments = &this->p_pipeInfo.cbAtt;
    this->p_pipeInfo.cb.logicOpEnable = VK_FALSE;                       // VK_TRUE
    this->p_pipeInfo.cb.logicOp = VK_LOGIC_OP_COPY;
    this->p_pipeInfo.cb.blendConstants[0] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[1] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[2] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[3] = 0.0f;

    this->p_pipeInfo.init = true;
}

void ProgramVK::createPipeline(RenderInfo *render, ModelInfo *m, int vs, int fs, int ia) {
    std::vector<VkPipelineShaderStageCreateInfo> shaderModules = { m->pipeInfo->vsCreates[vs], m->pipeInfo->fsCreates[fs] };

    // Global pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderModules.data();
    pipelineInfo.pViewportState = &this->p_pipeInfo.vp;
    pipelineInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipelineInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipelineInfo.pDepthStencilState = &this->p_pipeInfo.ds;
    pipelineInfo.pColorBlendState = &this->p_pipeInfo.cb;
    pipelineInfo.layout = this->p_pipeLayout;
    pipelineInfo.renderPass = this->p_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // Model-specific pipeline
    pipelineInfo.pVertexInputState = &m->pipeInfo->vboCreate;
    pipelineInfo.pInputAssemblyState = &m->pipeInfo->iaCreates[ia];
    pipelineInfo.pRasterizationState = &m->pipeInfo->rsCreate;

    if ((this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, p_pipeCache, 1, &pipelineInfo, nullptr, &render->pipeline)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void ProgramVK::genVertexInputPipeLib(ModelInfo *model, VkPipelineVertexInputStateCreateInfo &vbo, VkPipelineInputAssemblyStateCreateInfo &ia) {
    // Declare model-specific pipeline library part: Vertex Input State
    VkGraphicsPipelineLibraryCreateInfoEXT pipeLibVBOInfo{};
    pipeLibVBOInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
    pipeLibVBOInfo.pNext = nullptr;
    pipeLibVBOInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT;

    // Create model-specific pipeline library part: Vertex Input State
    VkGraphicsPipelineCreateInfo pipeCreateLibVBOInfo{};
    pipeCreateLibVBOInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCreateLibVBOInfo.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
    pipeCreateLibVBOInfo.pNext = &pipeLibVBOInfo;
    pipeCreateLibVBOInfo.pVertexInputState = &vbo;
    pipeCreateLibVBOInfo.pInputAssemblyState = &ia;
    pipeCreateLibVBOInfo.pDynamicState = &this->p_pipeInfo.dyn;

    VkPipeline pipe = VK_NULL_HANDLE;
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibVBOInfo, nullptr, &pipe);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vertex Input pipeline library!");
    }
    model->pipeInfo->pipeLib->vertexInput.push_back(pipe);
    // TODO : Make this use direct assignment to the vector
}

void ProgramVK::genPreRasterizationPipeLib(ModelInfo *model, VkPipelineShaderStageCreateInfo &vert, VkPipelineRasterizationStateCreateInfo &rs) {
    // Declare model-specific pipeline library part: Pre-Rasterization State
    VkGraphicsPipelineLibraryCreateInfoEXT pipeLibPRSInfo{};
    pipeLibPRSInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
    pipeLibPRSInfo.pNext = nullptr;
    pipeLibPRSInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT;

    // Create model-specific pipeline library part: Pre-Rasterization State
    VkGraphicsPipelineCreateInfo pipeCreateLibPRSInfo{};
    pipeCreateLibPRSInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCreateLibPRSInfo.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
    pipeCreateLibPRSInfo.pNext = &pipeLibPRSInfo;
    pipeCreateLibPRSInfo.stageCount = 1;
    pipeCreateLibPRSInfo.pStages = &vert;
    pipeCreateLibPRSInfo.pRasterizationState = &rs;
    pipeCreateLibPRSInfo.pViewportState = &this->p_pipeInfo.vp;
    pipeCreateLibPRSInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipeCreateLibPRSInfo.layout = this->p_pipeLayout;
    pipeCreateLibPRSInfo.renderPass = this->p_renderPass;
    pipeCreateLibPRSInfo.subpass = 0;

    VkPipeline pipe = VK_NULL_HANDLE;
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibPRSInfo, nullptr, &pipe);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Pre-Rasterization pipeline library!");
    }
    model->pipeInfo->pipeLib->preRasterization.push_back(pipe);
}

void ProgramVK::genFragmentShaderPipeLib(ModelInfo *model, VkPipelineShaderStageCreateInfo &frag) {
    // Create global pipeline library part: Fragment Shader State
    VkGraphicsPipelineLibraryCreateInfoEXT pipeLibFSInfo{};
    pipeLibFSInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
    pipeLibFSInfo.pNext = nullptr;
    pipeLibFSInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT;

    VkGraphicsPipelineCreateInfo pipeCreateLibFSInfo {};
    pipeCreateLibFSInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCreateLibFSInfo.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
    pipeCreateLibFSInfo.pNext = &pipeLibFSInfo;
    pipeCreateLibFSInfo.stageCount = 1;
    pipeCreateLibFSInfo.pStages = &frag;
    pipeCreateLibFSInfo.pDepthStencilState = &this->p_pipeInfo.ds;
    pipeCreateLibFSInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipeCreateLibFSInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipeCreateLibFSInfo.renderPass = this->p_renderPass;
    pipeCreateLibFSInfo.subpass = 0;

    VkPipeline pipe = VK_NULL_HANDLE;
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibFSInfo, nullptr, &pipe);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Fragment Shader pipeline library!");
    }
    model->pipeInfo->pipeLib->fragmentShader.push_back(pipe);
}

void ProgramVK::genFragmentOutputPipeLib() {
    // Create global pipeline library part: Fragment Output State
    VkGraphicsPipelineLibraryCreateInfoEXT pipeLibFOInfo{};
    pipeLibFOInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
    pipeLibFOInfo.pNext = nullptr;
    pipeLibFOInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT;
    
    VkGraphicsPipelineCreateInfo pipeCreateLibFOInfo{};
    pipeCreateLibFOInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCreateLibFOInfo.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
    pipeCreateLibFOInfo.pNext = &pipeLibFOInfo;
    pipeCreateLibFOInfo.pColorBlendState = &this->p_pipeInfo.cb;
    pipeCreateLibFOInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipeCreateLibFOInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipeCreateLibFOInfo.renderPass = this->p_renderPass;
    pipeCreateLibFOInfo.subpass = 0;

    VkPipeline pipe = VK_NULL_HANDLE;
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibFOInfo, nullptr, &pipe);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Fragment Output pipeline library!");
    }
    this->p_fragmentOutput = pipe;
}

void ProgramVK::createPipeFromLibraries(RenderInfo *render, VkPipeline &vis, VkPipeline &pre, VkPipeline &frag) {
    std::vector<VkPipeline> libs = { vis, pre, frag, this->p_fragmentOutput };

    VkPipelineLibraryCreateInfoKHR pipeLibLinkInfo{};
    pipeLibLinkInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
    pipeLibLinkInfo.pNext = nullptr;
    pipeLibLinkInfo.libraryCount = static_cast<uint32_t>(libs.size());
    pipeLibLinkInfo.pLibraries = libs.data();

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.pNext = &pipeLibLinkInfo;
    pipeInfo.flags = VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT;

    VkPipeline pipe = VK_NULL_HANDLE;
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeInfo, nullptr, &pipe);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    render->pipeline = pipe;
}

void ProgramVK::updatePipeFromLibraries() {
    // Update pipe with new pipeLibs
}

void ProgramVK::createCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(this->p_phydev);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if ((this->p_vdf->vkCreateCommandPool(this->p_dev, &poolInfo, nullptr, &this->p_cmdpool)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void ProgramVK::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->p_cmdpool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if ((this->p_vdf->vkAllocateCommandBuffers(this->p_dev, &allocInfo, &this->p_cmdbuff)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

uint32_t ProgramVK::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flagProperties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    this->p_vf->vkGetPhysicalDeviceMemoryProperties(this->p_phydev, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flagProperties) == flagProperties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

void ProgramVK::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (this->p_vdf->vkCreateBuffer(this->p_dev, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    this->p_vdf->vkGetBufferMemoryRequirements(this->p_dev, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    err = this->p_vdf->vkAllocateMemory(this->p_dev, &allocInfo, nullptr, &bufferMemory);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory: " + std::to_string(err));
    }
    
    this->p_vdf->vkBindBufferMemory(this->p_dev, buffer, bufferMemory, 0);
}

void ProgramVK::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->p_cmdpool;
    allocInfo.commandBufferCount = 1;

    this->p_vdf->vkAllocateCommandBuffers(this->p_dev, &allocInfo, &this->p_cmdbuff);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    this->p_vdf->vkBeginCommandBuffer(this->p_cmdbuff, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    this->p_vdf->vkCmdCopyBuffer(this->p_cmdbuff, src, dst, 1, &copyRegion);

    this->p_vdf->vkEndCommandBuffer(this->p_cmdbuff);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->p_cmdbuff;
    
    this->p_vdf->vkQueueSubmit(this->p_queue, 1, &submitInfo, VK_NULL_HANDLE);
    this->p_vdf->vkQueueWaitIdle(this->p_queue);

    this->p_vdf->vkFreeCommandBuffers(this->p_dev, this->p_cmdpool, 1, &this->p_cmdbuff);
}

AttribInfo* ProgramVK::defineModelAttributes(ModelCreateInfo *info) {
    AttribInfo *attribInfo = new AttribInfo{};
    int bindings = 0;
    int locations = 0;

    // Populate binding and attribute info
    for (auto &vbo : info->vbos) {
        uint thisOffset = 0;
        std::vector<uint> offsets;
        offsets.push_back(0);

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = bindings;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Calculate atribute locations and sizes
        for (auto &attrib : vbo->dataTypes) {
            VkFormat thisFormat = dataFormats[static_cast<uint>(attrib)];

            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.binding = bindings;
            attributeDescription.format = thisFormat;
            attributeDescription.location = locations;
            attributeDescription.offset = thisOffset;
            attribInfo->attributes.push_back(attributeDescription);

            locations += ((thisFormat == VK_FORMAT_R64G64B64_SFLOAT) || (thisFormat == VK_FORMAT_R64G64B64A64_SFLOAT)) ? 2 : 1;
            thisOffset = dataSizes[static_cast<uint>(attrib)];
            offsets.push_back(thisOffset);
        }
        bindingDescription.stride = std::accumulate(offsets.cbegin(), offsets.cend(), 0);
        attribInfo->bindings.push_back(bindingDescription);
        bindings++;
    }

    return attribInfo;
}

void ProgramVK::stageAndCopyVertexBuffer(ModelInfo *model, VKuint idx, bool update, VKuint64 bufSize, const void *bufData) {
    VkBuffer *vbo = &model->vbos[idx];
    VkDeviceMemory *vboMem = &model->vboMemory[idx];

    createBuffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_stagingBuffer, p_stagingMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, p_stagingMemory, 0, bufSize, 0, &data);
    memcpy(data, bufData, bufSize);
    this->p_vdf->vkUnmapMemory(this->p_dev, p_stagingMemory);   

    if (!update) {
        createBuffer(bufSize, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *vbo, *vboMem);
    }
    
    copyBuffer(*vbo, p_stagingBuffer, bufSize);

    this->p_vdf->vkDestroyBuffer(this->p_dev, p_stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_stagingMemory, nullptr);
}

void ProgramVK::stageAndCopyIndexBuffer(ModelInfo *model, bool update, VKuint64 bufSize, const void *bufData) {
    VkBuffer *ibo = &model->ibo;
    VkDeviceMemory *iboMem = &model->iboMemory;

    createBuffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_stagingBuffer, p_stagingMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, p_stagingMemory, 0, bufSize, 0, &data);
    memcpy(data, bufData, bufSize);
    this->p_vdf->vkUnmapMemory(this->p_dev, p_stagingMemory);

    if (!update) {
        createBuffer(bufSize, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *ibo, *iboMem);
    }

    copyBuffer(*ibo, p_stagingBuffer, bufSize);

    this->p_vdf->vkDestroyBuffer(this->p_dev, p_stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_stagingMemory, nullptr);
}

void ProgramVK::createPersistentUniformBuffer(VkDeviceSize bufferSize) {
    p_descSets.resize(MAX_FRAMES_IN_FLIGHT);
    p_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    p_uniformMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMappings.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_uniformBuffers[i], p_uniformMemory[i]);
        this->p_vdf->vkMapMemory(this->p_dev, p_uniformMemory[i], 0, bufferSize, 0, &uniformMappings[i]);
    }

    defineDescriptorSets();
}

void ProgramVK::defineDescriptorSets() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
}

void ProgramVK::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, this->p_descSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->p_descPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (this->p_vdf->vkAllocateDescriptorSets(this->p_dev, &allocInfo, this->p_descSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = this->p_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = this->p_descSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        this->p_vdf->vkUpdateDescriptorSets(this->p_dev, 1, &descriptorWrite, 0, nullptr);
    }
}

void ProgramVK::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.flags = 0;

    if (this->p_vdf->vkCreateDescriptorPool(this->p_dev, &poolInfo, nullptr, &this->p_descPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void ProgramVK::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (this->p_vdf->vkCreateDescriptorSetLayout(this->p_dev, &layoutInfo, nullptr, &this->p_descSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void ProgramVK::updateRender(const std::string &modelName, OffsetInfo &off) {
    ModelInfo *model = getModelFromName(modelName);

    // Recreate pipeline to use new shader
    for (auto &render : model->renders) {
        if (render->iboOffset == off.offset) {
            this->createPipeFromLibraries(render,
                                          model->pipeInfo->pipeLib->vertexInput[off.topologyIndex],
                                          model->pipeInfo->pipeLib->preRasterization[off.vertShaderIndex],
                                          model->pipeInfo->pipeLib->fragmentShader[off.fragShaderIndex]);
        }
    }
}

void ProgramVK::updateBuffer(BufferUpdateInfo &buf) {
    VKuint id = -1;
    
    // Find model
    ModelInfo *model = getModelFromName(buf.modelName);

    // Find buffer and update with new data
    if (buf.type == BufferType::VERTEX) {
        try {
            id = p_mapBuffers.at(buf.bufferName);
        } catch (const std::out_of_range &e) {
            std::cout << "Buffer not found: " << buf.bufferName << std::endl;
            return;
        }
        this->stageAndCopyVertexBuffer(model, id, true, buf.size, buf.data);
    } else if (buf.type == BufferType::INDEX) {
        this->stageAndCopyIndexBuffer(model, true, buf.size, buf.data);
    }
}

void ProgramVK::updateUniformBuffer(uint32_t currentImage, uint32_t uboSize, const void *uboData) {
    void *dest = this->uniformMappings[currentImage];
    memcpy(dest, uboData, uboSize);
}

void ProgramVK::updateClearColor(float r, float g, float b, float a) {
    p_clearColor[0] = r;
    p_clearColor[1] = g;
    p_clearColor[2] = b;
    p_clearColor[3] = a;
}

void ProgramVK::updateSwapExtent(int x, int y) {
    this->p_swapExtent.width = x;
    this->p_swapExtent.height = y;
}

void ProgramVK::render(VkCommandBuffer &cmdBuff, VkExtent2D &renderExtent) {
    VKuint image = this->p_vkw->currentSwapChainImageIndex();

    // Set clear color and depth stencil
    /* std::array<VkClearValue, 3> clearValues{};
    for (auto &clear : clearValues) {
        clear.color = {{ p_clearColor[0], p_clearColor[1], p_clearColor[2], p_clearColor[3] }};
        clear.depthStencil = {1.0f, 0};
        clear.color = {{ p_clearColor[0], p_clearColor[1], p_clearColor[2], p_clearColor[3] }};
    } */
    VkClearColorValue clearColor = { p_clearColor[0], p_clearColor[1], p_clearColor[2], p_clearColor[3] };
    VkClearDepthStencilValue clearDepth = { 1.0f, 0 };
    VkClearValue clearValues[3];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDepth;

    // Set viewport and scissor
    p_viewport.x = 0.0f;
    p_viewport.y = 0.0f;
    p_viewport.width = static_cast<float>(renderExtent.width);
    p_viewport.height = static_cast<float>(renderExtent.height);
    p_viewport.minDepth = 0.0f;
    p_viewport.maxDepth = 1.0f;
    p_scissor.offset = {0, 0};
    p_scissor.extent = renderExtent;

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->p_vkw->defaultRenderPass();
    renderPassInfo.framebuffer = this->p_vkw->currentFramebuffer();
    renderPassInfo.renderArea.extent = renderExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = this->p_vkw->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    renderPassInfo.pClearValues = clearValues;
    
    this->p_vdf->vkCmdBeginRenderPass(cmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // For each active model, bind and draw for all render targets
    for (auto &actMod : this->p_activeModels) {
        ModelInfo *model = this->p_models[actMod];

        for (auto &render : model->renders) {
            this->p_vdf->vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, render->pipeline);
            this->p_vdf->vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, this->p_pipeLayout, 0, 1, &p_descSets[image], 0, nullptr);
            this->p_vdf->vkCmdSetViewport(cmdBuff, 0, 1, &this->p_viewport);
            this->p_vdf->vkCmdSetScissor(cmdBuff, 0, 1, &this->p_scissor);
            this->p_vdf->vkCmdBindVertexBuffers(cmdBuff, 0, model->vbos.size(), render->firstVbo, render->vboOffsets.data());
            this->p_vdf->vkCmdBindIndexBuffer(cmdBuff, *render->ibo, 0, VK_INDEX_TYPE_UINT32);
            if (render->pushConst.first != 0) {
                this->p_vdf->vkCmdPushConstants(cmdBuff, this->p_pipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, render->pushConst.first, render->pushConst.second);
            }
            this->p_vdf->vkCmdDrawIndexed(cmdBuff, render->indexCount, 1, render->iboOffset, 0, 0);
        }
    }
    
    // Cleanup
    this->p_vdf->vkCmdEndRenderPass(cmdBuff);
}

Shader* ProgramVK::getShaderFromName(const std::string& fileName) {
    assert(stage >= 2);
    Shader *s = nullptr;
    
    for (const auto& shader : p_compiledShaders) {
        if (shader->getName() == fileName) {
            s = shader;
        }
    }

    if (s == nullptr) {
        throw std::runtime_error("Shader " + fileName + " not found.");
    }
    
    return s;
}

ModelInfo* ProgramVK::getModelFromName(const std::string& name) {
    ModelInfo *m = nullptr;
    uint id;

    try {
        id = p_mapModels.at(name);
    } catch (const std::out_of_range &e) {
        std::cout << "Model not found: " << name << std::endl;
        return nullptr;
    }
    
    return p_models[id];
}

VKuint ProgramVK::getModelIdFromName(const std::string& name) {
    VKuint id = -1;
    
    if (p_mapModels.find(name) == p_mapModels.end()) {
        std::cout << "Model not found: " << name << std::endl;
        return id;
    }
    
    // None of these are catching, fml.
    try {
        id = p_mapModels.at(name);
    } catch (const std::out_of_range &e) {
        std::cout << "Model not found: " << name << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception" << std::endl;
    }

    return id;
}

std::vector<VKuint> ProgramVK::getActiveModelsById() {
    return p_activeModels;
}

std::vector<std::string> ProgramVK::getActiveModelsByName() {
    std::vector<std::string> names;

    for (VKuint id : p_activeModels) {
        names.push_back(p_models[id]->name);
    }

    return names;
}

bool ProgramVK::isActive(const std::string &modelName) {
    bool active = false;
    VKuint id = getModelIdFromName(modelName);

    bool found = id >= 0;
    if (found) {
        active = (p_activeModels.end() != std::find(p_activeModels.cbegin(), p_activeModels.cend(), id));
    }

    return active;
}

void ProgramVK::printModel(ModelInfo *model) {
    std::cout << "Model: " << model->id << "\n";
    
    if (!model->attributes) {
        std::cout << "    No attributes." << "\n";
    } else {
        for (int i = 0; i < model->attributes->bindings.size(); i++) {
            std::cout << "    Binding " << i << ": " << model->attributes->bindings[i].binding << "\n";

            for (int j = 0; j < model->attributes->attributes.size(); j++) {
                if (model->attributes->attributes[j].binding == i) {
                    std::cout << "        Attribute " << j << ": " << model->attributes->attributes[j].location << "\n";
                    std::cout << "            Offset: " << model->attributes->attributes[j].offset << "\n";
                    std::cout << "            Format: " << dataFormatNames[dataFormatIdx[model->attributes->attributes[j].format]] << "\n";
                    std::cout << "            Size  : " << dataSizes[dataFormatIdx[model->attributes->attributes[j].format]] << "\n";
                }
            }
        }
    }
    
    if (!model->shaders) {
        std::cout << "    No shaders." << "\n";
    } else {
        for (int i = 0; i < model->shaders->vertModules.size(); i++) {
            std::cout << "    Vertex Shader   " << i << ": " << model->shaders->vertShaders[i]->getName() << "\n";
        }
        for (int i = 0; i < model->shaders->fragModules.size(); i++) {
            std::cout << "    Fragment Shader " << i << ": " << model->shaders->fragShaders[i]->getName() << "\n";
        }
    }

    if (!model->renders.size()) {
        std::cout << "    No renders." << "\n";
    } else {
        for (int i = 0; i < model->renders.size(); i++) {
            std::cout << "    Render " << i << ": " << "\n";
            std::cout << "        IBO Offset : " << model->renders[i]->iboOffset << "\n";
            std::cout << "        Index Count: " << model->renders[i]->indexCount << "\n";
        }
    }

    std::cout << std::endl;
}

void ProgramVK::printInfo(ModelCreateInfo *info) {
    std::cout << "Model: " << info->name << "\n";
    
    for (auto &vbo : info->vbos) {
        std::cout << "    VBO: " << vbo->name << "\n";
        std::cout << "        Type        : " << bufferTypeNames[uint(vbo->type)] << "\n";
        std::cout << "        Vertex Count: " << vbo->count << "\n";
        std::cout << "        Vertex Size : " << vbo->size << "\n";
        std::cout << "        Data Types  : " << "\n";
        for (auto &dt : vbo->dataTypes) {
            std::cout << "            " << dataTypeNames[uint(dt)] << "\n";
        }
    }
    
    auto &ibo = info->ibo;
    std::cout << "    IBO: " << ibo->name << "\n";
    std::cout << "        Type       : " << bufferTypeNames[uint(ibo->type)] << "\n";
    std::cout << "        Index Count: " << ibo->count << "\n";
    std::cout << "        Index Size : " << ibo->size << "\n";
    std::cout << "        Data Types : " << "\n";
    for (auto &dt : ibo->dataTypes) {
        std::cout << "            " << dataTypeNames[uint(dt)] << "\n";
    }

    std::cout << "    UBO Size: " << info->uboSize << "\n";

    std::cout << "    Shaders: " <<  "\n";
    for (auto &s : info->vertShaders) {
        std::cout << "        " << s << "\n";
    }
    for (auto &s : info->fragShaders) {
        std::cout << "        " << s << "\n";
    }

    std::cout << "    Offsets: " << "\n";
    for (int i = 0; i < info->offsets.size(); i++) {
        auto &offset = info->offsets[i];
        std::cout << "        [" << i << "]: " << offset.offset << "\n";
        std::cout << "            Vertex Shader  : " << info->vertShaders[offset.vertShaderIndex] << "\n";
        std::cout << "            Fragment Shader: " << info->fragShaders[offset.fragShaderIndex] << "\n";
        std::cout << "            Topology       : " << topologyNames[info->topologies[offset.topologyIndex]] << "\n";
    }

    std::cout << std::endl;
}
