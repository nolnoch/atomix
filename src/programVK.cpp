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

using std::cout;
using std::endl;
using std::string;


/**
 * Default Constructor.
 */
ProgramVK::ProgramVK(AtomixDevice *atomixDevice) 
    : p_dev(atomixDevice->device), p_phydev(atomixDevice->physicalDevice), p_vi(atomixDevice->instance), p_vf(atomixDevice->instance->functions()), p_vdf(atomixDevice->instance->deviceFunctions(this->p_dev)) {
}

/**
 * Default Destructor.
 */
ProgramVK::~ProgramVK() {
    delete samplers;

    for (auto &b : p_buffers) {
        delete b.second;
    }

    cleanup();
}

void ProgramVK::cleanup() {
    // cleanupSwapChain();

    this->p_vdf->vkDestroyBuffer(this->p_dev, indexBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, indexBufferMemory, nullptr);

    this->p_vdf->vkDestroyBuffer(this->p_dev, vertexBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, vertexBufferMemory, nullptr);
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
int ProgramVK::addShader(string fName, VKuint type) {
    int validFile = 0;
    string fileLoc;

    if (fName.find('/') == std::string::npos) {
        fileLoc = string(ROOT_DIR) + string(SHADERS) + fName;
    } else {
        fileLoc = fName;
    }

    this->registeredShaders.push_back(Shader(fileLoc, type));

    if (!this->registeredShaders.back().isValid()) {
        this->registeredShaders.pop_back();
        std::cout << "Failed to add shader source." << std::endl;
    } else {
        validFile = 1;
        this->stage = 1;
    }

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
int ProgramVK::addAllShaders(std::vector<std::string> *fList, VKuint type) {
    int errors = fList->size();
    VKuint shID, shIdx = 0;
    
    for (auto &fName : *fList) {
        uint validFile = 0;
        string fileLoc;
        
        if (fName.find('/') == std::string::npos) {
            fileLoc = string(ROOT_DIR) + string(SHADERS) + fName;
        } else {
            fileLoc = fName;
        }
        
        registeredShaders.push_back(Shader(fileLoc, type));
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
void ProgramVK::addDefaultShaders() {
    this->addShader("shader.vert", GL_VERTEX_SHADER);
    this->addShader("shader.frag", GL_FRAGMENT_SHADER);
    // this->addShader("shader.geom", GL_GEOMETRY_SHADER);
}

/**
 * Generates a sampler uniform bind target for use in the GLSL shader code.
 */
void ProgramVK::addSampler(string sName) {
    SamplerInfo info;
    VKuint sample;

    if (!samplers)
        this->samplers = new std::vector<SamplerInfo>();

    // qgf->glGenSamplers(1, &sample);

    info.samplerID = sample;
    info.samplerName = sName;

    this->samplers->push_back(info);
}

void ProgramVK::addBufferConfig(string name, ProgBufInfo &info) {
    this->p_buffers[name] = new ProgBufInfo(info);
}

/**
 * Initializes the program. Then initializes, loads, and compiles all shaders
 * associated with the ProgramVK object.
 */
void ProgramVK::init() {
    int numShaders = this->registeredShaders.size();

    if (!numShaders || !stage) {
        cout << "No shader files associated with program. Aborting..." << endl;
        return;
    }

    // Process registered shaders
    for (auto &shad : this->registeredShaders) {
        // TODO implement shader compilation
    }
    for (int i = 0; i < numShaders; i++) {
        // Init shader
        Shader *shad = &(this->registeredShaders[i]);
        VKuint id = qgf->glCreateShader(shad->type());
        shad->setId(id);

        // Load shader sources.
        const VKchar *shaderSource = shad->source().c_str();
        qgf->glShaderSource(id, 1, &shaderSource, NULL);

        // Compile shader from source.
        qgf->glCompileShader(id);

        // Display any problems with shader compilation.
        displayLogShader(id);

        // Add to compiledShaders map
        compiledShaders[shad->name()] = id;
    }

    createCommandPool();
    createCommandBuffers();
    createSwapChain();
    createRenderPass();
    createPipelineParts();
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
    SwapChainSupportInfo info;
    
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
    
    return info;
}

void ProgramVK::createSwapChain() {
    // VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(this->p_vkw->availableFormats);
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

    // this->p_vkw->swapchain = swapchain;
}

/* void ProgramVK::createImageViews() {
    
} */

void ProgramVK::createRenderPass() {
    VkAttachmentDescription2 colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    colorAttachment.format = this->p_vkw->format;
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

void ProgramVK::createPipelineParts() {
    // Input Assembly
    memset(&this->p_pipeInfo.ia, 0, sizeof(this->p_pipeInfo.ia));
    this->p_pipeInfo.ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    this->p_pipeInfo.ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

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

    // Rasterization
    memset(&this->p_pipeInfo.rs, 0, sizeof(this->p_pipeInfo.rs));
    this->p_pipeInfo.rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    this->p_pipeInfo.rs.polygonMode = VK_POLYGON_MODE_LINE;
    this->p_pipeInfo.rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    this->p_pipeInfo.rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    this->p_pipeInfo.rs.lineWidth = 1.0f;
    this->p_pipeInfo.rs.depthClampEnable = VK_FALSE;
    this->p_pipeInfo.rs.depthBiasEnable = VK_FALSE;
    this->p_pipeInfo.rs.depthBiasConstantFactor = 0.0f;
    this->p_pipeInfo.rs.depthBiasClamp = 0.0f;
    this->p_pipeInfo.rs.depthBiasSlopeFactor = 0.0f;

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
    memset(&this->p_pipeInfo.layoutInfo, 0, sizeof(this->p_pipeInfo.layoutInfo));
    this->p_pipeInfo.layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    this->p_pipeInfo.layoutInfo.setLayoutCount = 0;
    this->p_pipeInfo.layoutInfo.pSetLayouts = nullptr;
    this->p_pipeInfo.layoutInfo.pushConstantRangeCount = 0;
    this->p_pipeInfo.layoutInfo.pPushConstantRanges = nullptr;

    if (this->p_vdf->vkCreatePipelineLayout(this->p_dev, &this->p_pipeInfo.layoutInfo, nullptr, &this->p_pipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void ProgramVK::createPipeline() {
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = this->p_pipeInfo.shaderStages;
    pipelineInfo.pVertexInputState = &this->p_pipeInfo.vboInfo;
    pipelineInfo.pInputAssemblyState = &this->p_pipeInfo.ia;
    pipelineInfo.pViewportState = &this->p_pipeInfo.vp;
    pipelineInfo.pRasterizationState = &this->p_pipeInfo.rs;
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

ProgBufInfo* ProgramVK::getBufferInfo(std::string name) {
    for (auto b : this->p_buffers) {
        if (b.first == name) {
            return b.second;
        }
    }
    return nullptr;
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

void ProgramVK::createVertexBuffer(std::string name) {
    ProgBufInfo *buf = nullptr;
    
    if ((buf = getBufferInfo(name)) == nullptr) {
        throw std::runtime_error("Vertex buffer not found.");
    }

    int locs = buf->dataTypes.size();
    std::vector<uint> offsets;
    offsets.push_back(0);
    for (int i = 0; i < locs; i++) {
        offsets.push_back(dataSizes[static_cast<uint>(buf->dataTypes[i])]);
    }
    uint vboStride = std::accumulate(offsets.cbegin(), offsets.cend(), 0);

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = vbosBound++;
    bindingDescription.stride = vboStride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    uint prevDescs = this->p_pipeInfo.vboInfo.vertexAttributeDescriptionCount;

    for (int i = 0; i < locs; i++) {
        this->p_pipeInfo.attributeDescriptions.push_back({});
        this->p_pipeInfo.attributeDescriptions[prevDescs + i].binding = bindingDescription.binding;
        this->p_pipeInfo.attributeDescriptions[prevDescs + i].location = prevDescs + i;
        this->p_pipeInfo.attributeDescriptions[prevDescs + i].format = dataFormats[static_cast<uint>(buf->dataTypes[i])];
        this->p_pipeInfo.attributeDescriptions[prevDescs + i].offset = offsets[i];
    }

    this->p_pipeInfo.vboInfo.vertexBindingDescriptionCount = vbosBound;
    this->p_pipeInfo.vboInfo.vertexAttributeDescriptionCount = prevDescs + locs;
    if (prevDescs == 0) {
        this->p_pipeInfo.vboInfo.pVertexBindingDescriptions = &bindingDescription;
        this->p_pipeInfo.vboInfo.pVertexAttributeDescriptions = this->p_pipeInfo.attributeDescriptions.data();
    }

    createBuffer(buf->bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), stagingBuffer, stagingBufferMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, stagingBufferMemory, 0, buf->bufSize, 0, &data);
    std::copy(buf->data, buf->data + buf->bufSize, data);
    this->p_vdf->vkUnmapMemory(this->p_dev, stagingBufferMemory);

    createBuffer(buf->bufSize, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, buf->bufSize);

    this->p_vdf->vkDestroyBuffer(this->p_dev, stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, stagingBufferMemory, nullptr);
}

void ProgramVK::createIndexBuffer(std::string name) {
    ProgBufInfo *buf = nullptr;

    if ((buf = getBufferInfo(name)) == nullptr) {
        throw std::runtime_error("Index buffer not found.");
    }

    createBuffer(buf->bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), stagingBuffer, stagingBufferMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, stagingBufferMemory, 0, buf->bufSize, 0, &data);
    std::copy(buf->data, buf->data + buf->bufSize, data);
    this->p_vdf->vkUnmapMemory(this->p_dev, stagingBufferMemory);

    createBuffer(buf->bufSize, (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, buf->bufSize);

    this->p_vdf->vkDestroyBuffer(this->p_dev, stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, stagingBufferMemory, nullptr);
}

void ProgramVK::createUniformBuffer(std::string name) {
    ProgBufInfo *buf = nullptr;

    if ((buf = getBufferInfo(name)) == nullptr) {
        throw std::runtime_error("Uniform buffer not found.");
    }

    createBuffer(buf->bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}

void ProgramVK::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if ((this->p_vdf->vkBeginCommandBuffer(commandBuffer, &beginInfo)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->p_renderPass;
    renderPassInfo.framebuffer = this->p_frames[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->p_swapExtent;
    renderPassInfo.clearValueCount = 1; // Optional
    renderPassInfo.pClearValues = &this->p_clearColor; // Optional

    this->p_vdf->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    this->p_vdf->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->p_pipe);

    this->p_vdf->vkCmdSetViewport(commandBuffer, 0, 1, &this->p_viewport);
    this->p_vdf->vkCmdSetScissor(commandBuffer, 0, 1, &this->p_scissor);

    this->p_vdf->vkCmdBindVertexBuffers(commandBuffer, 0, vbosBound, &this->p_vb, &this->p_offset);
    this->p_vdf->vkCmdBindIndexBuffer(commandBuffer, this->p_ib, 0, VK_INDEX_TYPE_UINT32);

    this->p_vdf->vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
    this->p_vdf->vkCmdEndRenderPass(commandBuffer);

    if ((this->p_vdf->vkEndCommandBuffer(commandBuffer)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

/**
 * Attribute binding (for use with Vertex Array/Buffer Objects) must happen
 * after initialization of the program but is only recognized on the next
 * linking. Linking may skip this step, but warnings will be given if done
 * out of order.
 * @param location - explicitly specify the integer binding target
 * @param name - string representation of the GLSL attribute name
 */
void ProgramVK::bindAttribute(int location, string name) {
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

VKuint ProgramVK::getShaderIdFromName(std::string& fileName) {
    assert(stage >= 2);
    assert(compiledShaders.count(fileName));
    
    return compiledShaders[fileName];
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
        cout << "Invalid linking. Must init (and bind attributes) first." << endl;
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
        cout << "ProgramVK not ready to enable: must link before use." << endl;
        else
        cout << "ProgramVK not ready to enable: linked but not valid." << endl;

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
 * @param name - string representation of the GLSL uniform name
 * @param n - uniform value
 */
void ProgramVK::setUniform(int type, string name, double n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());
    float m = static_cast<float>(n);

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        cout << "Uniform failure: double to float" << endl;
}

void ProgramVK::setUniform(int type, string name, float n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_FLOAT) {
        qgf->glUniform1f(loc, n);
    } else
        cout << "Uniform failure: float to float" << endl;
}

void ProgramVK::setUniform(int type, string name, int n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

    if (type == GL_INT) {
        qgf->glUniform1i(loc, n);
    } else
        cout << "Uniform failure: int to int" << endl;
}

void ProgramVK::setUniform(int type, string name, uint n) {
    VKint loc = qgf->glGetUniformLocation(this->programId, name.c_str());

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
void ProgramVK::setUniformv(int count, int size, int type, string name, const float *n) {
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
 * @param name - string representation of the GLSL uniform name
 * @param m - pointer to the first matrix value
 */
void ProgramVK::setUniformMatrix(int size, string name, float *m) {
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
        cout << "ProgramVK Info Log content available." << endl;

        VKsizei MAXLENGTH = 1 << 30;
        VKchar *logBuffer = new VKchar[logLength];
        qgf->glGetProgramVKInfoLog(this->programId, MAXLENGTH, &logLength, logBuffer);
        if (strlen(logBuffer)) {
            cout << "************ Begin ProgramVK Log ************" << "\n";
            cout << logBuffer << "\n";
            cout << "************* End ProgramVK Log *************" << endl;
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
        cout << "Shader compile failure for shader #" << shader << endl;
    
    VKsizei logLength;
    qgf->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    VKsizei MAXLENGTH = 1 << 30;
    VKchar *logBuffer = new VKchar[MAXLENGTH];
    qgf->glGetShaderInfoLog(shader, MAXLENGTH, &logLength, logBuffer);
    if (strlen(logBuffer)) {
        cout << "************ Begin Shader Log ************" << "\n";
        cout << logBuffer << "\n";
        cout << "************* End Shader Log *************" << endl;
    }
    delete[] logBuffer;
}
