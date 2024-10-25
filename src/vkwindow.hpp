/**
 * vkwindow.hpp
 *
 *    Created on: Oct 22, 2024
 *   Last Update: Oct 22, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023, 2024 Wade Burch (GPLv3)
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

#ifndef VKWINDOW_H
#define VKWINDOW_H

#include <QVulkanWindow>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QFutureWatcher>
#include <QFuture>
#include <QPromise>
#include <QtConcurrent/QtConcurrent>
#include <QOpenGLFunctions_4_5_Core>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <ranges>
// #include "programVK.hpp"
#include "quaternion.hpp"
#include "wavemanager.hpp"
#include "cloudmanager.hpp"

class VKWindow : public QVulkanWindow {

public:
    VKWindow(QWidget *parent = nullptr, ConfigParser *configParser = nullptr);
    ~VKWindow();

    VKRenderer* createRenderer() override;

signals:

public slots:

protected:

private:
};

class VKRenderer : public QVulkanWindowRenderer {

public:
    VKRenderer(VKWindow *vkWin);
    ~VKRenderer();

    void initResources() override;
    void initSwapChainResources() override;
    // void logicalDeviceLost() override;
    // void physicalDeviceLost() override;
    // void preInitResources() override;
    void releaseResources() override;
    void releaseSwapChainResources() override;
    void startNextFrame() override;

    VkShaderModule createShader(const std::string &name);

private:
    VKWindow *vkw = nullptr;
    VkDevice dev;
    QVulkanInstance *vi = nullptr;
    QVulkanFunctions *vf = nullptr;
    QVulkanDeviceFunctions *vdf = nullptr;

    VkDeviceMemory vr_bufMem = VK_NULL_HANDLE;
    VkBuffer vr_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo vr_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool vr_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout vr_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet vr_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache vr_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout vr_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline vr_pipeline = VK_NULL_HANDLE;

    glm::mat4 m4_proj;
    glm::mat4 m4_view;
    glm::mat4 m4_world;
    glm::mat4 m4_rotation;
    glm::mat4 m4_translation;
};

#endif
