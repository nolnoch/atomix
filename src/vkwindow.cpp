/**
 * vkwindow.cpp
 *
 *    Created on: Oct 22, 2024
 *   Last Update: Oct 22, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 *
 *  Copyright 2024 Wade Burch (GPLv3)
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

#include <QMainWindow>
#include "vkwindow.hpp"

#define RADN(t) (glm::radians((t)))


VKWindow::VKWindow(QWidget *parent, ConfigParser *configParser)
    : QOpenGLWidget(parent), cfgParser(configParser) {
    setFocusPolicy(Qt::StrongFocus);
}

VKWindow::~VKWindow() {
    cleanup();
}

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VKRenderer::VKRenderer(VKWindow *vkWin)
    : vkw(vkWin) {
}

VKRenderer::~VKRenderer() {
    // TODO Destructor
}

void VKRenderer::initResources() {
    VkResult err;
    float vertexData[] = { 0.0f, 0.0f };

    // Define instance, device, and function pointers
    dev = vkw->device();
    vi = vkw->vulkanInstance();
    vdf = vi->deviceFunctions(dev);
    vf = vi->functions();

    // Retrieve physical device constraints
    const int concFrameCount = vkw->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdLimits = &vkw->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlignment = pdLimits->minUniformBufferOffsetAlignment;

    // Assign vertex buffer and uniforms
    VkBufferCreateInfo bufCreateInfo;
    memset(&bufCreateInfo, 0, sizeof(bufCreateInfo));
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlignment);
    const VkDeviceSize uniformAllocSize = aligned(sizeof(float) * 16, uniAlignment);
    bufCreateInfo.size = vertexAllocSize + concFrameCount * uniformAllocSize;
    bufCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    // Create vertex buffer
    err = vdf->vkCreateBuffer(dev, &bufCreateInfo, nullptr, &vr_buf);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create vertex buffer: %d", err);
    }

    // Get memory requirements for buffer vr_buf
    VkMemoryRequirements memReq;
    vdf->vkGetBufferMemoryRequirements(dev, vr_buf, &memReq);
    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        vkw->hostVisibleMemoryIndex()
    };

    // Allocate, bind, and map memory for buffer vr_buf
    err = vdf->vkAllocateMemory(dev, &memAllocInfo, nullptr, &vr_bufMem);
    if (err != VK_SUCCESS) {
        qFatal("Failed to allocate memory for vr_buf: %d", err);
    }
    err = vdf->vkBindBufferMemory(dev, vr_buf, vr_bufMem, 0);
    if (err != VK_SUCCESS) {
        qFatal("Failed to bind buffer memory: %d", err);
    }
    uint8_t *p;
    err = vdf->vkMapMemory(dev, vr_bufMem, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS) {
        qFatal("Failed to map buffer memory: %d", err);
    }

    // Copy data to mapped memory, then unmap
    memcpy(p, vertexData, sizeof(vertexData));
    glm::mat4 ident;
    memset(vr_uniformBufInfo, 0, sizeof(vr_uniformBufInfo));
    for (int i = 0; i < concFrameCount; i++) {
        const VkDeviceSize offset = vertexAllocSize + (i * uniformAllocSize);
        memcpy(p + offset, glm::value_ptr(ident), 16 * sizeof(float));
        vr_uniformBufInfo[i].buffer = vr_buf;
        vr_uniformBufInfo[i].offset = offset;
        vr_uniformBufInfo[i].range = uniformAllocSize;
    }
    vdf->vkUnmapMemory(dev, vr_bufMem);

    // Describe vertex data
    VkVertexInputBindingDescription vertexBindingDesc{};
    memset(&vertexBindingDesc, 0, sizeof(vertexBindingDesc));
    vertexBindingDesc.binding = 0;
    vertexBindingDesc.stride = 5 * sizeof(float);
    vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // Describe vertex attributes
    VkVertexInputAttributeDescription vertexAttrDesc[2]{};
    memset(vertexAttrDesc, 0, sizeof(vertexAttrDesc));
    // Position
    vertexAttrDesc[0].location = 0;
    vertexAttrDesc[0].binding = 0;
    vertexAttrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttrDesc[0].offset = 0;
    // Colour
    vertexAttrDesc[1].location = 1;
    vertexAttrDesc[1].binding = 0;
    vertexAttrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttrDesc[1].offset = 2 * sizeof(float);

    // Describe Pipeline using above structs (which define vertex data and attributes)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Define descriptor pool with layout
    VkDescriptorPoolSize descPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concFrameCount) };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concFrameCount;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &descPoolSizes;

    // Create descriptor pool
    err = vdf->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &vr_descPool);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create descriptor pool: %d", err);
    }

    // Define descriptor set layouts for binding and creation
    VkDescriptorSetLayoutBinding layoutBinding{};
    memset(&layoutBinding, 0, sizeof(layoutBinding));
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
    memset(&descLayoutInfo, 0, sizeof(descLayoutInfo));
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descLayoutInfo.pNext = nullptr;
    descLayoutInfo.flags = 0;
    descLayoutInfo.bindingCount = 1;
    descLayoutInfo.pBindings = &layoutBinding;

    // Create descriptor set
    err = vdf->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &vr_descSetLayout);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create descriptor set layout: %d", err);
    }

    // Define allocation info for descriptor sets
    VkDescriptorSetAllocateInfo descSetAllocInfo{};
    memset(&descSetAllocInfo, 0, sizeof(descSetAllocInfo));
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.pNext = nullptr;
    descSetAllocInfo.descriptorPool = vr_descPool;
    descSetAllocInfo.descriptorSetCount = concFrameCount;
    descSetAllocInfo.pSetLayouts = &vr_descSetLayout;

    // For each possible conccurrent frame...
    for (int i = 0; i < concFrameCount; ++i) {
        // Allocate descriptor set
        err = vdf->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &vr_descSet[i]);
        if (err != VK_SUCCESS) {
            qFatal("Failed to allocate descriptor set: %d", err);
        }

        // Fill descriptor set
        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = vr_descSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &vr_uniformBufInfo[i];
        vdf->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);
    }

    // Describe pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheInfo.pNext = nullptr;
    pipelineCacheInfo.flags = 0;
    pipelineCacheInfo.initialDataSize = 0;

    // Create pipeline cache
    err = vdf->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &vr_pipelineCache);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create pipeline cache: %d", err);
    }

    // Describe pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vr_descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    // Create pipeline layout
    err = vdf->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &vr_pipelineLayout);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create pipeline layout: %d", err);
    }

    // Create shader modules
    VkShaderModule vertShaderModule = createShader("vert.spv");
    VkShaderModule fragShaderModule = createShader("frag.spv");

    // Describe shader stage
    VkPipelineShaderStageCreateInfo shaderStages[2];
    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].flags = 0;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;
    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].flags = 0;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and Scissor
    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling.
    ms.rasterizationSamples = vkw->sampleCountFlagBits();

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // Color Blending
    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;

    // Dynamic State
    VkDynamicState dynEnable[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;

    // Describe pipeline for creation
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &ia;
    pipelineInfo.pViewportState = &vp;
    pipelineInfo.pRasterizationState = &rs;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &ds;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.pDynamicState = &dyn;
    pipelineInfo.layout = vr_pipelineLayout;
    pipelineInfo.renderPass = vkw->defaultRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Create pipeline
    err = vdf->vkCreateGraphicsPipelines(dev, vr_pipelineCache, 1, &pipelineInfo, nullptr, &vr_pipeline);
    if (err != VK_SUCCESS) {
        qFatal("Failed to create graphics pipeline: %d", err);
    }

    if (vertShaderModule) {
        vdf->vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    }
    if (fragShaderModule) {
        vdf->vkDestroyShaderModule(dev, fragShaderModule, nullptr);
    }
}

void VKRenderer::initSwapChainResources() {
    m4_proj = glm::perspective(glm::radians(45.0f), (float)vkw->width() / (float)vkw->height(), 0.1f, 100.0f);
}

/**
 * @brief Release all Vulkan resources allocated by this object.
 *
 * This function should be called when the renderer is no longer needed.
 * It will release all Vulkan resources allocated by the renderer,
 * including pipelines, pipeline layouts, pipeline caches, descriptor
 * set layouts, descriptor pools, buffers, and buffer memory.
 *
 * Note that this function does not release the Vulkan instance or
 * physical device, as these are owned by the QVulkanWindow.
 */
void VKRenderer::releaseResources() {
    if (vr_pipeline) {
        vdf->vkDestroyPipeline(dev, vr_pipeline, nullptr);
    }
    if (vr_pipelineLayout) {
        vdf->vkDestroyPipelineLayout(dev, vr_pipelineLayout, nullptr);
    }
    if (vr_pipelineCache) {
        vdf->vkDestroyPipelineCache(dev, vr_pipelineCache, nullptr);
    }
    if (vr_descSetLayout) {
        vdf->vkDestroyDescriptorSetLayout(dev, vr_descSetLayout, nullptr);
    }
    if (vr_descPool) {
        vdf->vkDestroyDescriptorPool(dev, vr_descPool, nullptr);
    }
    if (vr_buf) {
        vdf->vkDestroyBuffer(dev, vr_buf, nullptr);
    }
    if (vr_bufMem) {
        vdf->vkFreeMemory(dev, vr_bufMem, nullptr);
    }
}
