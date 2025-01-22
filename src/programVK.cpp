/**
 * programVK.cpp
 *
 *    Created on: Oct 21, 2013
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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

#include <unordered_set>

/**
 * Default Constructor.
 */
ProgramVK::ProgramVK(FileHandler *fileHandler) {
    p_fileHandler = fileHandler;
}

/**
 * Default Destructor.
 */
ProgramVK::~ProgramVK() {
    cleanup();
}

/**
 * Cleanup method to be called when the program is no longer needed.
 * 
 * This is a rather large method, as it is responsible for cleaning up all
 * allocated resources.  It is called by the destructor, but if the object
 * is not being destroyed, it should be called manually to prevent memory
 * leaks.
 * 
 * This method will deallocate all shaders, buffers, and pipeline objects.
 */
void ProgramVK::cleanup() {
    // clear active models
    this->clearActiveModels();

    // destruct models
    for (auto &model : p_models) {
        // renders
        for (auto &render : model->renders) {
            this->p_vdf->vkDestroyPipeline(this->p_dev, render->pipeline, nullptr);
            delete render;
        }
        model->renders.clear();
        
        // pipeline layouts
        delete model->pipeInfo;
        
        delete model;
    }
    this->p_models.clear();

    // shaders
    for (auto &shader : this->p_shaderModules) {
        this->p_vdf->vkDestroyShaderModule(this->p_dev, shader, nullptr);
    }
    this->p_shaderModules.clear();
    for (auto &shader : this->p_registeredShaders) {
        delete shader;
    }
    this->p_registeredShaders.clear();

    // desc sets
    for (auto &descLayout : this->p_setLayouts) {
        this->p_vdf->vkDestroyDescriptorSetLayout(this->p_dev, descLayout, nullptr);
    }
    this->p_setLayouts.clear();
    for (uint i = 0; i < this->p_descSets.size(); i++) {
        for (auto &buf : this->p_uniformBuffers[i]) {
            this->p_vdf->vkDestroyBuffer(this->p_dev, buf, nullptr);
        }
        this->p_uniformBuffers[i].clear();
        for (auto &mem : this->p_uniformBuffersMemory[i]) {
            this->p_vdf->vkFreeMemory(this->p_dev, mem, nullptr);
        }
        this->p_uniformBuffersMemory[i].clear();
    }
    this->p_uniformBuffers.clear();
    this->p_uniformBuffersMemory.clear();
    this->p_uniformBufferMappings.clear();
    this->p_mapDescriptors.clear();

    // buffers
    for (auto &buf : this->p_buffers) {
        this->p_vdf->vkDestroyBuffer(this->p_dev, buf, nullptr);
    }
    this->p_buffers.clear();
    for (auto &mem : this->p_buffersMemory) {
        this->p_vdf->vkFreeMemory(this->p_dev, mem, nullptr);
    }
    this->p_buffersMemory.clear();
    for (auto &info : this->p_buffersInfo) {
        delete info;
    }
    this->p_buffersInfo.clear();

    // descriptor pool
    if (this->p_descPool != VK_NULL_HANDLE) {
        this->p_vdf->vkDestroyDescriptorPool(this->p_dev, this->p_descPool, nullptr);
        this->p_descPool = VK_NULL_HANDLE;
    }

    // global pipeline objects
    if (this->p_fragmentOutput != VK_NULL_HANDLE) {
        this->p_vdf->vkDestroyPipeline(this->p_dev, this->p_fragmentOutput, nullptr);
        this->p_fragmentOutput = VK_NULL_HANDLE;
    }
    if (this->p_pipeCache != VK_NULL_HANDLE) {
        this->p_vdf->vkDestroyPipelineCache(this->p_dev, this->p_pipeCache, nullptr);
        this->p_pipeCache = VK_NULL_HANDLE;
    }

    // pipeline layouts
    for (auto &pipeLayout : p_pipeLayouts) {
        this->p_vdf->vkDestroyPipelineLayout(this->p_dev, pipeLayout, nullptr);
    }
    this->p_pipeLayouts.clear();
}

/**
 * Associate the ProgramVK with an AtomixDevice object.
 * 
 * This function sets the Vulkan Window and Instance Objects, Logical Device
 * and Function Objects, Physical Device and Function Objects, and Command Pool,
 * Queue, and Render Pass on the ProgramVK object.
 * 
 * @param atomixDevice - The AtomixDevice object to associate with the ProgramVK
 */
void ProgramVK::setInstance(AtomixDevice *atomixDevice) {
    // Link Vulkan Window and Instance Objects
    this->p_vkw = atomixDevice->window;
    this->p_vi = this->p_vkw->vulkanInstance();
    
    // Link Vulkan Logical Device and Function Objects
    this->p_dev = atomixDevice->device;
    this->p_vdf = this->p_vi->deviceFunctions(this->p_dev);
    
    // Link Vulkan Physical Device and Function Objects
    this->p_phydev = atomixDevice->physicalDevice;
    this->p_vf = this->p_vi->functions();

    // Link Command Pool, Queue, and Render Pass to Qt defaults
    this->p_cmdpool = this->p_vkw->graphicsCommandPool();
    this->p_queue = this->p_vkw->graphicsQueue();
    this->p_renderPass = this->p_vkw->defaultRenderPass();
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
        fileLoc = this->p_fileHandler->atomixFiles.shaders() + fName;
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
        shader->setId(this->p_registeredShaders.size());
        this->p_registeredShaders.push_back(shader);
        validFile = true;
        this->p_stage = 1;
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

/**
 * @brief Compile and reflect a shader.
 *
 * This function will compile the shader with the `compile` method and, if
 * successful, reflect the shader with the `reflect` method.  If the shader
 * fails to compile or reflect, the shader object will be deleted and the
 * function will return false.  If compilation and reflection are successful,
 * the shader object will be added to the `p_compiledShaders` vector and the
 * function will return true.
 *
 * @param shader - pointer to Shader object to compile and reflect
 * @return true if shader compiles and reflects successfully, false otherwise
 */
bool ProgramVK::compileShader(Shader *shader) {
    bool compiled = shader->compile(VK_SPIRV_VERSION);
    bool reflected = false;
    
    if (compiled) {
        this->p_compiledShaders.push_back(shader);
        this->p_mapShaders[shader->getName()] = this->p_compiledShaders.size() - 1;
        
        reflected = shader->reflect();
        if (!reflected) {
            std::cout << "Failed to reflect shader. Deleting shader..." << std::endl;
            delete shader;
        }
    } else {
        std::cout << "Failed to compile shader. Deleting shader..." << std::endl;
        delete shader;
    }

    return compiled && reflected;
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

    std::erase_if(this->p_registeredShaders, [this](Shader *s) { return !this->compileShader(s); });

    errors -= this->p_registeredShaders.size();

    for (uint i = 0; i < this->p_registeredShaders.size(); i++) {
        assert(this->p_registeredShaders[i]->getId() == this->p_compiledShaders[i]->getId());
    }

    for (auto &s : this->p_compiledShaders) {
        s->setStageIdx(this->createShaderStage(s));
    }

    return errors;
}

/**
 * @brief Create a Vulkan shader module from a Shader object.
 *
 * @param shader - pointer to Shader object to create module from
 * @return VkShaderModule& - reference to the created shader module
 */
VkShaderModule& ProgramVK::createShaderModule(Shader *shader) {
    if (!shader->isValidReflect()) {
        throw std::runtime_error("Shader not reflected: " + shader->getName());
    }

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = shader->getLengthCompiled() * sizeof(uint32_t);
    moduleCreateInfo.pCode = shader->getSourceCompiled();

    this->p_shaderModules.push_back({});
    VkShaderModule &shaderModule = this->p_shaderModules.back();
    err = p_vdf->vkCreateShaderModule(p_dev, &moduleCreateInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module: " + std::to_string(err));
    }

    return shaderModule;
}

/**
 * @brief Create a Vulkan shader stage from a Shader object.
 *
 * @param s - pointer to Shader object to create shader stage from
 * @return VKuint - index of created shader stage in p_shaderStages vector
 *
 * This function will create a shader stage information structure and store it
 * in the p_shaderStages vector. The shader stage will be for either the vertex
 * or fragment stage depending on the type of the Shader object. The entry point
 * name will be "main".
 */
VKuint ProgramVK::createShaderStage(Shader *s) {
    VkShaderStageFlagBits stage = (s->getType() == GL_VERTEX_SHADER) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;

    VKuint stageIdx = this->p_shaderStages.size();
    this->p_shaderStages.push_back({});
    VkPipelineShaderStageCreateInfo *shaderStage = &this->p_shaderStages.back();
    shaderStage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage->stage = stage;
    shaderStage->module = createShaderModule(s);
    shaderStage->pName = "main";

    return stageIdx;
}

/**
 * @brief Add all uniforms and push constants from shaders to the program's descriptor sets, uniform buffers, and push constant ranges.
 *
 * This function will iterate over all of the shaders in the program and add all of
 * the uniforms and push constants to the descriptor sets, uniform buffers, and push
 * constant ranges. It will also create a mapping from the uniform or push constant
 * name to the index in the p_setLayouts and p_pushConstRanges vectors, respectively.
 *
 * The function will also create a descriptor pool with the required number of
 * sets, and create a descriptor set for each set in the pool. The descriptor sets
 * will be stored in the p_descSets vector.
 *
 * The function will also create a uniform buffer for each uniform, and map it so
 * that the host can access it. The uniform buffers will be stored in the
 * p_uniformBuffers and p_uniformBuffersMemory vectors.
 *
 * The function will also create a push constant range for each push constant, and
 * store it in the p_pushConstRanges vector.
 *
 * Finally, the function will create a mapping from the uniform or push constant
 * name to the index in the p_uniformBufferMappings and p_pushConstMappings vectors,
 * respectively.
 */
void ProgramVK::addUniformsAndPushConstants() {
    std::vector<VKuint> sets;
    std::vector<VKuint> bindings;
    std::vector<VKuint> sizes;
    std::set<std::string> names;

    for (const auto &s : this->p_compiledShaders) {
        if (s->getType() == GL_VERTEX_SHADER) {
            for (const auto &uni : s->getUniforms()) {
                if (this->p_mapDescriptors.find(uni.name) == this->p_mapDescriptors.end()) {
                    names.insert(uni.name);
                }
            }
        }
    }
    
    // Descriptor Pool
    VKuint setCount = names.size();
    createDescriptorPool(setCount);
    this->p_descSets.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBufferMappings.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        this->p_descSets[i].resize(setCount, {});
        this->p_uniformBuffers[i].resize(setCount, {});
        this->p_uniformBuffersMemory[i].resize(setCount, {});
        this->p_uniformBufferMappings[i].resize(setCount, {});
    }

    // Uniform Buffers and Push Constants
    for (const auto &s : this->p_compiledShaders) {
        if (s->getType() == GL_VERTEX_SHADER) {
            for (const auto &uni : s->getUniforms()) {
                if (this->p_mapDescriptors.find(uni.name) == this->p_mapDescriptors.end()) {
                    VKuint j = this->p_setLayouts.size();

                    sets.push_back(uni.set);
                    bindings.push_back(uni.binding);
                    sizes.push_back(uni.size);

                    this->p_mapDescriptors[uni.name] = j;
                    assert(this->p_setLayouts.size() == j);
                    
                    // Create a descriptor set layout
                    s->addDescIdx(j);
                    createDescriptorSetLayout(uni.binding);
                    
                    // Create a uniform buffer with persistent mapping
                    for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                        createBuffer(uni.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), this->p_uniformBuffers[i][j], this->p_uniformBuffersMemory[i][j]);
                        this->p_vdf->vkMapMemory(this->p_dev, this->p_uniformBuffersMemory[i][j], 0, uni.size, 0, &this->p_uniformBufferMappings[i][j]);
                    }
                    if (isDebug) {
                        std::cout << "Uniform '" << uni.name << "' [" << "set: " << uni.set << ", binding: " << uni.binding << ", size: " << uni.size << "] added to program.\n";
                    }
                } else {
                    // std::cout << "Uniform " << uni.name << " already exists in program.\n";
                }
            }
            for (const auto &push : s->getPushConstants()) {
                // Create a push constant
                if (this->p_mapPushConsts.find(push.name) == this->p_mapPushConsts.end()) {
                    VKuint j = this->p_pushConsts.size();
                    this->p_mapPushConsts[push.name] = j;
                    s->setPushIdx(j);
                    this->p_pushConstRanges.push_back({});
                    
                    VkPushConstantRange *pcr = &this->p_pushConstRanges.back();
                    pcr->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    pcr->offset = 0;
                    pcr->size = push.size;
                    this->p_pushConsts.push_back(std::pair<uint64_t, void *>(push.size, nullptr));
                    if (isDebug) {
                        std::cout << "Push constant '" << push.name << "' [" << "size: " << push.size << "] added to program.\n";
                    }
                } else {
                    // std::cout << "Push constant " << push.name << " already exists in program.\n";
                }
            }
        }
    }
    if (isDebug) {
        std::cout << std::endl;
    }

    // Descriptor Sets
    for (uint i = 0; i < setCount; i++) {
        createDescriptorSets(sets[i], bindings[i], sizes[i]);
    }

    definePipeLayouts();
}

/**
 * @brief Add a model to the program.
 *
 * @param[in] info The creation information for the model.
 * @return The index of the added model.
 *
 * This function adds a model to the program, creating all necessary
 * resources including buffers, shaders, and pipelines. It also sets
 * up the pipeline libraries if enabled.
 *
 * If the model already exists, it will be updated with the new
 * information.
 */
VKuint ProgramVK::addModel(ModelCreateInfo &info) {
    assert(this->p_mapDescriptors.size());
    ModelInfo *model;
    VKuint idx = p_models.size();
    if (isDebug) {
        printInfo(&info);
    }

    // Check for existing model and add if it doesn't exist
    const auto [it, success] = p_mapModels.insert({info.name, idx});
    if (!success) {
        // TODO: Handle old model data
        std::cout << "Model already exists. Updating model " << info.name << "..." << std::endl;
        // model = p_models[it->second];
        return uint(-1);
    } else {
        model = new ModelInfo{};
        model->id = idx;
        model->name = info.name;
        p_models.push_back(model);
        p_mapModels[info.name] = model->id;
    }

    // Shaders
    model->valid.shaders = true;
    model->valid.uniforms = true;

    // Buffers: VBO
    for (auto &vbo : info.vbos) {
        idx = this->p_buffers.size();
        p_mapBuffers[vbo->name] = idx;
        p_mapBufferToModel[vbo->name] = model->id;
        this->p_buffers.push_back(VkBuffer{});
        this->p_buffersMemory.push_back(VkDeviceMemory{});
        this->p_buffersInfo.push_back(new BufferCreateInfo(*vbo));
        this->p_buffersInfo[idx]->id = idx;
        model->vbos.push_back(idx);
        if (vbo->data) {
            this->stageAndCopyBuffer(this->p_buffers.back(), this->p_buffersMemory.back(), BufferType::VERTEX, vbo->size, vbo->data);
            model->valid.vbo = true;
        }
    }
    this->defineBufferAttributes(info, model);

    // Buffers: IBO
    idx = this->p_buffers.size();
    p_mapBuffers[info.ibo->name] = idx;
    p_mapBufferToModel[info.ibo->name] = model->id;
    this->p_buffers.push_back(VkBuffer{});
    this->p_buffersMemory.push_back(VkDeviceMemory{});
    this->p_buffersInfo.push_back(new BufferCreateInfo(*info.ibo));
    this->p_buffersInfo[idx]->id = idx;
    model->ibo = idx;
    if (info.ibo->data) {
        this->stageAndCopyBuffer(this->p_buffers.back(), this->p_buffersMemory.back(), BufferType::INDEX, info.ibo->size, info.ibo->data);
        model->valid.ibo = true;
    }

    // Pipeline Model Setup
    this->pipelineModelSetup(info, model);
    if (!info.pushConstant.empty()) {
        VKuint pcrIdx = this->p_mapPushConsts[info.pushConstant];
        model->pipeLayouts.push_back(pcrIdx + 1);
    } else {
        model->pipeLayouts.push_back(0);
    }

    // Pipeline Libraries
    if (p_libEnabled) {
        model->pipeInfo->library = new PipelineLibrary{};
        auto hash = [](const std::pair<VKuint, VKuint> &p) {
            return std::hash<VKuint>()(p.first) ^ std::hash<VKuint>()(p.second);
        };
        std::map<uint64_t, int> vis;
        std::map<uint64_t, int> pre;
        std::map<uint64_t, int> fsc;
        uint64_t hashVal = 0;

        for (auto &off : info.offsets) {
            int v = 0, p = 0, f = 0;
            VKuint tv = 0, tp = 0, tf = 0;
            
            hashVal = hash({off.bufferComboIndex, off.topologyIndex});
            if (vis.insert({hashVal, v}).second) {
                this->genVertexInputPipeLib(model, off.bufferComboIndex, off.topologyIndex);
                tv = v++;
            } else {
                tv = vis[hashVal];
            }
            hashVal = hash({off.vertShaderIndex, off.pushConstantIndex});
            if (pre.insert({hashVal, p}).second) {
                this->genPreRasterizationPipeLib(model, off.vertShaderIndex, off.pushConstantIndex);
                tp = p++;
            } else {
                tp = pre[hashVal];
            }
            hashVal = std::hash<VKuint>()(off.fragShaderIndex);
            if (fsc.insert({hashVal, f}).second) {
                this->genFragmentShaderPipeLib(model, off.fragShaderIndex);
                tf = f++;
            } else {
                tf = fsc[hashVal];
            }

            off.offsetLibs = VKtuple(tv, tp, tf);
        }
    }

    // Generate index counts for Render objects based on specifed offsets
    std::vector<VKuint> indexCount;
    for (uint i = 0; i < info.offsets.size(); i++) {
        VKuint end = 0;
        if ((i < info.offsets.size() - 1) && (info.offsets[i].offset != info.offsets[i+1].offset)) {
            end = info.offsets[i+1].offset;
        } else {
            end = info.ibo->count;
        }
        indexCount.push_back(end - info.offsets[i].offset);
    }

    // Populate Render objects with parameters and final Pipelines
    for (uint i = 0; i < info.offsets.size(); i++) {
        OffsetInfo &off = info.offsets[i];
        model->renders.push_back(new RenderInfo{});
        RenderInfo *render = model->renders.back();
        
        for (auto &vIdx : info.bufferCombos[off.bufferComboIndex]) {
            render->vbos.push_back(vIdx);
        }
        render->vboOffsets.resize(model->vbos.size(), 0);
        
        render->indexOffset = off.offset;
        render->indexCount = indexCount[i];
        
        if (info.pushConstant.empty()) {
            render->pushConst = -1;
            render->pipeLayoutIndex = 0;
        } else {
            VKuint pcrIdx = this->p_mapPushConsts[info.pushConstant];
            render->pushConst = pcrIdx;
            render->pipeLayoutIndex = 0;
        }
        
        if (p_libEnabled) {
            // Final Pipelines for Renders (Libraries)
            this->createPipeFromLibraries(render, model, std::get<0>(off.offsetLibs), std::get<1>(off.offsetLibs), std::get<2>(off.offsetLibs));
        } else {
            // Final Pipelines for Renders (Full PSO)
            VKuint vs = this->getShaderFromName(info.vertShaders[off.vertShaderIndex])->getStageIdx();
            VKuint fs = this->getShaderFromName(info.fragShaders[off.fragShaderIndex])->getStageIdx();
            this->createPipeline(render, model, vs, fs, off.bufferComboIndex, off.topologyIndex);
        }
    }
    for (uint i = 0; i < info.programs.size(); i++) {
        model->programs = info.programs;
    }
    model->activePrograms.clear();
    model->valid.renders = true;

    if (isDebug) {
        printModel(model);
        if (model->valid.validate()) {
            std::cout << "Model added and validated: " << info.name << std::endl;
        } else {
            std::cout << "Model added but not validated: " << info.name << std::endl;
        }
    }

    return model->id;
}

/**
 * Activate a model for rendering.  The model must first be validated
 * with addModel().  Once activated, the model's active programs will
 * be rendered when drawFrame() is called.
 *
 * @param name - the name of the model to activate
 * @return true if the model was activated, false otherwise
 */
bool ProgramVK::activateModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    bool success = false;

    if (this->p_models[id]->valid.validate()) {
        if ((success = p_activeModels.insert(id).second)) {
            return success;
        } else {
            std::cout << "Model already added to active models: " << name << std::endl;
        }
    } else {
        std::cout << "Model not validated and not added to active models: " << name << std::endl;
    }
    
    return success;
}

/**
 * Associate a program with a model for rendering.  The model must first be
 * added to the active models list with activateModel() and the program must
 * exist in the model's list of programs.
 *
 * @param name - the name of the model to associate the program with
 * @param program - the name of the program to associate with the model
 * @return true if the association was successful, false otherwise
 */
bool ProgramVK::addModelProgram(const std::string &name, const std::string &program) {
    VKuint id = getModelIdFromName(name);
    ModelInfo *model = p_models[id];
    VKint programId = -1;

    if (!p_activeModels.contains(id)) {
        std::cout << "Model not active: " << name << std::endl;
        return false;
    }

    for (uint i = 0; i < model->programs.size(); i++) {
        if (model->programs[i].name == program) {
            programId = i;
            break;
        }
    }

    if (programId == -1) {
        std::cout << "Program not found: " << program << std::endl;
        return false;
    }
    
    return (model->activePrograms.insert(programId).second);
}

/**
 * Disassociate a program from a model.  The model must first be
 * added to the active models list with activateModel() and the program must
 * exist in the model's list of programs.
 *
 * @param name - the name of the model to disassociate the program from
 * @param program - the name of the program to disassociate from the model
 * @return true if the disassociation was successful, false otherwise
 */
bool ProgramVK::removeModelProgram(const std::string &name, const std::string &program) {
    VKuint id = getModelIdFromName(name);
    ModelInfo *model = p_models[id];
    VKint programId = -1;

    if (!p_activeModels.contains(id)) {
        std::cout << "Model not active: " << name << std::endl;
        return false;
    }

    for (uint i = 0; i < model->programs.size(); i++) {
        if (model->programs[i].name == program) {
            programId = i;
            break;
        }
    }

    if (programId == -1) {
        std::cout << "Program not found: " << program << std::endl;
        return false;
    }
    
    return (model->activePrograms.erase(programId));
}

/**
 * Clear all active programs from a model.  The model must first be
 * added to the active models list with activateModel().
 *
 * @param name - the name of the model to clear programs from
 * @return true if the model was found and cleared, false otherwise
 */
bool ProgramVK::clearModelPrograms(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    bool success = false;

    if (p_activeModels.contains(id)) {
        p_models[id]->activePrograms.clear();
        success = true;
    }
    
    return success;
}

/**
 * Deactivate a model and remove it from the active models list.
 * This will prevent the model from being rendered and will also
 * clear any active programs from the model.
 *
 * @param name - the name of the model to deactivate
 * @return true if the model was found and deactivated, false otherwise
 */
bool ProgramVK::deactivateModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    bool success = false;

    if (p_activeModels.contains(id)) {
        p_models[id]->activePrograms.clear();
        success = (p_activeModels.erase(id) > 0);
    }

    return success;
}

/**
 * Suspend a model from rendering.  The model must first be
 * added to the active models list with activateModel().
 *
 * @param name - the name of the model to suspend
 * @return true if the model was found and suspended, false otherwise
 */
bool ProgramVK::suspendModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    bool success = false;

    if (p_activeModels.contains(id)) {
        p_models[id]->valid.suspended = true;
        success = true;
    }

    return success;
}

/**
 * Suspend all active models from rendering.  Models must first be
 * added to the active models list with activateModel().
 *
 * @return true if any models were suspended, false otherwise
 */
bool ProgramVK::suspendActiveModels() {
    bool success = false;

    for (VKuint id : p_activeModels) {
        p_models[id]->valid.suspended = true;
        success = true;
    }

    return success;
}

/**
 * Resume a suspended model.  The model must first be
 * added to the active models list with activateModel(),
 * and then suspended with suspendModel().
 *
 * @param name - the name of the model to resume
 * @return true if the model was found and resumed, false otherwise
 */
bool ProgramVK::resumeModel(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    bool success = false;

    if (p_activeModels.contains(id)) {
        p_models[id]->valid.suspended = false;
        success = true;
    }

    return success;
}

/**
 * Resume all active models from suspension.  Models must first be
 * added to the active models list with activateModel(),
 * and then suspended with suspendModel() or suspendActiveModels().
 *
 * @return true if any models were resumed, false otherwise
 */
bool ProgramVK::resumeActiveModels() {
    bool success = false;

    for (VKuint id : p_activeModels) {
        p_models[id]->valid.suspended = false;
        success = true;
    }

    return success;
}

/**
 * Check if a model is currently suspended.
 *
 * @param name - the name of the model to query
 * @return true if the model is suspended, false otherwise
 */
bool ProgramVK::isSuspended(const std::string &name) {
    VKuint id = getModelIdFromName(name);
    return p_activeModels.contains(id) && p_models[id]->valid.suspended;
}

/**
 * Clear the active models list.  This should be called
 * when exiting the current rendering context, or when
 * all models should be removed from the current rendering
 * context.
 */
void ProgramVK::clearActiveModels() {
    p_activeModels.clear();
}

/**
 * Create a pipeline cache for the currently associated device.
 * This is a Vulkan requirement for pipeline creation.
 *
 * @throws std::runtime_error if the pipeline cache could not be created
 */
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

/**
 * Saves the current pipeline to the associated pipeline cache.
 * This is a Vulkan requirement for pipeline creation.
 *
 * @throws std::runtime_error if the pipeline cache data could not be retrieved
 */
void ProgramVK::savePipelineToCache() {
    std::cout << "Saving pipeline to cache..." << std::endl;

    err = p_vdf->vkGetPipelineCacheData(p_dev, p_pipeCache, nullptr, nullptr);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to get pipeline cache data: " + std::to_string(err));
    }
}

/**
 * Loads the pipeline from the associated pipeline cache.
 * This is a Vulkan requirement for pipeline creation.
 *
 * @throws std::runtime_error if the pipeline cache data could not be retrieved
 */
void ProgramVK::loadPipelineFromCache() {
    std::cout << "Loading pipeline from cache..." << std::endl;

}


/**
 * Initializes the ProgramVK object.
 *
 * Checks if there are any shaders registered and if the program is not already
 * initialized. If either condition is not met, the function will print a
 * message and return false.
 *
 * Otherwise, it will compile all registered shaders, set up the global pipeline
 * by calling pipelineGlobalSetup, and set the program stage to 2.
 *
 * @return true if the program was successfully initialized, false otherwise
 */
bool ProgramVK::init() {
    int numShaders = this->p_registeredShaders.size();

    if (!numShaders || !p_stage) {
        std::cout << "No shader files associated with program. Aborting..." << std::endl;
        return false;
    }

    // Process registered shaders
    compileAllShaders();
    addUniformsAndPushConstants();

    // Init pipeline cache and global setup
    createPipelineCache();
    this->pipelineGlobalSetup();

    this->p_stage = 2;

    return true;
}

/**
 * Finds the indices of the graphics and present queue families for a physical device.
 *
 * @param device - the physical device to query
 * @return a QueueFamilyIndices object containing the indices of the graphics and present queue families
 */
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

/**
 * Creates the render pass for this program.
 *
 * The render pass is created using one attachment with the format returned by
 * QVulkanWindow::colorFormat(). The attachment is cleared before use and
 * stored after use. The stencil load and store operations are set to
 * VK_ATTACHMENT_LOAD_OP_DONT_CARE and VK_ATTACHMENT_STORE_OP_DONT_CARE,
 * respectively. The initial and final layouts of the attachment are set to
 * VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, respectively.
 *
 * One subpass is created that references the attachment and has a pipeline bind
 * point of VK_PIPELINE_BIND_POINT_GRAPHICS. The render pass is created using
 * the render pass create info and the result is stored in p_renderPass.
 */
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

/**
 * @brief Defines pipeline layouts with no push constants and one layout per push constant.
 *
 * Two types of pipeline layouts are created: one with no push constants and one with
 * one push constant range per layout. The layouts are stored in p_pipeLayouts.
 *
 * If the creation of any pipeline layout fails, an exception is thrown.
 */
void ProgramVK::definePipeLayouts() {
    
    // Define pipeline layout with no push constants
    VkPipelineLayoutCreateInfo layNo{};
    layNo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layNo.setLayoutCount = this->p_setLayouts.size();
    layNo.pSetLayouts = this->p_setLayouts.data();
    layNo.pushConstantRangeCount = 0;
    layNo.pPushConstantRanges = VK_NULL_HANDLE;

    this->p_pipeLayouts.push_back({});
    if (this->p_vdf->vkCreatePipelineLayout(this->p_dev, &layNo, nullptr, &this->p_pipeLayouts.back()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Define pipeline layout with push constants, one layout per push constant
    for (auto &pcr : this->p_pushConstRanges) {
        VkPipelineLayoutCreateInfo lay{};
        lay.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        lay.setLayoutCount = this->p_setLayouts.size();
        lay.pSetLayouts = this->p_setLayouts.data();
        lay.pushConstantRangeCount = 1;
        lay.pPushConstantRanges = &pcr;

        this->p_pipeLayouts.push_back({});
        if (this->p_vdf->vkCreatePipelineLayout(this->p_dev, &lay, nullptr, &this->p_pipeLayouts.back()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }
    
}

/**
 * @brief Initializes the pipeline info of a model.
 *
 * This function takes a ModelCreateInfo and a ModelInfo object and initializes the
 * pipeline info of the model. It creates the necessary Vulkan pipeline input assembly
 * and vertex input state create infos.
 *
 * @param info The ModelCreateInfo object to initialize the pipeline info from.
 * @param m The ModelInfo object to initialize the pipeline info for.
 */
void ProgramVK::pipelineModelSetup(ModelCreateInfo &info, ModelInfo *m) {
    m->pipeInfo = new ModelPipelineInfo{};

    // For each topology
    for (auto &topology : info.topologies) {
        // Input Assembly
        m->pipeInfo->iaCreates.push_back({});
        VkPipelineInputAssemblyStateCreateInfo *ia = &m->pipeInfo->iaCreates.back();
        ia->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia->topology = topology;
        ia->primitiveRestartEnable = VK_FALSE;
        ia->flags = 0;
        ia->pNext = nullptr;
    }

    // Vertex Input Info
    for (const auto &attr : m->attributes) {
        m->pipeInfo->vboCreates.push_back({});
        VkPipelineVertexInputStateCreateInfo *vboCreate = &m->pipeInfo->vboCreates.back();
        vboCreate->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vboCreate->vertexBindingDescriptionCount = attr->bindings.size();
        vboCreate->vertexAttributeDescriptionCount = attr->attributes.size();
        vboCreate->pVertexBindingDescriptions = attr->bindings.data();
        vboCreate->pVertexAttributeDescriptions = attr->attributes.data();
        vboCreate->flags = 0;
        vboCreate->pNext = nullptr;
    }
}

/**
 * @brief Initializes the global pipeline info.
 *
 * This function initializes the global pipeline info which is used by all models.
 * It sets up the viewport, scissor, tessellation, dynamic state, rasterization,
 * multisampling, depth stencil, and color blending pipeline state create infos.
 *
 * The pipeline state infos are stored in the p_pipeInfo object.
 */
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

    // Rasterization
    this->p_pipeInfo.rsCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    this->p_pipeInfo.rsCreate.pNext = nullptr;
    this->p_pipeInfo.rsCreate.flags = 0;
    this->p_pipeInfo.rsCreate.polygonMode = VK_POLYGON_MODE_FILL;
    this->p_pipeInfo.rsCreate.cullMode = VK_CULL_MODE_BACK_BIT;  // VK_CULL_MODE_BACK_BIT or VK_CULL_MODE_NONE
    this->p_pipeInfo.rsCreate.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    this->p_pipeInfo.rsCreate.lineWidth = 1.0f;
    this->p_pipeInfo.rsCreate.rasterizerDiscardEnable = VK_FALSE;
    this->p_pipeInfo.rsCreate.depthClampEnable = VK_FALSE;
    this->p_pipeInfo.rsCreate.depthBiasEnable = VK_FALSE;
    this->p_pipeInfo.rsCreate.depthBiasConstantFactor = 0.0f;
    this->p_pipeInfo.rsCreate.depthBiasClamp = 0.0f;
    this->p_pipeInfo.rsCreate.depthBiasSlopeFactor = 0.0f;

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

/**
 * @brief Creates a graphics pipeline.
 *
 * Creates a graphics pipeline using the specified vertex and fragment shaders,
 * and the specified vertex buffer and index buffer.
 *
 * @param[in] render Render information to store the created pipeline in.
 * @param[in] m Model information to create the pipeline for.
 * @param[in] vs Index of the vertex shader to use.
 * @param[in] fs Index of the fragment shader to use.
 * @param[in] vbo Index of the vertex buffer to use.
 * @param[in] ia Index of the index buffer to use.
 */
void ProgramVK::createPipeline(RenderInfo *render, ModelInfo *m, int vs, int fs, int vbo, int ia) {
    std::vector<VkPipelineShaderStageCreateInfo> shaderModules = { this->p_shaderStages[vs], this->p_shaderStages[fs] };

    // Global pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderModules.data();
    pipelineInfo.pViewportState = &this->p_pipeInfo.vp;
    pipelineInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipelineInfo.pRasterizationState = &this->p_pipeInfo.rsCreate;
    pipelineInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipelineInfo.pDepthStencilState = &this->p_pipeInfo.ds;
    pipelineInfo.pColorBlendState = &this->p_pipeInfo.cb;
    pipelineInfo.renderPass = this->p_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // Model-specific pipeline
    pipelineInfo.pVertexInputState = &m->pipeInfo->vboCreates[vbo];
    pipelineInfo.pInputAssemblyState = &m->pipeInfo->iaCreates[ia];
    pipelineInfo.layout = this->p_pipeLayouts[m->pipeLayouts[0]];

    if ((this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, p_pipeCache, 1, &pipelineInfo, nullptr, &render->pipeline)) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    m->valid.pipelines = true;
}

/**
 * @brief Generates a vertex input pipeline library part.
 *
 * Generates a model-specific pipeline library part for the vertex input state.
 * This pipeline library part is used to optimize the linking of the pipeline.
 * When a pipeline is created, it is linked against all the pipeline library
 * parts that are relevant to the pipeline. This allows the Vulkan driver to
 * optimize the pipeline by only compiling the shaders that are needed.
 *
 * @param[in] m Model information to generate the pipeline library part for.
 * @param[in] vbo Index of the vertex buffer to use.
 * @param[in] ia Index of the index buffer to use.
 */
void ProgramVK::genVertexInputPipeLib(ModelInfo *m, int vbo, int ia) {
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
    pipeCreateLibVBOInfo.pVertexInputState = &m->pipeInfo->vboCreates[vbo];
    pipeCreateLibVBOInfo.pInputAssemblyState = &m->pipeInfo->iaCreates[ia];
    pipeCreateLibVBOInfo.pDynamicState = &this->p_pipeInfo.dyn;

    m->pipeInfo->library->vertexInput.push_back({});
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibVBOInfo, nullptr, &m->pipeInfo->library->vertexInput.back());
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vertex Input pipeline library!");
    }
}

/**
 * @brief Generates a pre-rasterization pipeline library part.
 *
 * Generates a model-specific pipeline library part for the pre-rasterization state.
 * This pipeline library part is used to optimize the linking of the pipeline.
 * When a pipeline is created, it is linked against all the pipeline library
 * parts that are relevant to the pipeline. This allows the Vulkan driver to
 * optimize the pipeline by only compiling the shaders that are needed.
 *
 * @param[in] m Model information to generate the pipeline library part for.
 * @param[in] vs Index of the vertex shader to use.
 * @param[in] lay Index of the pipeline layout to use.
 */
void ProgramVK::genPreRasterizationPipeLib(ModelInfo *m, int vs, int lay) {
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
    pipeCreateLibPRSInfo.pStages = &this->p_shaderStages[vs];
    pipeCreateLibPRSInfo.pRasterizationState = &this->p_pipeInfo.rsCreate;
    pipeCreateLibPRSInfo.pViewportState = &this->p_pipeInfo.vp;
    pipeCreateLibPRSInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipeCreateLibPRSInfo.layout = this->p_pipeLayouts[m->pipeLayouts[lay]];
    pipeCreateLibPRSInfo.renderPass = this->p_renderPass;
    pipeCreateLibPRSInfo.subpass = 0;

    m->pipeInfo->library->preRasterization.push_back({});
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibPRSInfo, nullptr, &m->pipeInfo->library->preRasterization.back());
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Pre-Rasterization pipeline library!");
    }
}

/**
 * @brief Generates a fragment shader pipeline library part.
 *
 * Generates a model-specific pipeline library part for the fragment shader
 * state. This pipeline library part is used to optimize the linking of the
 * pipeline. When a pipeline is created, it is linked against all the pipeline
 * library parts that are relevant to the pipeline. This allows the Vulkan
 * driver to optimize the pipeline by only compiling the shaders that are
 * needed.
 *
 * @param[in] m Model information to generate the pipeline library part for.
 * @param[in] fs Index of the fragment shader to use.
 */
void ProgramVK::genFragmentShaderPipeLib(ModelInfo *m, int fs) {
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
    pipeCreateLibFSInfo.pStages = &this->p_shaderStages[fs];
    pipeCreateLibFSInfo.pDepthStencilState = &this->p_pipeInfo.ds;
    pipeCreateLibFSInfo.pMultisampleState = &this->p_pipeInfo.ms;
    pipeCreateLibFSInfo.pDynamicState = &this->p_pipeInfo.dyn;
    pipeCreateLibFSInfo.renderPass = this->p_renderPass;
    pipeCreateLibFSInfo.subpass = 0;

    m->pipeInfo->library->fragmentShader.push_back({});
    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibFSInfo, nullptr, &m->pipeInfo->library->fragmentShader.back());
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Fragment Shader pipeline library!");
    }
}

/**
 * @brief Generates a fragment output pipeline library part.
 *
 * Generates a global pipeline library part for the fragment output state.
 * This pipeline library part is used to optimize the linking of the pipeline.
 * When a pipeline is created, it is linked against all the pipeline library
 * parts that are relevant to the pipeline. This allows the Vulkan driver to
 * optimize the pipeline by only compiling the shaders that are needed.
 */
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

    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeCreateLibFOInfo, nullptr, &this->p_fragmentOutput);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Fragment Output pipeline library!");
    }
}

/**
 * @brief Creates a graphics pipeline from the given pipeline libraries.
 *
 * Creates a graphics pipeline using the given pipeline libraries.
 * The pipeline is created from the given vertex input, pre-rasterization,
 * fragment shader, and fragment output pipeline libraries.
 *
 * @param[in] render The render information to create the pipeline for.
 * @param[in] m The model information to create the pipeline for.
 * @param[in] vis The index of the vertex input pipeline library to use.
 * @param[in] pre The index of the pre-rasterization pipeline library to use.
 * @param[in] frag The index of the fragment shader pipeline library to use.
 */
void ProgramVK::createPipeFromLibraries(RenderInfo *render, ModelInfo *m, int vis, int pre, int frag) {
    std::vector<VkPipeline> libs = { m->pipeInfo->library->vertexInput[vis], m->pipeInfo->library->preRasterization[pre], m->pipeInfo->library->fragmentShader[frag], this->p_fragmentOutput };

    VkPipelineLibraryCreateInfoKHR pipeLibLinkInfo{};
    pipeLibLinkInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
    pipeLibLinkInfo.pNext = nullptr;
    pipeLibLinkInfo.libraryCount = static_cast<uint32_t>(libs.size());
    pipeLibLinkInfo.pLibraries = libs.data();

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.pNext = &pipeLibLinkInfo;
    pipeInfo.flags = VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT;

    err = this->p_vdf->vkCreateGraphicsPipelines(this->p_dev, this->p_pipeCache, 1, &pipeInfo, nullptr, &render->pipeline);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void ProgramVK::updatePipeFromLibraries() {
    // Update pipe with new pipeLibs
}

/**
 * @brief Creates a command pool for the given physical device.
 *
 * Creates a command pool for the given physical device using the graphics
 * family index from the given QueueFamilyIndices.
 *
 * @throws std::runtime_error if the command pool could not be created
 */
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

/**
 * @brief Allocate a single command buffer from the command pool.
 *
 * Allocates a single command buffer from the command pool created by
 * createCommandPool(). The command buffer is allocated as a primary command buffer.
 *
 * @throws std::runtime_error if the command buffer could not be allocated
 */
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

/**
 * Finds a memory type that matches the given type filter and property flags.
 *
 * @param[in] typeFilter a bit field of memory types to filter against
 * @param[in] flagProperties the memory property flags to filter against
 * @return the index of the memory type that matches the given criteria
 * @throws std::runtime_error if no matching memory type is found
 */
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

/**
 * @brief Create a Vulkan buffer with the given properties, and allocate memory for it.
 *
 * Creates a Vulkan buffer with the given size, usage, and properties. It then
 * allocates memory for the buffer with the given properties and binds the buffer
 * to the allocated memory.
 *
 * @param[in] size the size of the buffer to create
 * @param[in] usage the usage of the buffer to create
 * @param[in] properties the properties of the buffer memory to allocate
 * @param[out] buffer the created buffer handle
 * @param[out] bufferMemory the allocated memory handle
 * @throws std::runtime_error if the buffer or memory could not be created
 */
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

/**
 * @brief Copies the contents of one buffer to another.
 *
 * Copies the contents of one buffer to another. This function is meant to be used
 * for copying data from a staging buffer to a device local buffer.
 *
 * @param[in] dst the destination buffer to copy to
 * @param[in] src the source buffer to copy from
 * @param[in] size the size of the region to copy
 */
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

/**
 * @brief Defines the buffer attributes of a model.
 *
 * This function defines the buffer attributes of a model. It creates an AttribInfo
 * object for each buffer combination of the model and defines the vertex input
 * bindings and attributes.
 *
 * @param[in] info The ModelCreateInfo object containing the information
 *                 about the model.
 * @param[in] m The ModelInfo object to define the buffer attributes for.
 */
void ProgramVK::defineBufferAttributes(ModelCreateInfo &info, ModelInfo *m) {
    for (auto &combo : info.bufferCombos) {
        m->attributes.push_back(new AttribInfo{});
        AttribInfo *attrib = m->attributes.back();
        int bindings = 0;
        int locations = 0;

        for (auto &vboIdx : combo) {
            BufferCreateInfo *vbo = info.vbos[vboIdx];

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = bindings;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            // Calculate offsets from dataTypes
            uint thisOffset = 0;
            std::vector<uint> theseOffsets;
            theseOffsets.push_back(0);

            // Calculate atribute locations and sizes
            for (auto &dType : vbo->dataTypes) {
                VkFormat thisFormat = dataFormats[static_cast<uint>(dType)];

                VkVertexInputAttributeDescription attributeDescription{};
                attributeDescription.binding = bindings;
                attributeDescription.location = locations;
                attributeDescription.format = thisFormat;
                attributeDescription.offset = thisOffset;
                attrib->attributes.push_back(attributeDescription);

                locations += ((thisFormat == VK_FORMAT_R64G64B64_SFLOAT) || (thisFormat == VK_FORMAT_R64G64B64A64_SFLOAT)) ? 2 : 1;
                thisOffset = dataSizes[static_cast<uint>(dType)];
                theseOffsets.push_back(thisOffset);
            }

            bindingDescription.stride = std::accumulate(theseOffsets.cbegin(), theseOffsets.cend(), 0);
            attrib->bindings.push_back(bindingDescription);
            bindings++;
        }
    }
}

/**
 * @brief Stages and copies a buffer.
 *
 * Stages and copies a buffer from a host-visible staging buffer to a device-local
 * buffer. The buffer is created with the appropriate usage flags for the given
 * BufferType.
 *
 * @param[in] buffer The device-local buffer to copy to
 * @param[in] bufferMemory The device memory for the buffer
 * @param[in] type The type of buffer to create
 * @param[in] bufSize The size of the buffer in bytes
 * @param[in] bufData The data to copy into the buffer
 * @param[in] create If true, creates the buffer and allocates memory for it
 */
void ProgramVK::stageAndCopyBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory, BufferType type, VKuint64 bufSize, const void *bufData, bool create) {
    VkBufferUsageFlags usage = ((type == BufferType::VERTEX || type == BufferType::DATA) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_INDEX_BUFFER_BIT) | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    createBuffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), p_stagingBuffer, p_stagingMemory);

    void* data;
    this->p_vdf->vkMapMemory(this->p_dev, p_stagingMemory, 0, bufSize, 0, &data);
    memcpy(data, bufData, bufSize);
    this->p_vdf->vkUnmapMemory(this->p_dev, p_stagingMemory);

    if (create) {
        createBuffer(bufSize, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
    }

    copyBuffer(buffer, p_stagingBuffer, bufSize);

    this->p_vdf->vkDestroyBuffer(this->p_dev, p_stagingBuffer, nullptr);
    this->p_vdf->vkFreeMemory(this->p_dev, p_stagingMemory, nullptr);
}

/**
 * @brief Creates persistent uniform buffers for all uniforms in the program.
 *
 * Creates persistent uniform buffers for all uniforms in the program. This
 * function is called once when the program is first initialized.
 *
 * The function iterates over all of the shaders in the program and adds all of
 * the uniforms to the descriptor sets, uniform buffers, and push constant ranges.
 * It will also create a mapping from the uniform or push constant name to the
 * index in the p_setLayouts and p_pushConstRanges vectors, respectively.
 *
 * The function will also create a descriptor pool with the required number of
 * sets, and create a descriptor set for each set in the pool. The descriptor
 * sets will be stored in the p_descSets vector.
 *
 * The function will also create a uniform buffer for each uniform, and map it so
 * that the host can access it. The uniform buffers will be stored in the
 * p_uniformBuffers and p_uniformBuffersMemory vectors.
 *
 * The function will also create a push constant range for each push constant, and
 * store it in the p_pushConstRanges vector.
 *
 * Finally, the function will create a mapping from the uniform or push constant
 * name to the index in the p_uniformBufferMappings and p_pushConstMappings
 * vectors, respectively.
 */
void ProgramVK::createPersistentUniformBuffers() {
    std::vector<VKuint> sets;
    std::vector<VKuint> bindings;
    std::vector<VKuint> sizes;

    for (const auto &s : this->p_compiledShaders) {
        if (s->getType() == GL_VERTEX_SHADER) {
            for (const auto &uni : s->getUniforms()) {
                if (this->p_mapDescriptors.find(uni.name) == this->p_mapDescriptors.end()) {
                    uint j = uni.set;
                    sets.push_back(j);
                    bindings.push_back(uni.binding);
                    sizes.push_back(uni.size);

                    this->p_mapDescriptors[uni.name] = j;
                    assert(this->p_setLayouts.size() == j);
                    
                    this->p_setLayouts.push_back({});
                    createDescriptorSetLayout(uni.binding);
                    
                    for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                        createBuffer(uni.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), this->p_uniformBuffers[i][j], this->p_uniformBuffersMemory[i][j]);
                        this->p_vdf->vkMapMemory(this->p_dev, this->p_uniformBuffersMemory[i][j], 0, uni.size, 0, &this->p_uniformBufferMappings[i][j]);
                    }
                } else {
                    std::cout << "Uniform " << uni.name << " already exists in program.\n";
                }
            }
        }
    }

    VKuint setCount = sets.size();
    createDescriptorPool(setCount);
    this->p_descSets.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    this->p_uniformBufferMappings.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        this->p_descSets[i].resize(setCount, {});
        this->p_uniformBuffers[i].resize(setCount, {});
        this->p_uniformBuffersMemory[i].resize(setCount, {});
        this->p_uniformBufferMappings[i].resize(setCount, {});
    }

    for (uint i = 0; i < setCount; i++) {
        createDescriptorSets(sets[i], bindings[i], sizes[i]);
    }
}

/**
 * @brief Create a descriptor set for a given set and binding.
 *
 * This function will allocate a descriptor set from the descriptor pool, and
 * update the descriptor set with the uniform buffer at the given binding.
 *
 * @param set - the set to use for the descriptor set
 * @param binding - the binding to use for the descriptor set
 * @param size - the size of the uniform buffer
 */
void ProgramVK::createDescriptorSets(VKuint set, VKuint binding, VKuint size) {
    for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->p_descPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &this->p_setLayouts[set];

        if (this->p_vdf->vkAllocateDescriptorSets(this->p_dev, &allocInfo, &this->p_descSets[j][set]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = this->p_uniformBuffers[j][set];
        bufferInfo.offset = 0;
        bufferInfo.range = size;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = this->p_descSets[j][set];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        this->p_vdf->vkUpdateDescriptorSets(this->p_dev, 1, &descriptorWrite, 0, nullptr);
    }
}

/**
 * @brief Create a descriptor pool for a given number of bindings.
 *
 * This function will create a descriptor pool for the given number of bindings.
 * The descriptor pool will be created with the type of descriptor
 * VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, and the descriptor count set to
 * MAX_FRAMES_IN_FLIGHT. The pool size count is set to 1, and the pool size is
 * set to the given number of bindings. The flags are set to 0.
 *
 * @param bindings - the number of bindings for the descriptor pool
 *
 * @throws std::runtime_error if the descriptor pool could not be created
 */
void ProgramVK::createDescriptorPool(VKuint bindings) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * bindings;
    poolInfo.flags = 0;

    if (this->p_vdf->vkCreateDescriptorPool(this->p_dev, &poolInfo, nullptr, &this->p_descPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

/**
 * @brief Creates a descriptor set layout for a single uniform buffer.
 *
 * This function creates a descriptor set layout with a single binding for a
 * uniform buffer. The binding is set to the given value, and the descriptor
 * type is set to VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER. The descriptor count is
 * set to 1, and the stage flags are set to VK_SHADER_STAGE_VERTEX_BIT.
 *
 * The created descriptor set layout is stored in the p_setLayouts vector.
 *
 * @param binding - the binding of the uniform buffer
 *
 * @throws std::runtime_error if the descriptor set layout could not be created
 */
void ProgramVK::createDescriptorSetLayout(VKuint binding) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = binding;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    this->p_setLayouts.push_back({});
    if (this->p_vdf->vkCreateDescriptorSetLayout(this->p_dev, &layoutInfo, nullptr, &this->p_setLayouts.back()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

/**
 * @brief Updates a buffer with the given data.
 *
 * This function updates a buffer with the given name by calling
 * _updateBuffer with the given parameters. The buffer's index is found
 * using the p_mapBuffers map, and the buffer info and model are found
 * using the p_mapBufferToModel map. The buffer's type is found in the
 * buffer info.
 *
 * @param bufferName - the name of the buffer to update
 * @param bufferOffset - the offset of the data in the buffer
 * @param bufferCount - the number of elements in the data
 * @param bufferSize - the size of the data in bytes
 * @param bufferData - a pointer to the data to update the buffer with
 */
void ProgramVK::updateBuffer(std::string bufferName, VKuint64 bufferOffset, VKuint64 bufferCount, VKuint64 bufferSize, const void *bufferData) {
    VKuint idx = this->p_mapBuffers[bufferName];
    BufferCreateInfo *bufferInfo = this->p_buffersInfo[idx];
    ModelInfo *model = this->p_models[this->p_mapBufferToModel[bufferName]];
    const BufferType type = bufferInfo->type;

    this->_updateBuffer(idx, bufferInfo, model, type, bufferOffset, bufferCount, bufferSize, bufferData);
}

/**
 * @brief Updates a buffer with the given data.
 *
 * This function updates a buffer with the given buffer name by calling
 * _updateBuffer with the given parameters. The buffer's index is found
 * using the p_mapBuffers map, and the buffer info and model are found
 * using the p_mapBufferToModel map. The buffer's type is found in the
 * buffer info.
 *
 * @param info - the info for the buffer to update, containing the buffer name,
 * buffer offset, buffer count, buffer size, and buffer data.
 */
void ProgramVK::updateBuffer(BufferUpdateInfo &info) {
    VKuint idx = this->p_mapBuffers[info.bufferName];
    BufferCreateInfo *bufferInfo = this->p_buffersInfo[idx];
    ModelInfo *model = this->p_models[this->p_mapBufferToModel[info.bufferName]];
    const BufferType type = info.type;
    VKuint64 offset = info.offset;
    VKuint64 count = info.count;
    VKuint64 size = info.size;
    const void *data = info.data;

    this->_updateBuffer(idx, bufferInfo, model, type, offset, count, size, data);
}

/**
 * @brief Updates a buffer with the given data.
 *
 * This function updates a buffer with the given index by calling
 * stageAndCopyBuffer with the given parameters. The buffer's info and model
 * are found using the p_mapBuffers map and the buffer type is found in the
 * buffer info.
 *
 * If the buffer has not been created yet (i.e. bufferInfo->data == 0), then
 * the buffer is created and the model's valid flags are set. If the buffer
 * has already been created, but the new size is larger than the old size,
 * then the buffer is recreated and the model's valid flags are set. If the
 * buffer has already been created and the new size is smaller than the old
 * size, then the buffer is updated in place.
 *
 * @param idx - the index of the buffer to update
 * @param bufferInfo - the info for the buffer to update, containing the buffer name,
 * buffer offset, buffer count, buffer size, and buffer data
 * @param model - the model that the buffer belongs to
 * @param type - the type of the buffer (VERTEX, INDEX, or DATA)
 * @param offset - the offset of the data in the buffer
 * @param count - the number of elements in the data
 * @param size - the size of the data in bytes
 * @param data - a pointer to the data to update the buffer with
 */
void ProgramVK::_updateBuffer(const VKuint idx, BufferCreateInfo *bufferInfo, ModelInfo *model, const BufferType type, const VKuint64 offset, const VKuint64 count, const VKuint64 size, const void *data) {
    bool isVBO = (type == BufferType::VERTEX || type == BufferType::DATA);
    bool isIBO = (type == BufferType::INDEX);
    
    if (!bufferInfo->data) {
        // Model was pre-declared and needs to be updated for initialization
        bufferInfo->count = count;
        bufferInfo->size = size;
        bufferInfo->data = data;

        this->stageAndCopyBuffer(this->p_buffers[idx], this->p_buffersMemory[idx], type, size, data);

        if (isIBO) {
            for (auto &prog : model->programs) {
                for (auto &renderIdx : prog.offsets) {
                    model->renders[renderIdx]->indexOffset = offset;
                    model->renders[renderIdx]->indexCount = count;
                }
            }
            model->valid.ibo = true;
        } else if (isVBO) {
            model->valid.vbo = true;
        }

    } else {
        if (bufferInfo->size >= size) {
            // Model was already initialized, and buffer is large enough to update in place            
            if (data != 0) {
                bufferInfo->count = count;
                bufferInfo->size = size;
                bufferInfo->data = data;
                this->stageAndCopyBuffer(this->p_buffers[idx], this->p_buffersMemory[idx], type, size, data, false);
            }
            
        } else {
            // Model was already initialized, but buffer needs to be recreated to fit new size
            VKuint frame = this->p_vkw->currentSwapChainImageIndex();

            bufferInfo->count = count;
            bufferInfo->size = size;
            bufferInfo->data = data;

            VKuint zombieIdx = 0;
            if (this->p_buffersFree.size()) {
                zombieIdx = this->p_buffersFree.front();
                this->p_buffersFree.pop_front();
            }

            VKuint newIdx = 0;
            if (zombieIdx) {
                newIdx = zombieIdx;
                this->p_buffersInfo[newIdx] = new BufferCreateInfo(*bufferInfo);
            } else {
                newIdx = this->p_buffers.size();
                this->p_buffers.push_back({});
                this->p_buffersMemory.push_back({});
                this->p_buffersInfo.push_back(new BufferCreateInfo(*bufferInfo));
            }
            this->p_buffersInfo[newIdx]->id = newIdx;
            this->p_mapBuffers[bufferInfo->name] = newIdx;
            delete this->p_buffersInfo[idx];
            this->p_buffersInfo[idx] = nullptr;

            this->stageAndCopyBuffer(this->p_buffers[newIdx], this->p_buffersMemory[newIdx], type, size, data);
            
            if (isVBO) {
                std::replace(model->vbos.begin(), model->vbos.end(), idx, newIdx);
            } else if (isIBO) {
                model->ibo = newIdx;
            }

            this->p_mapZombieIndices[frame].push_back(idx);
        }

        if (isIBO) {
            if (model->activePrograms.size() != 0) {
                for (auto &prog : model->activePrograms) {
                    for (auto &renderIdx : model->programs[prog].offsets) {
                        model->renders[renderIdx]->indexOffset = offset;
                        model->renders[renderIdx]->indexCount = count;
                    }
                }
            } else {
                for (auto &render : model->renders) {
                    render->indexOffset = offset;
                    render->indexCount = count;
                }
            }
        }
    }
}

/**
 * @brief Update the contents of a uniform buffer object (UBO) on the GPU.
 *
 * @param currentImage The index of the current swap chain image.
 * @param uboName The name of the UBO to update.
 * @param uboSize The size of the UBO in bytes.
 * @param uboData A pointer to the data to copy into the UBO.
 *
 * The function looks up the index of the UBO in the descriptor set and then
 * copies the data from the host to the UBO on the GPU using the mapping stored
 * in p_uniformBufferMappings.
 */
void ProgramVK::updateUniformBuffer(uint32_t currentImage, std::string uboName, uint32_t uboSize, const void *uboData) {
    VKuint uboIdx = this->p_mapDescriptors[uboName];

    void *dest = this->p_uniformBufferMappings[currentImage][uboIdx];
    memcpy(dest, uboData, uboSize);
}

/**
 * @brief Update the contents of a push constant.
 *
 * @param name The name of the push constant to update.
 * @param data A pointer to the data to copy into the push constant.
 * @param size The size of the push constant in bytes.
 *
 * The function looks up the index of the push constant in the descriptor set and
 * then sets the data and range for that push constant.
 */
void ProgramVK::updatePushConstant(std::string name, const void *data, uint32_t size) {
    VKuint pid = this->p_mapPushConsts[name];
    this->p_pushConsts[pid].second = data;
    
    if (size) {
        // Update existing PCR with new size, data, and Range
        VkPushConstantRange *pcr = &this->p_pushConstRanges[pid];
        pcr->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcr->offset = 0;
        pcr->size = size;

        this->p_pushConsts[pid].first = size;
        this->p_pushConsts[pid].second = data;
    }
}

/**
 * @brief Update the clear color of the program.
 *
 * The clear color is used to fill the color attachment of the framebuffer
 * before drawing. The clear color is specified in 4-component floating point
 * format, with the first component being red, the second component being green,
 * the third component being blue, and the fourth component being alpha.
 *
 * @param r The red component of the clear color.
 * @param g The green component of the clear color.
 * @param b The blue component of the clear color.
 * @param a The alpha component of the clear color.
 */
void ProgramVK::updateClearColor(float r, float g, float b, float a) {
    p_clearColor[0] = r;
    p_clearColor[1] = g;
    p_clearColor[2] = b;
    p_clearColor[3] = a;
}

/**
 * @brief Update the swap chain extent.
 *
 * The swap chain extent is used to determine the size of the swap chain images
 * when the swap chain is recreated. The extent is specified in pixels, with the
 * first component being the width and the second component being the height.
 *
 * @param x The width of the swap chain extent.
 * @param y The height of the swap chain extent.
 */
void ProgramVK::updateSwapExtent(int x, int y) {
    this->p_swapExtent.width = x;
    this->p_swapExtent.height = y;
}

/**
 * @brief Renders the scene to the swap chain image.
 *
 * This function renders the scene to the swap chain image specified by the
 * current swap chain image index. It uses the render pass and frame buffer
 * specified by the current swap chain image, and the command buffer specified
 * by the current command buffer index. It also clears the color and depth
 * buffers to the specified values.
 * 
 * Rendering is done by iterating over the models in the active models list,
 * then iterating over the active programs for each model, and finally iterating
 * over the described renders ("offsets") for each program.
 *
 * @param renderExtent The extent of the swap chain image.
 */
void ProgramVK::render(VkExtent2D &renderExtent) {
    VKuint image = this->p_vkw->currentSwapChainImageIndex();
    VkCommandBuffer cmdBuff = this->p_vkw->currentCommandBuffer();

    // Set clear color and depth stencil
    VkClearColorValue clearColor = { { p_clearColor[0], p_clearColor[1], p_clearColor[2], p_clearColor[3] } };
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
    renderPassInfo.clearValueCount = (this->p_vkw->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT) ? 3 : 2;
    renderPassInfo.pClearValues = clearValues;
    
    this->p_vdf->vkCmdBeginRenderPass(cmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // For each active program for each active model, bind and draw for all render targets
    for (auto &modelIdx : this->p_activeModels) {
        ModelInfo *model = this->p_models[modelIdx];
        if (model->valid.suspended) {
            continue;
        }

        for (auto &prog : model->activePrograms) {
            for (auto &renderIdx : model->programs[prog].offsets) {
                RenderInfo *render = model->renders[renderIdx];
                std::vector<VkBuffer> renderVbos;
                for (auto &vbo : render->vbos) {
                    renderVbos.push_back(this->p_buffers[model->vbos[vbo]]);
                }

                this->p_vdf->vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, render->pipeline);
                this->p_vdf->vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, this->p_pipeLayouts[model->pipeLayouts[render->pipeLayoutIndex]], 0, this->p_descSets[image].size(), this->p_descSets[image].data(), 0, nullptr);                
                this->p_vdf->vkCmdSetViewport(cmdBuff, 0, 1, &this->p_viewport);
                this->p_vdf->vkCmdSetScissor(cmdBuff, 0, 1, &this->p_scissor);
                this->p_vdf->vkCmdBindVertexBuffers(cmdBuff, 0, render->vbos.size(), renderVbos.data(), render->vboOffsets.data());
                this->p_vdf->vkCmdBindIndexBuffer(cmdBuff, this->p_buffers[model->ibo], 0, VK_INDEX_TYPE_UINT32);
                if (render->pushConst >= 0) {
                    this->p_vdf->vkCmdPushConstants(cmdBuff, this->p_pipeLayouts[model->pipeLayouts[render->pipeLayoutIndex]], VK_SHADER_STAGE_VERTEX_BIT, 0, this->p_pushConsts[render->pushConst].first, this->p_pushConsts[render->pushConst].second);
                }
                this->p_vdf->vkCmdDrawIndexed(cmdBuff, render->indexCount, 1, render->indexOffset, 0, 0);
            }
        }
    }
    
    // Cleanup
    this->p_vdf->vkCmdEndRenderPass(cmdBuff);
}

/**
 * ReapZombies is a function that is called once per frame to clean up any
 * buffers that are no longer in use. It checks the current swap chain image
 * index against the map of zombie indices, and if the indices no longer match,
 * it deletes the buffer and memory associated with the index, and adds the
 * index to the list of free indices.
 */
void ProgramVK::reapZombies() {
    VKuint frame = this->p_vkw->currentSwapChainImageIndex();
    if (!this->p_mapZombieIndices.size()) {
        return;
    }

    for (auto &frameIdx : this->p_mapZombieIndices) {
        if (frameIdx.first == frame) {
            continue;
        }

        for (auto &idx : frameIdx.second) {
            this->p_vdf->vkDestroyBuffer(this->p_dev, this->p_buffers[idx], nullptr);
            this->p_vdf->vkFreeMemory(this->p_dev, this->p_buffersMemory[idx], nullptr);
            this->p_buffers[idx] = VK_NULL_HANDLE;
            this->p_buffersMemory[idx] = VK_NULL_HANDLE;
            this->p_buffersFree.push_back(idx);
        }

        frameIdx.second.clear();
    }
}

/**
 * @brief Retrieves a pointer to a Shader object associated with a filename.
 *
 * @param fileName - std::string representation of the shader filename
 * @return Shader* - pointer to the Shader object associated with the filename
 * @throws std::runtime_error if shader is not found
 */
Shader* ProgramVK::getShaderFromName(const std::string& fileName) {
    assert(p_stage >= 2);
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

/**
 * @brief Retrieves a pointer to a Shader object associated with a shader id.
 *
 * @param id - uint32_t representation of the shader id
 * @return Shader* - pointer to the Shader object associated with the shader id
 * @throws std::runtime_error if shader is not found
 */
Shader* ProgramVK::getShaderFromId(VKuint id) {
    if (id >= p_compiledShaders.size()) {
        throw std::runtime_error("Shader with id " + std::to_string(id) + " not found.");
    }

    return p_compiledShaders[id];
}

/**
 * Retrieves a shader id associated with a filename.
 * @param fileName - std::string representation of the shader filename
 * @return uint32_t - shader id associated with the filename
 * @throws std::runtime_error if shader is not found
 */
VKuint ProgramVK::getShaderIdFromName(const std::string& fileName) {
    VKuint id = uint(-1);
    
    if (p_mapShaders.find(fileName) == p_mapShaders.end()) {
        std::cout << "Shader not found: " << fileName << std::endl;
    } else {
        id = p_mapShaders.at(fileName);
        if (id >= p_compiledShaders.size()) {
            std::cout << "Invalid shader id: " << id << ". Shader not found: " << fileName << std::endl;
        }
    }

    return id;
}

/**
 * Retrieves a pointer to a ModelInfo object associated with a model name.
 *
 * @param name - std::string representation of the model name
 * @return ModelInfo* - pointer to the ModelInfo object associated with the model name
 * @throws std::runtime_error if model is not found
 */
ModelInfo* ProgramVK::getModelFromName(const std::string& name) {
    VKint id = getModelIdFromName(name);

    if (id == -1) {
        throw std::runtime_error("Model " + name + " not found.");
    }

    return p_models[id];
}

/**
 * Retrieves the model id associated with a model name.
 *
 * @param name - std::string representation of the model name
 * @return VKint - model id associated with the model name, -1 if model is not found
 */
VKint ProgramVK::getModelIdFromName(const std::string& name) {
    VKint id = -1;
    
    if (p_mapModels.find(name) == p_mapModels.end()) {
        std::cout << "Model not found: " << name << std::endl;
    } else {
        id = p_mapModels.at(name);
        if (id >= (int) p_models.size()) {
            std::cout << "Invalid model id: " << id << ". Model not found: " << name << std::endl;
        }
    }

    return id;
}

/**
 * Retrieves a set of the ids of all active models.
 *
 * @return std::set<VKuint> - set of active model ids
 */
std::set<VKuint> ProgramVK::getActiveModelsById() {
    return p_activeModels;
}

/**
 * Retrieves a vector of the names of all active models.
 *
 * @return std::vector<std::string> - vector of active model names
 */
std::vector<std::string> ProgramVK::getActiveModelsByName() {
    std::vector<std::string> names;

    for (VKuint id : p_activeModels) {
        names.push_back(p_models[id]->name);
    }

    return names;
}

/**
 * Retrieves a set of the ids of all active programs for a model.
 *
 * @param modelName - the name of the model to retrieve programs for
 * @return std::set<VKuint> - set of active program ids
 */
std::set<VKuint> ProgramVK::getModelActivePrograms(std::string modelName) {
    VKuint id = getModelIdFromName(modelName);
    return p_models[id]->activePrograms;
}

/**
 * Checks if a model and program are active.
 *
 * @param modelName - the name of the model
 * @param program - the id of the program
 * @return true if the model and program are active, false otherwise
 */
bool ProgramVK::isActive(const std::string &modelName, VKuint program) {
    bool active = false;
    VKint id = getModelIdFromName(modelName);

    if (id >= 0) {
        active = (p_activeModels.end() != std::find(p_activeModels.cbegin(), p_activeModels.cend(), id));
        if (active) {
            active = (p_models[id]->activePrograms.end() != std::find(p_models[id]->activePrograms.cbegin(), p_models[id]->activePrograms.cend(), program));
        }
    }

    return active;
}

/**
 * Prints information about a model.
 *
 * @param model - the model to print information for
 */
void ProgramVK::printModel(ModelInfo *model) {
    std::cout << "Model: " << model->id << "\n";
    
    for (const auto &vboIdx : model->vbos) {
        BufferCreateInfo *vbo = this->p_buffersInfo[vboIdx];
        std::cout << "    VBO: " << vbo->name << "\n";
    }

    if (!model->renders.size()) {
        std::cout << "    No renders." << "\n";
    } else {
        for (uint i = 0; i < model->renders.size(); i++) {
            std::cout << "    Render " << i << ": " << "\n";
            std::cout << "        IBO Offset : " << model->renders[i]->indexOffset << "\n";
            std::cout << "        Index Count: " << model->renders[i]->indexCount << "\n";
        }
    }

    std::cout << std::endl;
}

/**
 * Prints information about a model creation information object.
 *
 * This function prints information about a ModelCreateInfo object, including
 * its name, vertex buffer objects, index buffer object, shaders, and offsets.
 *
 * @param info - the ModelCreateInfo object to print information for
 */
void ProgramVK::printInfo(ModelCreateInfo *info) {
    std::cout << "\nInfo: " << info->name << "\n";
    
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

    std::cout << "    Shaders: " <<  "\n";
    for (auto &s : info->vertShaders) {
        std::cout << "        " << s << "\n";
    }
    for (auto &s : info->fragShaders) {
        std::cout << "        " << s << "\n";
    }

    std::cout << "    Offsets: " << "\n";
    for (uint i = 0; i < info->offsets.size(); i++) {
        auto &offset = info->offsets[i];
        std::cout << "        [" << i << "]: Offset         : " << offset.offset << "\n";
        std::cout << "             Vertex Shader  : " << info->vertShaders[offset.vertShaderIndex] << "\n";
        std::cout << "             Fragment Shader: " << info->fragShaders[offset.fragShaderIndex] << "\n";
        std::cout << "             Topology       : " << topologyNames[info->topologies[offset.topologyIndex]] << "\n";
    }

    std::cout << std::endl;
}
