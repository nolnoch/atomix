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
    delete samplers;

    // destruct buffers

    cleanup();
}

void ProgramVK::cleanup() {
    this->p_vdf->vkDestroyBuffer(this->p_dev, p_indexBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_indexMemory, nullptr);

    for (int i = 0; i < p_vertexBuffers.size(); i++) {
        this->p_vdf->vkDestroyBuffer(this->p_dev, p_vertexBuffers[i], nullptr);
        this->p_vdf->vkFreeMemory(this->p_dev, p_vertexMemory[i], nullptr);
    }

    for (int i = 0; i < p_uniformBuffers.size(); i++) {
        this->p_vdf->vkDestroyBuffer(this->p_dev, p_uniformBuffers[i], nullptr);
        this->p_vdf->vkFreeMemory(this->p_dev, p_uniformMemory[i], nullptr);
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
bool ProgramVK::addShader(std::string fName, VKuint type) {
    bool validFile = false;
    std::string fileLoc;
    
    if (fName.find('/') == std::string::npos) {
        fileLoc = std::string(ROOT_DIR) + std::string(SHADERS) + fName;
    } else {
        fileLoc = fName;
    }

    Shader *shader = new Shader(fileLoc, type);

    for (auto &s : this->registeredShaders) {
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
        this->registeredShaders.push_back(shader);
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
    if (!shader->compile()) {
        std::cout << "Failed to compile shader. Deleting Shader." << std::endl;
        auto it = std::find(this->registeredShaders.begin(), this->registeredShaders.end(), shader);
        this->registeredShaders.erase(it);
        delete shader;
        return false;
    }

    this->compiledShaders.push_back(shader);

    return true;
}

/**
 * Compile all shaders that have been added with addShader() or
 * addAllShaders().  This function will return the number of errors
 * encountered during compilation.
 *
 * @return number of errors, or 0 if all shaders compiled successfully
 */
int ProgramVK::compileAllShaders() {
    int errors = this->registeredShaders.size();
    
    for (auto &shader : this->registeredShaders) {
        if (this->compileShader(shader)) {
            errors--;
        }
    }

    return errors;
}

void ProgramVK::bindShader(std::string name) {
    Shader *activeShader;
    for (auto &shader : this->compiledShaders) {
        if (shader->getName() == name) {
            activeShader = shader;
            break;
        }
    }

    if (!activeShader) {
        std::cout << "Invalid shader name. Aborting..." << std::endl;
        return;
    }

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = activeShader->lengthCompiled();
    moduleCreateInfo.pCode = activeShader->getSourceCompiled();

    VkShaderModule shaderModule;
    err = p_vdf->vkCreateShaderModule(p_dev, &moduleCreateInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module: " + std::to_string(err));
    }

    if (activeShader->getType() == GL_VERTEX_SHADER) {
        this->p_shaderVert = shaderModule;
    } else if (activeShader->getType() == GL_FRAGMENT_SHADER) {
        this->p_shaderFrag = shaderModule;
    } else if (activeShader->getType() == GL_GEOMETRY_SHADER) {
        this->p_shaderGeom = shaderModule;
    } else if (activeShader->getType() == GL_COMPUTE_SHADER) {
        this->p_shaderComp = shaderModule;
    }
}

/**
 * Generates a sampler uniform bind target for use in the GLSL shader code.
 */
void ProgramVK::addSampler(std::string sName) {
    SamplerInfo info;
    VKuint sample;

    if (!samplers)
        this->samplers = new std::vector<SamplerInfo>();

    // qgf->glGenSamplers(1, &sample);

    info.samplerID = sample;
    info.samplerName = sName;

    this->samplers->push_back(info);
}

BufferInfo *ProgramVK::addBuffer(BufferCreateInfo &info) {
    VKuint idx = p_buffers.size();
    BufferInfo *buf;

    const auto [it, success] = p_mapBuf.insert({info.name, idx});

    if (success) {
        buf = new BufferInfo{};
        buf->id = idx;
        buf->type = info.type;
        buf->binding = info.binding;
        buf->size = info.size;
        if (info.storeData) {
            buf->data = operator new(buf->size);
            memcpy(buf->data, info.data, buf->size);
            p_allocatedBuffers.push_back(buf->data);
        } else {
            buf->data = info.data;
        }
        buf->dataTypes = info.dataTypes;
        p_buffers.push_back(buf);
        return buf;
    } else {
        std::cout << "Buffer already exists. Returning existing buffer." << std::endl;
        return this->p_buffers[it->second];
    }

    return nullptr;
}

void ProgramVK::addModel(ModelCreateInfo &info) {
    ModelInfo *model;
    uint idx = p_models.size();

    const auto [it, success] = p_mapModel.insert({info.name, idx});

    if (!success) {
        std::cout << "Model already exists. Updating model " << info.name << "..." << std::endl;
        model = p_models[it->second];
    } else {
        model = new ModelInfo{0, std::vector<BufferInfo *>(info.vboCount), nullptr, nullptr, nullptr, nullptr};
        model->id = idx;
    }

    for (int i = 0; i < info.vboCount; i++) {
        model->vbos[i] = addBuffer(info.vbos[i]);
    }
    model->ibo = addBuffer(*info.ibo);
    model->ubo = addBuffer(*info.ubo);
    
    model->shaders = new ShaderInfo;
    model->shaders->vertShader = getShaderFromName(info.vertShader);
    model->shaders->fragShader = getShaderFromName(info.fragShader);

    model->pipe = new ModelPipelineInfo;

    if (success) {
        p_models.push_back(model);
    }
}

/**
 * Initializes the program. Then initializes, loads, and compiles all shaders
 * associated with the ProgramVK object.
 */
bool ProgramVK::init() {
    int numShaders = this->registeredShaders.size();

    if (!numShaders || !stage) {
        std::cout << "No shader files associated with program. Aborting..." << std::endl;
        return false;
    }

    // Process registered shaders
    compileAllShaders();

    // Init program

    createCommandPool();
    createCommandBuffers();
    createSwapChain();
    createRenderPass();
    assemblePipeline();
    createPipeline();

    this->stage = 2;
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

SwapChainSupportInfo ProgramVK::querySwapChainSupport(VkPhysicalDevice device) {
/*     SwapChainSupportInfo info;
    
    this->p_vkw->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->p_vkw->surface, &info.capabilities);
    
    VKuint formatCount = 0;
    this->p_vkw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->p_vkw->surface, &formatCount, nullptr);
    if (formatCount) {
        info.formats.resize(formatCount);
        this->p_vkw->vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->p_vkw->surface, &formatCount, info.formats.data());
    }

    VKuint presentModeCount = 0;
    this->p_vkw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->p_vkw->surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        info.presentModes.resize(presentModeCount);
        this->p_vkw->vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->p_vkw->surface, &presentModeCount, info.presentModes.data());
    }
    
    return info; */
}

void ProgramVK::createSwapChain() {
/*     // VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(this->p_vkw->availableFormats);
    // VkPresentModeKHR presentMode = chooseSwapPresentMode(this->p_vkw->availablePresentModes);
    // VkExtent2D extent = chooseSwapExtent(this->p_vkw->availableResolutions);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->p_vkw->surface;
    createInfo.minImageCount = this->p_vkw->minImageCount;
    createInfo.imageFormat = this->p_vkw->availableFormats[0].format;
    createInfo.imageColorSpace = this->p_vkw->availableFormats[0].colorSpace;
    createInfo.imageExtent = this->p_vkw->availableResolutions[0];
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(this->p_phydev);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = this->p_vkw->capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = this->p_vkw->presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain;
    if ((this->p_vkw->vkCreateSwapchainKHR(this->p_dev, &createInfo, nullptr, &swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // this->p_vkw->swapchain = swapchain; */
}

/* void ProgramVK::createImageViews() {
    
} */

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

void ProgramVK::createShaderStages(ModelInfo *m) {
    if (m->shaders->vertShader) {
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = m->shaders->vertShader->getLengthCompiled();
        moduleCreateInfo.pCode = m->shaders->vertShader->getSourceCompiled();
        
        if ((this->p_vdf->vkCreateShaderModule(this->p_dev, &moduleCreateInfo, nullptr, &m->shaders->vertModule)) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module: " + m->shaders->vertShader->getName());
        }

        m->pipe->vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m->pipe->vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        m->pipe->vertStage.module = m->shaders->vertModule;
        m->pipe->vertStage.pName = "main";
    }

    if (m->shaders->fragShader) {
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = m->shaders->fragShader->getLengthCompiled();
        moduleCreateInfo.pCode = m->shaders->fragShader->getSourceCompiled();
        
        if ((this->p_vdf->vkCreateShaderModule(this->p_dev, &moduleCreateInfo, nullptr, &m->shaders->fragModule)) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module: " + m->shaders->fragShader->getName());
        }

        m->pipe->fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m->pipe->fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        m->pipe->fragStage.module = m->shaders->fragModule;
        m->pipe->fragStage.pName = "main";
    }
}

void ProgramVK::pipelineModelSetup(ModelInfo *m) {
    VkPolygonMode polyMode;
    if (m->pipe->topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST) {
        polyMode = VK_POLYGON_MODE_POINT;
    } else if (m->pipe->topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST) {
        polyMode = VK_POLYGON_MODE_LINE;
    } else {
        polyMode = VK_POLYGON_MODE_FILL;
    }

    // Shader
    this->createShaderStages(m);

    // Vertex Input
    this->defineModelAttributes(m);
    this->lockModelAttributes(m);

    // Input Assembly
    memset(&m->pipe->ia, 0, sizeof(m->pipe->ia));
    m->pipe->ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m->pipe->ia.topology = m->pipe->topology;

    // Rasterization
    memset(&m->pipe->rs, 0, sizeof(m->pipe->rs));
    m->pipe->rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m->pipe->rs.polygonMode = polyMode;
    m->pipe->rs.cullMode = VK_CULL_MODE_NONE;
    m->pipe->rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m->pipe->rs.lineWidth = 1.0f;
    m->pipe->rs.depthClampEnable = VK_FALSE;
    m->pipe->rs.depthBiasEnable = VK_FALSE;
    m->pipe->rs.depthBiasConstantFactor = 0.0f;
    m->pipe->rs.depthBiasClamp = 0.0f;
    m->pipe->rs.depthBiasSlopeFactor = 0.0f;
}

void ProgramVK::assemblePipeline() {
    // Viewport and Scissor
    memset(&this->p_pipeInfo.vp, 0, sizeof(this->p_pipeInfo.vp));
    this->p_pipeInfo.vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    this->p_pipeInfo.vp.viewportCount = 1;
    this->p_pipeInfo.vp.scissorCount = 1;

    // Dynamic State (in place of above viewport and scissor)
    memset(&this->p_pipeInfo.dyn, 0, sizeof(this->p_pipeInfo.dyn));
    this->p_pipeInfo.dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    this->p_pipeInfo.dyn.dynamicStateCount = sizeof(this->p_pipeInfo.dynStates) / sizeof(VkDynamicState);
    this->p_pipeInfo.dyn.pDynamicStates = this->p_pipeInfo.dynStates;

    // Multisampling
    memset(&this->p_pipeInfo.ms, 0, sizeof(this->p_pipeInfo.ms));
    this->p_pipeInfo.ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    this->p_pipeInfo.ms.rasterizationSamples = this->p_vkw->sampleCountFlagBits();
    this->p_pipeInfo.ms.sampleShadingEnable = VK_FALSE;
    this->p_pipeInfo.ms.minSampleShading = 1.0f;
    this->p_pipeInfo.ms.pSampleMask = nullptr;
    this->p_pipeInfo.ms.alphaToCoverageEnable = VK_FALSE;
    this->p_pipeInfo.ms.alphaToOneEnable = VK_FALSE;

    // Depth Stencil
    memset(&this->p_pipeInfo.ds, 0, sizeof(this->p_pipeInfo.ds));
    this->p_pipeInfo.ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    this->p_pipeInfo.ds.depthTestEnable = VK_TRUE;
    this->p_pipeInfo.ds.depthWriteEnable = VK_TRUE;
    this->p_pipeInfo.ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    this->p_pipeInfo.ds.depthBoundsTestEnable = VK_FALSE;
    this->p_pipeInfo.ds.stencilTestEnable = VK_FALSE;
    this->p_pipeInfo.ds.front = {};
    this->p_pipeInfo.ds.back = {};
    this->p_pipeInfo.ds.minDepthBounds = 0.0f;
    this->p_pipeInfo.ds.maxDepthBounds = 1.0f;

    // Color Blending
    memset(&this->p_pipeInfo.cb, 0, sizeof(this->p_pipeInfo.cb));
    memset(&this->p_pipeInfo.cbAtt, 0, sizeof(this->p_pipeInfo.cbAtt));
    this->p_pipeInfo.cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    this->p_pipeInfo.cbAtt.blendEnable = VK_FALSE;                      // VK_TRUE
    this->p_pipeInfo.cbAtt.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // VK_BLEND_FACTOR_SRC_ALPHA
    this->p_pipeInfo.cbAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
    this->p_pipeInfo.cbAtt.colorBlendOp = VK_BLEND_OP_ADD;              // VK_BLEND_OP_ADD
    this->p_pipeInfo.cbAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // VK_BLEND_FACTOR_ONE
    this->p_pipeInfo.cbAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // VK_BLEND_FACTOR_ZERO
    this->p_pipeInfo.cbAtt.alphaBlendOp = VK_BLEND_OP_ADD;              // VK_BLEND_OP_ADD

    this->p_pipeInfo.cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    this->p_pipeInfo.cb.attachmentCount = 1;
    this->p_pipeInfo.cb.pAttachments = &this->p_pipeInfo.cbAtt;
    this->p_pipeInfo.cb.logicOpEnable = VK_FALSE;                       // VK_TRUE
    this->p_pipeInfo.cb.logicOp = VK_LOGIC_OP_COPY;
    this->p_pipeInfo.cb.blendConstants[0] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[1] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[2] = 0.0f;
    this->p_pipeInfo.cb.blendConstants[3] = 0.0f;

    // Layout
    memset(&this->p_pipeInfo.pipeLayInfo, 0, sizeof(this->p_pipeInfo.pipeLayInfo));
    this->p_pipeInfo.pipeLayInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    this->p_pipeInfo.pipeLayInfo.setLayoutCount = 0;
    this->p_pipeInfo.pipeLayInfo.pSetLayouts = nullptr;
    this->p_pipeInfo.pipeLayInfo.pushConstantRangeCount = 0;
    this->p_pipeInfo.pipeLayInfo.pPushConstantRanges = nullptr;

    if (this->p_vdf->vkCreatePipelineLayout(this->p_dev, &this->p_pipeInfo.pipeLayInfo, nullptr, &this->p_pipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void ProgramVK::createPipeline(ModelInfo *m) {
    // TODO redo this

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = this->p_pipeInfo.shaderStages.data();
    pipelineInfo.pVertexInputState = &this->p_pipeInfo.vboInfo;
    pipelineInfo.pInputAssemblyState = &this->p_pipeInfo.ia;
    pipelineInfo.pViewportState = &this->p_pipeInfo.vp;
    pipelineInfo.pRasterizationState = &m->pipe->rs;
    pipelineInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipelineInfo.pDepthStencilState = &this->p_pipeInfo.ds;
    pipelineInfo.pColorBlendState = &this->p_pipeInfo.cb;
    pipelineInfo.layout = this->p_pipeLayout;
    pipelineInfo.renderPass = this->p_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if ((this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->p_pipe)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
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

BufferInfo* ProgramVK::getBufferInfo(std::string name) {
    BufferInfo *buf;

    buf = p_buffers[p_mapBuf[name]];

    if (buf == nullptr) {
        throw std::runtime_error("Buffer not found!");
    }

    return buf;
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
        throw std::runtime_error("failed to create buffer!");
    }   

    VkMemoryRequirements memRequirements;
    this->p_vdf->vkGetBufferMemoryRequirements(this->p_dev, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (this->p_vdf->vkAllocateMemory(this->p_dev, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    
    this->p_vdf->vkBindBufferMemory(this->p_dev, buffer, bufferMemory, 0);
}

void ProgramVK::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->p_cmdpool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    this->p_vdf->vkAllocateCommandBuffers(this->p_dev, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    this->p_vdf->vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    this->p_vdf->vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    this->p_vdf->vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    this->p_vdf->vkQueueSubmit(this->p_queue, 1, &submitInfo, VK_NULL_HANDLE);
    this->p_vdf->vkQueueWaitIdle(this->p_queue);

    this->p_vdf->vkFreeCommandBuffers(this->p_dev, this->p_cmdpool, 1, &commandBuffer);
}

void ProgramVK::defineModelAttributes(ModelInfo *model) {
    AttribInfo *attribInfo = new AttribInfo{};
    int bindings = 0;
    int locations = 0;

    // Populate binding and attribute info
    for (auto &vbo : model->vbos) {
        uint thisOffset = 0;
        std::vector<uint> offsets;
        offsets.push_back(0);

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = bindings;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Calculate atribute locations and sizes
        for (auto &attrib : vbo->dataTypes) {
            VkFormat thisFormat = DataFormats[static_cast<uint>(attrib)];

            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.binding = bindings;
            attributeDescription.format = thisFormat;
            attributeDescription.location = locations;
            attributeDescription.offset = thisOffset;
            attribInfo->attributes.push_back(attributeDescription);

            offsets.push_back(thisOffset);
            locations += (thisFormat & (VK_FORMAT_R64G64B64_SFLOAT | VK_FORMAT_R64G64B64A64_SFLOAT)) ? 2 : 1;
            thisOffset = dataSizes[static_cast<uint>(attrib)];
        }
        bindingDescription.stride = std::accumulate(offsets.cbegin(), offsets.cend(), 0);
        attribInfo->bindings.push_back(bindingDescription);
        bindings++;
    }

    model->attributes = attribInfo;
}

void ProgramVK::lockModelAttributes(ModelInfo *model) {
    model->pipe->vboInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    model->pipe->vboInfo.vertexBindingDescriptionCount = model->attributes->bindings.size();
    model->pipe->vboInfo.vertexAttributeDescriptionCount = model->attributes->attributes.size();
    model->pipe->vboInfo.pVertexBindingDescriptions = model->attributes->bindings.data();
    model->pipe->vboInfo.pVertexAttributeDescriptions = model->attributes->attributes.data();
    model->pipe->vboInfo.flags = 0;
    model->pipe->vboInfo.pNext = nullptr;
}

void ProgramVK::createVertexBuffer(BufferInfo *buf) {
    uint idx = buf->binding;

    createBuffer(buf->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_stagingBuffer, p_stagingMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, p_stagingMemory, 0, buf->size, 0, &data);
    std::copy(buf->data, buf->data + buf->size, data);
    this->p_vdf->vkUnmapMemory(this->p_dev, p_stagingMemory);

    createBuffer(buf->size, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_vertexBuffers[idx], p_vertexMemory[idx]);
    
    copyBuffer(p_stagingBuffer, p_vertexBuffers[idx], buf->size);

    this->p_vdf->vkDestroyBuffer(this->p_dev, p_stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_stagingMemory, nullptr);
}

void ProgramVK::createIndexBuffer(BufferInfo *buf) {
    createBuffer(buf->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_stagingBuffer, p_stagingMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, p_stagingMemory, 0, buf->size, 0, &data);
    std::copy(buf->data, buf->data + buf->size, data);
    this->p_vdf->vkUnmapMemory(this->p_dev, p_stagingMemory);

    createBuffer(buf->size, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_indexBuffer, p_indexMemory);

    copyBuffer(p_stagingBuffer, p_indexBuffer, buf->size);

    this->p_vdf->vkDestroyBuffer(this->p_dev, p_stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_stagingMemory, nullptr);
}

void ProgramVK::createPersistentUniformBuffer(BufferInfo *buf) {
    VkDeviceSize bufferSize = buf->size;
    
    p_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    p_uniformMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMappings.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_uniformBuffers[i], p_uniformMemory[i]);
    }

    this->p_vdf->vkMapMemory(this->p_dev, p_uniformMemory[0], 0, bufferSize, 0, &uniformMappings[0]);
}

void ProgramVK::createDescriptorSet() {
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

void ProgramVK::recordCommandBuffer() {
    VkCommandBuffer cmdBuf = this->p_vkw->currentCommandBuffer();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->p_vkw->defaultRenderPass();
    renderPassInfo.framebuffer = this->p_vkw->currentFramebuffer();
    renderPassInfo.renderArea.extent = this->p_swapExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = 1;                                 // Optional
    renderPassInfo.pClearValues = &this->p_clearColor;                  // Optional

    this->p_vdf->vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    this->p_vdf->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, this->p_pipe);
    this->p_vdf->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, this->p_pipeLayout, 0, 1, &p_descSet[p_vkw->currentFrame()], 0, nullptr);
    this->p_vdf->vkCmdSetViewport(cmdBuf, 0, 1, &this->p_viewport);
    this->p_vdf->vkCmdSetScissor(cmdBuf, 0, 1, &this->p_scissor);
    this->p_vdf->vkCmdBindVertexBuffers(cmdBuf, 0, vbosBound, &this->p_vb, &this->p_offset);
    this->p_vdf->vkCmdBindIndexBuffer(cmdBuf, this->p_ib, 0, VK_INDEX_TYPE_UINT32);
    this->p_vdf->vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);
    this->p_vdf->vkCmdEndRenderPass(cmdBuf);
    this->p_vkw->frameReady();
    this->p_vkw->requestUpdate();
}

/**
 * Attribute binding (for use with Vertex Array/Buffer Objects) must happen
 * after initialization of the program but is only recognized on the next
 * linking. Linking may skip this step, but warnings will be given if done
 * out of order.
 * @param location - explicitly specify the integer binding target
 * @param name - std::string representation of the GLSL attribute name
 */
void ProgramVK::bindAttribute(int location, std::string name) {
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

VKuint ProgramVK::getShaderIdFromName(std::string& fileName) {
    assert(stage >= 2);
    assert(compiledShaders.count(fileName));
    
    return compiledShaders[fileName];
}

Shader* ProgramVK::getShaderFromName(std::string& fileName) {
    assert(stage >= 2);
    Shader *s = nullptr;
    
    for (const auto& shader : compiledShaders) {
        if (shader->getName() == fileName) {
            s = shader;
        }
    }

    if (s == nullptr) {
        throw std::runtime_error("Shader " + fileName + " not found.");
    }
    
    return s;
}

void ProgramVK::attachShader(std::string& name) {
    VKuint shID = getShaderIdFromName(name);
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
VKint ProgramVK::linkAndValidate() {
    if (stage < 2) {
        std::cout << "Invalid linking. Must init (and bind attributes) first." << std::endl;
        return 0;
    }

    VKint programValid = 0;
    
    // Link the compiled and attached program to this code.
    qgf->glLinkProgramVK(programId);
    if (programId == GL_INVALID_VALUE)
        exit(-1);

    // Verify program compilation and linkage.
    qgf->glValidateProgramVK(programId);
    qgf->glGetProgramVKiv(programId, GL_VALIDATE_STATUS, &programValid);
    if (!programValid)
        displayLogProgramVK();

    this->stage = programValid ? 5 : 4;

    return programValid;
}

/**
 * @brief Detach only attached program shaders.
 * This should be done after successful link and validate.
 */
void ProgramVK::detachShaders() {
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
void ProgramVK::detachDelete() {
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
 * A sequence-protected wrapper for glUseProgramVK().  This completely preempts
 * the OpenGL graphics pipeline for any shader functions implemented.
 */
void ProgramVK::enable() {
    if (stage < 5) {
        if (stage < 4)
        std::cout << "ProgramVK not ready to enable: must link before use." << std::endl;
        else
        std::cout << "ProgramVK not ready to enable: linked but not valid." << std::endl;

        return;
    }

    qgf->glUseProgramVK(this->programId);
    enabled = true;
}

/**
 * Set the current program to NULL and resume normal OpenGL (direct-mode)
 * operation.
 */
void ProgramVK::disable() {
    qgf->glUseProgramVK(0);
    enabled = false;
}

void ProgramVK::initVAO() {
    qgf->glGenVertexArrays(1, &this->vao);
}

void ProgramVK::bindVAO() {
    qgf->glBindVertexArray(this->vao);
}

void ProgramVK::clearVAO() {
    qgf->glBindVertexArray(0);
}

VKuint ProgramVK::bindVBO(std::string name, uint bufCount, uint bufSize, const VKfloat *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ARRAY_BUFFER);
    VKuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void ProgramVK::setAttributePointerFormat(VKuint attrIdx, VKuint binding, VKuint count, VKenum type, VKuint offset, VKuint step) {
    if (std::find(attribs.begin(), attribs.end(), attrIdx) != attribs.end())
        attribs.push_back(attrIdx);
    //qgf->glVertexAttribPointer(idx, count, GL_FLOAT, GL_FALSE, stride, offset);
    qgf->glVertexArrayAttribFormat(this->vao, attrIdx, count, type, GL_FALSE, offset);
    qgf->glVertexArrayAttribBinding(this->vao, attrIdx, binding);
    qgf->glVertexArrayBindingDivisor(this->vao, attrIdx, step);
}

void ProgramVK::setAttributeBuffer(VKuint binding, VKuint vboIdx, VKsizei stride) {
    qgf->glVertexArrayVertexBuffer(this->vao, binding, vboIdx, 0, stride);
}

void ProgramVK::enableAttribute(VKuint idx) {
    qgf->glEnableVertexArrayAttrib(this->vao, idx);
}

void ProgramVK::enableAttributes() {
    for (auto a : attribs)
        qgf->glEnableVertexArrayAttrib(this->vao, a);
}

void ProgramVK::disableAttributes() {
    for (auto a : attribs)
        qgf->glDisableVertexArrayAttrib(this->vao, a);
}

void ProgramVK::updateVBO(uint offset, uint bufCount, uint bufSize, const VKfloat *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }
    
    qgf->glBufferSubData(GL_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgramVK();
}

void ProgramVK::updateVBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const VKfloat *buf) {
    VKuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgramVK();
}

void ProgramVK::resizeVBONamed(std::string name, uint bufCount, uint bufSize, const VKfloat *buf, uint mode) {
    VKuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgramVK();
}

void ProgramVK::clearVBO() {
    qgf->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VKuint ProgramVK::bindEBO(std::string name, uint bufCount, uint bufSize, const VKuint *buf, uint mode) {
    this->buffers[name] = glm::uvec3(bufCount, 0, GL_ELEMENT_ARRAY_BUFFER);
    VKuint *bufId = &buffers[name].y;
    qgf->glGenBuffers(1, bufId);
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *bufId);
    qgf->glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufSize, buf, mode);

    return *bufId;
}

void ProgramVK::updateEBO(uint offset, uint bufCount, uint bufSize, const VKuint *buf) {
    int bufId = 0;
    qgf->glGetIntegerv(GL_ARRAY_BUFFER, &bufId);

    for (auto &m : this->buffers) {
        if (m.second.y == bufId) {
            m.second.x = bufCount;
        }
    }

    qgf->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, bufSize, buf);
    displayLogProgramVK();
}

void ProgramVK::updateEBONamed(std::string name, uint bufCount, uint offset, uint bufSize, const VKuint *buf) {
    VKuint bufId = this->buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferSubData(bufId, offset, bufSize, buf);
    displayLogProgramVK();
}

void ProgramVK::resizeEBONamed(std::string name, uint bufCount, uint bufSize, const VKuint *buf, uint mode) {
    VKuint bufId = buffers[name].y;
    this->buffers[name].x = bufCount;
    qgf->glNamedBufferData(bufId, bufSize, buf, mode);
    displayLogProgramVK();
}

void ProgramVK::clearEBO() {
    qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ProgramVK::assignFragColour() {
    qgf->glBindFragDataLocation(this->programId, GL_COLOR_ATTACHMENT0, "FragColour");
}

void ProgramVK::beginRender() {
    enable();
    bindVAO();
    // qgf->glBindBuffer(GL_ARRAY_BUFFER, this->vbo.back());
    // qgf->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo.back());
}

void ProgramVK::endRender() {
    clearVAO();
    disable();
}

void ProgramVK::clearBuffers() {
    clearVBO();
    clearEBO();
}

void ProgramVK::deleteBuffer(std::string name) {
    assert(!enabled);

    VKuint bufId = this->buffers[name].y;

    qgf->glDeleteBuffers(1, &bufId);
    this->buffers.erase(name);
}

bool ProgramVK::hasBuffer(std::string name) {
    return (buffers.find(name) != buffers.end());
}

/**
 * A quick wrapper for single, non-referenced uniform values.
 * @param type - GL_FLOAT or GL_INT
 * @param name - std::string representation of the GLSL uniform name
 * @param n - uniform value
 */
void ProgramVK::setUniform(int type, std::string name, double n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());
    float m = static_cast<float>(n);

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        std::cout << "Uniform failure: double to float" << std::endl;
}

void ProgramVK::setUniform(int type, std::string name, float n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        std::cout << "Uniform failure: float to float" << std::endl;
}

void ProgramVK::setUniform(int type, std::string name, int n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_INT) {
        qgf->glUniform1i(loc, n);
    } else
        std::cout << "Uniform failure: int to int" << std::endl;
}

void ProgramVK::setUniform(int type, std::string name, uint n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

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
void ProgramVK::setUniformv(int count, int size, int type, std::string name, const float *n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

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
void ProgramVK::setUniformMatrix(int size, std::string name, float *m) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (size == 4) {
        qgf->glUniformMatrix4fv(loc, 1, GL_FALSE, m);
    } else if (size == 3) {
        qgf->glUniformMatrix3fv(loc, 1, GL_FALSE, m);
    }
}

/**
 * Accessor function for the VKenum program ID.
 * @return the program ID
 */
VKuint ProgramVK::getProgramVKId() {
    return this->programId;
}

VKuint ProgramVK::getSize(std::string name) {
    return this->buffers[name].x;
}

/**
 * Displays the info log for this program.
 */
void ProgramVK::displayLogProgramVK() {
    VKsizei logLength;
    qgf->glGetProgramVKiv(this->programId, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength) {
        std::cout << "ProgramVK Info Log content available." << std::endl;

        VKsizei MAXLENGTH = 1 << 30;
        VKchar *logBuffer = new VKchar[logLength];
        qgf->glGetProgramVKInfoLog(this->programId, MAXLENGTH, &logLength, logBuffer);
        if (strlen(logBuffer)) {
            std::cout << "************ Begin ProgramVK Log ************" << "\n";
            std::cout << logBuffer << "\n";
            std::cout << "************* End ProgramVK Log *************" << std::endl;
        }
        delete[] logBuffer;
    }
}

/**
 * Displays the info log for the specified shader.
 * @param shader - the shader to be evaluated
 */
void ProgramVK::displayLogShader(VKenum shader) {
    VKint success;
    qgf->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
        std::cout << "Shader compile failure for shader #" << shader << std::endl;
    
    VKsizei logLength;
    qgf->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    VKsizei MAXLENGTH = 1 << 30;
    VKchar *logBuffer = new VKchar[MAXLENGTH];
    qgf->glGetShaderInfoLog(shader, MAXLENGTH, &logLength, logBuffer);
    if (strlen(logBuffer)) {
        std::cout << "************ Begin Shader Log ************" << "\n";
        std::cout << logBuffer << "\n";
        std::cout << "************* End Shader Log *************" << std::endl;
    }
    delete[] logBuffer;
}

void ProgramVK::printModel(ModelInfo *model) {
    std::cout << "Model: " << model->id << std::endl;
    
    if (!model->attributes) {
        std::cout << "    No attributes." << std::endl;
        return;
    }
        
    for (int i = 0; i < model->attributes->bindings.size(); i++) {
        std::cout << "    Binding " << i << ": " << model->attributes->bindings[i].binding << std::endl;

        for (int j = 0; j < model->attributes->attributes.size(); j++) {
            if (model->attributes->attributes[j].binding == i) {
                std::cout << "      Attribute " << j << ": " << model->attributes->attributes[j].location << std::endl;
                std::cout << "        Type: " << model->attributes->attributes[j].format << std::endl;
                std::cout << "        Offset: " << model->attributes->attributes[j].offset << std::endl;
            }
        }
    }
}
