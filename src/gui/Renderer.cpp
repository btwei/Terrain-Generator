#include "tg/Renderer.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <nfd.hpp>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

namespace tg {

void Renderer::init(SDL_Window* window, char* argv0) {
    _window = window;
    executablePath = argv0;

    initVulkan();
    createSwapchain();
    initGUI();
    createViewportResources();
    initDefaultGeometry();
}

void Renderer::handleEvent(const SDL_Event& event) {
    // Forward event to ImGui
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
        case SDL_EVENT_QUIT:
            _isRunning = false;
            break;
        default:
            break;
    }   
}

void Renderer::update() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    float menuBarHeight = 0.0f;
    if(ImGui::BeginMainMenuBar()) {
        menuBarHeight = ImGui::GetFrameHeight();
        if(ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Export as .R16")) {

            }
            if(ImGui::MenuItem("Export as OBJ")) {

            }
            if(ImGui::MenuItem("Quit")) {
                _isRunning = false;
            }
        ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View")) {
            if(ImGui::MenuItem("Reset")) {
                distance = 4.0f;
                yaw = glm::radians(45.0f);
                pitch = glm::radians(30.0f);
                panOffset = glm::vec2(0.0f, 0.0f);
            }
        ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("About")) {
                ImGui::OpenPopup("AboutPopup");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((ImGui::GetIO().DisplaySize.x * ImGui::GetIO().DisplayFramebufferScale.x - _mainViewportWidth) / ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    ImGui::Begin("Left Panel", nullptr, panelFlags);
        ImGui::Text("Terrain Generation Settings");
        ImGui::Separator();

        ImGui::InputInt("Size", &selectedSize);

        if(ImGui::CollapsingHeader("Generation Method")){
            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.55f, 0.55f, 0.60f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.65f, 0.65f, 0.70f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.75f, 0.75f, 0.80f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.55f, 0.55f, 0.60f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.65f, 0.65f, 0.70f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.75f, 0.75f, 0.80f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45, 0.45, 0.45, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.37f, 0.33f, 0.40f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.70f, 0.70f, 0.75f, 1.0f));

            if(ImGui::BeginCombo("Method", methodNames[selectedMethod])) {
                for(int i=0; i < 3; i++) {
                    bool isSelected = (selectedMethod == i);
                    if(ImGui::Selectable(methodNames[i], isSelected)) {
                        selectedMethod = i;
                    }
                    if(isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if(selectedMethod == 0) {
                ImGui::Indent();
                if(ImGui::CollapsingHeader("Perlin Noise Parameters")) {
                    ImGui::InputInt("Grid Size", &perlinGridSize);
                }
                ImGui::Unindent();
            } else if(selectedMethod == 1) {
                ImGui::Indent();
                if(ImGui::CollapsingHeader("Diamond Square Parameters")) {
                    ImGui::InputFloat("Roughness", &diamondSquareRoughness);
                }
                ImGui::Unindent();
            } else if(selectedMethod == 2) {
                ImGui::Indent();
                if(ImGui::CollapsingHeader("Faulting Parameters")){
                    ImGui::InputInt("Iterations", &faultingIterations);
                }
                ImGui::Unindent();
            }

            ImGui::PopStyleColor(10);
        }

        if(ImGui::CollapsingHeader("Post Processing Filters")) {
            
            ImGui::Checkbox("Thermal Weathering", &shouldThermalWeather);
            ImGui::Indent();
                if(ImGui::CollapsingHeader("Thermal Parameters")) {
                    ImGui::InputInt("Iterations", &thermalIterations);
                    ImGui::InputFloat("Talus Slope", &thermalThreshold);
                    ImGui::InputFloat("Scaling constant", &thermalConstant);
                    thermalConstant = glm::clamp(thermalConstant, 0.0f, 1.0f);
                }
            ImGui::Unindent();
        }

        ImGui::Spacing();

        bool shouldGenerate = ImGui::Button("Generate");
        
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x * ImGui::GetIO().DisplayFramebufferScale.x - _mainViewportWidth) / ImGui::GetIO().DisplayFramebufferScale.x, menuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(_mainViewportWidth / ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    panelFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    bool viewportHovered;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Main Viewport", nullptr, panelFlags);
        ImGui::Image(getCurrentFrame()._GUIdescriptorSet, ImVec2(_mainViewportExtent.width / ImGui::GetIO().DisplayFramebufferScale.x, _mainViewportExtent.height / ImGui::GetIO().DisplayFramebufferScale.y));

        viewportHovered = ImGui::IsWindowHovered();
    ImGui::End();
    ImGui::PopStyleVar();

    if(ImGui::BeginPopupModal("AboutPopup")) {
        ImGui::Text("terrainGenerator");
        ImGui::Separator();
        ImGui::TextWrapped("Version 1.0.0\n(c) 2025 Benjamin Wei\n");
        ImGui::Spacing();
        ImGui::TextWrapped("This application uses third-party libraries:");
        ImGui::BulletText("Dear ImGui (MIT)");
        ImGui::BulletText("GLM (MIT)");
        ImGui::BulletText("nativefiledialog-extended (zlib)");
        ImGui::BulletText("SDL (zlib)");
        ImGui::BulletText("vk-bootstrap (MIT)");
        ImGui::BulletText("Vulkan Memory Allocator (MIT)");
        ImGui::Spacing();

        ImGui::Separator();
        ImGui::Text("Licenses:");
        ImGui::BeginChild("LicensesScroll", ImVec2(500, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

        // --- Dear ImGui License ---
        ImGui::Text("Dear ImGui - MIT License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Copyright (c) 2014-2025 Omar Cornut\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy "
            "of this software and associated documentation files (the \"Software\"), to deal "
            "in the Software without restriction, including without limitation the rights "
            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
            "copies of the Software, and to permit persons to whom the Software is "
            "furnished to do so, subject to the following conditions:\n\n"
            "The above copyright notice and this permission notice shall be included in all "
            "copies or substantial portions of the Software.\n\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND..."
        );

        ImGui::Spacing();

        // --- GLM License (MIT) ---
        ImGui::Text("GLM - MIT License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Copyright (c) 2005-2025 G-Truc Creation\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy..."
        );

        ImGui::Spacing();

        // --- nativefiledialog-extended License (zlib) ---
        ImGui::Text("nativefiledialog-extended - zlib License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "This software is provided 'as-is', without any express or implied warranty. "
            "In no event will the authors be held liable for any damages arising from the "
            "use of this software...\n"
            "Permission is granted to anyone to use this software for any purpose, including "
            "commercial applications, and to alter it and redistribute it freely..."
        );

        ImGui::Spacing();

        // --- SDL License (zlib) ---
        ImGui::Text("SDL - zlib License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "This software is provided 'as-is', without any express or implied warranty... "
            "Permission is granted to anyone to use this software for any purpose..."
        );

        ImGui::Spacing();

        // --- vk-bootstrap License (MIT) ---
        ImGui::Text("vk-bootstrap - MIT License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Copyright (c) 2019-2025 The vk-bootstrap Authors\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy..."
        );

        ImGui::Spacing();

        // --- Vulkan Memory Allocator License (MIT) ---
        ImGui::Text("Vulkan Memory Allocator - MIT License");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Copyright (c) 2017-2025 Advanced Micro Devices, Inc.\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy..."
        );

        ImGui::EndChild();

        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Handle Terrain Generation
    if(shouldGenerate) {
        if(selectedMethod == 0) {
            _currentHeightmap = generatePerlinNoiseHeightmap(selectedSize, selectedSize, perlinGridSize);
        } else if(selectedMethod == 1) {
            _currentHeightmap = generateDiamondSquareHeightmap(selectedSize, selectedSize, diamondSquareRoughness);
        } else if(selectedMethod == 2) {
            _currentHeightmap = generateFaultingHeightmap(selectedSize, selectedSize, faultingIterations);
        }

        if(shouldThermalWeather) {
            applyThermalWeathering(_currentHeightmap, thermalThreshold, thermalConstant, thermalIterations);
        }

        Mesh mesh = convertHeightmapToMesh(_currentHeightmap);

        vkDeviceWaitIdle(_device);
        vmaDestroyBuffer(_allocator, _vertexBuffer.buffer, _vertexBuffer.allocation);
        vmaDestroyBuffer(_allocator, _indexBuffer.buffer, _indexBuffer.allocation);

        // Upload index and vertex buffers 
        _vertexBuffer = uploadToNewDeviceLocalBuffer(mesh.interleavedAttributes.size() * sizeof(Attributes), mesh.interleavedAttributes.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        _indexBuffer = uploadToNewDeviceLocalBuffer(mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        // Reset view parameters, in case user gets lost or something
        distance = 4.0f;
        yaw = glm::radians(45.0f);
        pitch = glm::radians(30.0f);
        panOffset = glm::vec2(0.0f, 0.0f);
    }

    // Handle Mouse IO
    if(viewportHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;

        float rotation_sensitivity = 0.005f;

        yaw += delta.x * rotation_sensitivity;
        pitch += delta.y * rotation_sensitivity;

        pitch = glm::clamp(pitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);
    }

    if(viewportHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || viewportHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        float pan_sensitivity = 0.01f;

        panOffset.x += delta.x * pan_sensitivity;
        panOffset.y -= delta.y * pan_sensitivity;
    }

    if(viewportHovered) {
        float zoom_sensitivity = 0.1f;
        distance -= ImGui::GetIO().MouseWheel * zoom_sensitivity;
        distance = glm::max(distance, 0.1f);
    }

    glm::vec3 forward;
    forward.x = cos(pitch) * sin(yaw);
    forward.y = sin(pitch);
    forward.z = cos(pitch) * cos(yaw);
    forward = glm::normalize(forward);
    
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    target = right * panOffset.x + up * panOffset.y;

    glm::vec3 cameraPos = target - forward * distance;
    V_matrix = glm::lookAt(cameraPos, target, glm::vec3(0, -1, 0));

    glm::mat4 MVP = P_matrix * V_matrix * M_matrix;
    memcpy(getCurrentFrame().uboData, &MVP, sizeof(glm::mat4));

    memcpy(static_cast<char*>(getCurrentFrame().uboData) + sizeof(glm::mat4), &normal_matrix, sizeof(glm::mat4));
}

// @todo: Move some of these functions to a separate helper function header + implementation file
void Renderer::render() {
    if(vkWaitForFences(_device, 1, &getCurrentFrame().renderFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for fence!");
    }

    uint32_t imageIndex;
    VkResult acquireImageResult = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, getCurrentFrame().imageAcquireToRenderSemaphore, VK_NULL_HANDLE, &imageIndex);
    if(acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        // Perhaps in the future I will improve the responsiveness of window resizing by redesigning the swapchain recreation logic
        vkDeviceWaitIdle(_device);
        destroySwapchain();
        createSwapchain();
        destroyViewportResources();
        createViewportResources();
        return;
    } else if(acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    if(vkResetFences(_device, 1, &getCurrentFrame().renderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset fence!");
    }

    VkCommandBuffer buf = getCurrentFrame()._mainCommandBuffer;
    if(vkResetCommandBuffer(buf, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to reset command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if(vkBeginCommandBuffer(buf, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer!");
    }

        // Record main commands
        recordMainCommands(buf, imageIndex);

    vkEndCommandBuffer(buf);

    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = getCurrentFrame().imageAcquireToRenderSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = _renderToPresentSemaphores[imageIndex];
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = buf;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.commandBufferInfoCount = 1;

    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;

    if(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, getCurrentFrame().renderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit main command buffer!");
    }

    // Submit Queue Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderToPresentSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &imageIndex;

    // @todo: Does vkbootstrap initialize the presentQueue to be exclusive or concurrent? Potential issue. Will test later.
    VkResult presentResult = vkQueuePresentKHR(_presentQueue, &presentInfo);

    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        vkDeviceWaitIdle(_device);
        destroySwapchain();
        createSwapchain();
        destroyViewportResources();
        createViewportResources();
    } else if(presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    _frameCount++;
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(_device);

    // Cleanup VMA
    vmaDestroyBuffer(_allocator, _vertexBuffer.buffer, _vertexBuffer.allocation);
    vmaDestroyBuffer(_allocator, _indexBuffer.buffer, _indexBuffer.allocation);

    for(int i=0; i < NUM_FRAME_OVERLAP; i++) {
        vmaUnmapMemory(_allocator, _frames[i]._uboBuffer.allocation);
        vmaDestroyBuffer(_allocator, _frames[i]._uboBuffer.buffer, _frames[i]._uboBuffer.allocation);
        //vkFreeDescriptorSets(_device, _descriptorPool, 1, &_frames[i]._descriptorSet);
        vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
        vkDestroyFence(_device, _frames[i].renderFence, nullptr);
        vkDestroySemaphore(_device, _frames[i].imageAcquireToRenderSemaphore, nullptr);
    }

    vmaDestroyAllocator(_allocator);

    for(int i=0; i < _renderToPresentSemaphores.size(); i++) {
        vkDestroySemaphore(_device, _renderToPresentSemaphores[i], nullptr);
    }

    destroyViewportResources();

    // Cleanup ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    destroySwapchain();

    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    vkDestroySampler(_device, _sampler, nullptr);
    vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);

    vkDestroyCommandPool(_device, _commandPool, nullptr);

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
    vkDestroyInstance(_instance, nullptr);
}

void Renderer::initVulkan() {
    // Enable validation layers if in debug mode
#ifndef NDEBUG
    bool bRequestValidationLayers = true;
#else
    bool bRequestValidationLayers = false;
#endif

    // Use VkBootstrap to create an instance and debug messenger
    vkb::InstanceBuilder instanceBuilder;
    vkb::Result<vkb::Instance> instanceResult = instanceBuilder.set_app_name("Terrain Generator")
        .request_validation_layers(bRequestValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(VK_API_VERSION_1_3)
        .build();

    vkb::Instance vkbInstance = instanceResult.value();
    _instance = vkbInstance.instance;
    _debugMessenger = vkbInstance.debug_messenger;

    // Create a surface
    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    // Select a Physical device; create a logical device and queues
    VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
    physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    physicalDeviceVulkan13Features.dynamicRendering = VK_TRUE;
    physicalDeviceVulkan13Features.synchronization2 = VK_TRUE;

    vkb::PhysicalDeviceSelector selector{ vkbInstance };
    vkb::Result<vkb::PhysicalDevice> physicalDeviceResult = selector
        .set_minimum_version(1, 3)
        .set_surface(_surface)
        .set_required_features_13(physicalDeviceVulkan13Features)
        .select();

    vkb::PhysicalDevice vkbPhysicalDevice = physicalDeviceResult.value();
    _physicalDevice = vkbPhysicalDevice.physical_device;

    vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();
    _device = vkbDevice.device;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    _presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();

    // Create General Command Pool
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = _graphicsQueueFamily;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    // Initialize VMA
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;

    vmaCreateAllocator(&allocatorInfo, &_allocator);

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    if(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    VkDescriptorPoolSize uboSize{};
    uboSize.descriptorCount = static_cast<uint32_t>(NUM_FRAME_OVERLAP);
    uboSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(NUM_FRAME_OVERLAP);
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &uboSize;

    if(vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool) != VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool!");

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 0.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if(vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) throw std::runtime_error("Failed to create sampler!");

    createGraphicsPipeline();

    // Create command pools, command buffers, fences, semaphores for each frame
    for(int i=0; i < NUM_FRAME_OVERLAP; i++) {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = _graphicsQueueFamily;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_frames[i]._commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = _frames[i]._commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        if(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &_frames[i]._mainCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer!");
        }

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i].renderFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence!");
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i].imageAcquireToRenderSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore!");
        }

        // Create descriptor set
        VkDescriptorSetAllocateInfo descriptorAllocInfo{};
        descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorAllocInfo.descriptorPool = _descriptorPool;
        descriptorAllocInfo.descriptorSetCount = 1;
        descriptorAllocInfo.pSetLayouts = &_descriptorSetLayout;

        if(vkAllocateDescriptorSets(_device, &descriptorAllocInfo, &_frames[i]._descriptorSet) != VK_SUCCESS) throw std::runtime_error("Failed to allocate descriptor set!");

        _frames[i]._uboBuffer = createBuffer(2 * sizeof(glm::mat4), VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        vmaMapMemory(_allocator, _frames[i]._uboBuffer.allocation, &_frames[i].uboData);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _frames[i]._uboBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4);

        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = _frames[i]._descriptorSet;
        writeSet.dstBinding = 0;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSet.descriptorCount = 1;
        writeSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(_device, 1, &writeSet, 0, nullptr);
    }
}

void Renderer::createSwapchain() {
    vkb::SwapchainBuilder swapchainBuilder{ _physicalDevice, _device, _surface };

    int w, h;
    SDL_GetWindowSizeInPixels(_window, &w, &h);

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(static_cast<uint32_t>(w), static_cast<uint32_t>(h))
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        .build()
        .value();

    _swapchainExtent = vkbSwapchain.extent;
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImageFormat = vkbSwapchain.image_format;

    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

    while(_renderToPresentSemaphores.size() < _swapchainImages.size()) {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        if(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore!");
        }

        _renderToPresentSemaphores.push_back(semaphore);
    }
}

void Renderer::destroySwapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    for(int i=0; i < _swapchainImageViews.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}

/**
 * @brief Creates Viewport image resources for each frame in flight
 * @note requires _swapchainExtent to be set first
 */
void Renderer::createViewportResources() {
    _mainViewportExtent = VkExtent2D(_swapchainExtent.width * 0.65, _swapchainExtent.height);
    _mainViewportWidth = static_cast<float>(_swapchainExtent.width) * 0.65f; // @todo: cleanup duplicate variable

    P_matrix = glm::perspective(glm::radians(45.0f), static_cast<float>(_mainViewportExtent.width) / _mainViewportExtent.height, 0.1f, 100.0f);
    P_matrix[1][1] *= -1;

    for(int i=0; i < NUM_FRAME_OVERLAP; i++) {
        // Create the color attachment image for the main viewport
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        imageCreateInfo.extent.width = _swapchainExtent.width * 0.65;
        imageCreateInfo.extent.height = _swapchainExtent.height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if(vkCreateImage(_device, &imageCreateInfo, nullptr, &_frames[i].mainViewportImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create main viewport color attachment image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, _frames[i].mainViewportImage, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        uint32_t deviceLocalMemoryTypeIndex = 0;
        for(uint32_t i=0; i < memProperties.memoryTypeCount; i++) {
            if(memRequirements.memoryTypeBits & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                deviceLocalMemoryTypeIndex = i;
                break;
            }
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

        if(vkAllocateMemory(_device, &allocInfo, nullptr, &_frames[i].mainViewportImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate main viewport color attachment image memory!");
        }

        vkBindImageMemory(_device, _frames[i].mainViewportImage, _frames[i].mainViewportImageMemory, 0);

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = _frames[i].mainViewportImage;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &_frames[i].mainViewportImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create main viewport color attachment image view!");
        }

        // Create the depth attachment image for the main viewport

        imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
        imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if(vkCreateImage(_device, &imageCreateInfo, nullptr, &_frames[i].mainViewportDepthImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create main viewport depth attachment image!");
        }

        vkGetImageMemoryRequirements(_device, _frames[i].mainViewportDepthImage, &memRequirements);

        for(uint32_t i=0; i < memProperties.memoryTypeCount; i++) {
            if(memRequirements.memoryTypeBits & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                deviceLocalMemoryTypeIndex = i;
                break;
            }
        }

        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

        if(vkAllocateMemory(_device, &allocInfo, nullptr, &_frames[i].mainViewportDepthImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate main viewport depth attachment image memory!");
        }

        vkBindImageMemory(_device, _frames[i].mainViewportDepthImage, _frames[i].mainViewportDepthImageMemory, 0);

        imageViewCreateInfo.image = _frames[i].mainViewportDepthImage;
        imageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &_frames[i].mainViewportDepthImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create main viewport depth attachment image view!");
        }

        // Transition Depth attachment to depth attachment optimal
        VkFence copyFence;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        
        if(vkCreateFence(_device, &fenceInfo, nullptr, &copyFence) != VK_SUCCESS) throw std::runtime_error("Failed to create temp fence!");

        VkCommandBuffer buf;
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = _commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        if(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &buf) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(buf, &beginInfo);

            VkImageMemoryBarrier viewportBarrierFromUndefinedToDepthAttachment{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = _frames[i].mainViewportDepthImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }};

            vkCmdPipelineBarrier(
                buf,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &viewportBarrierFromUndefinedToDepthAttachment
            );

        vkEndCommandBuffer(buf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buf;

        vkQueueSubmit(_graphicsQueue, 1, &submitInfo, copyFence);

        vkWaitForFences(_device, 1, &copyFence, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(_device, _commandPool, 1, &buf);
        vkDestroyFence(_device, copyFence, nullptr);

        _frames[i]._GUIdescriptorSet = ImGui_ImplVulkan_AddTexture(_sampler, _frames[i].mainViewportImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void Renderer::destroyViewportResources() {
    for(int i=0; i < NUM_FRAME_OVERLAP; i++) {
        ImGui_ImplVulkan_RemoveTexture(_frames[i]._GUIdescriptorSet);

        vkDestroyImageView(_device, _frames[i].mainViewportImageView, nullptr);
        vkDestroyImage(_device, _frames[i].mainViewportImage, nullptr);
        vkFreeMemory(_device, _frames[i].mainViewportImageMemory, nullptr);

        vkDestroyImageView(_device, _frames[i].mainViewportDepthImageView, nullptr);
        vkDestroyImage(_device, _frames[i].mainViewportDepthImage, nullptr);
        vkFreeMemory(_device, _frames[i].mainViewportDepthImageMemory, nullptr);
    }
}

void Renderer::initGUI() {
    VkFormat colorAttachmentFormats[] = { VK_FORMAT_B8G8R8A8_UNORM };

    VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingCreateInfo.viewMask = 0;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
    renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(_window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_VERSION_1_3;
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _physicalDevice;
    init_info.Device = _device;
    init_info.QueueFamily = _graphicsQueueFamily;
    init_info.Queue = _graphicsQueue;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.RenderPass = VK_NULL_HANDLE;

    init_info.MinImageCount = _swapchainImages.size();
    init_info.ImageCount = _swapchainImages.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.Subpass = 0;

    init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + NUM_FRAME_OVERLAP;

    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = renderingCreateInfo;

    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.MinAllocationSize = 1024*1024;

    ImGui_ImplVulkan_Init(&init_info);
}

void Renderer::initDefaultGeometry() {
    _currentHeightmap = generateFlatHeightmap(512, 512);
    Mesh mesh = convertHeightmapToMesh(_currentHeightmap);

    _vertexBuffer = uploadToNewDeviceLocalBuffer(mesh.interleavedAttributes.size() * sizeof(Attributes), mesh.interleavedAttributes.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    _indexBuffer = uploadToNewDeviceLocalBuffer(mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    glm::vec3 translation(-0.5f, -0.5f, -0.5f);
    
    // This turns the model to be flat
    glm::mat4 swapYZ(1.0f);
    swapYZ[0] = glm::vec4(1, 0, 0, 0);
    swapYZ[1] = glm::vec4(0, 0, 1, 0);
    swapYZ[2] = glm::vec4(0, 1, 0, 0);

    // Set the model matrix once; no need to update it later
    // All models will start from X, Y ∈ [0, 1]
    M_matrix = glm::translate(glm::mat4(1.0f), translation) * swapYZ;

    normal_matrix = glm::transpose(glm::inverse(M_matrix));
}

void Renderer::generateUserGeometry() {

}

void Renderer::recordMainCommands(VkCommandBuffer& commandBuffer, int imageIndex) {
    // Render into mainViewport
    VkImageMemoryBarrier viewportBarrierFromUndefinedToColorAttachment{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = getCurrentFrame().mainViewportImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
    }};

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &viewportBarrierFromUndefinedToColorAttachment
    );

    VkClearValue viewportClearValue{};
    viewportClearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    viewportClearValue.depthStencil.depth = 1.0f;

    VkRenderingAttachmentInfo viewportColorAttachment{};
    viewportColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    viewportColorAttachment.imageView = getCurrentFrame().mainViewportImageView;
    viewportColorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    viewportColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    viewportColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    viewportColorAttachment.clearValue = { {0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderingAttachmentInfo viewportDepthAttachment{};
    viewportDepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    viewportDepthAttachment.imageView = getCurrentFrame().mainViewportDepthImageView;
    viewportDepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    viewportDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    viewportDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    viewportDepthAttachment.clearValue = viewportClearValue;

    VkRenderingInfo viewportRenderingInfo{};
    viewportRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    viewportRenderingInfo.renderArea.offset = {0, 0};
    viewportRenderingInfo.renderArea.extent = _mainViewportExtent;
    viewportRenderingInfo.layerCount = 1;
    viewportRenderingInfo.colorAttachmentCount = 1;
    viewportRenderingInfo.pColorAttachments = &viewportColorAttachment;
    viewportRenderingInfo.pDepthAttachment = &viewportDepthAttachment;

    vkCmdBeginRendering(commandBuffer, &viewportRenderingInfo);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) _mainViewportExtent.width;
        viewport.height = (float) _mainViewportExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _mainViewportExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {_vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &getCurrentFrame()._descriptorSet, 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>((_currentHeightmap.height - 1) * (_currentHeightmap.width - 1) * 6), 1, 0, 0, 0);

    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier viewportBarrierFromColorAttachmentToShaderOptimal{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = getCurrentFrame().mainViewportImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
    }};

    // Transition the viewport image to be used during the fragment shader later
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT ,
        0,
        0, nullptr,
        0, nullptr,
        1, &viewportBarrierFromColorAttachmentToShaderOptimal);

    // Render DearImGui onto swapchain image

    VkImageMemoryBarrier barrierFromUndefinedToColorAttachment{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = _swapchainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }};

    // Transition the swapchain image to a color attachment optimal layout
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrierFromUndefinedToColorAttachment
    );

    // Begin the main render pass
    VkClearValue clearValue{};
    clearValue.color = { {0.1f, 0.1f, 0.2f, 1.0f} };

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = _swapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValue;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = _swapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier barrierFromColorAttachmentToPresent{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = _swapchainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }};

    // Transition the swapchain image to a presentable layout
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrierFromColorAttachmentToPresent);

}

VkShaderModule Renderer::createShaderModule(const char* filename) {
    std::filesystem::path relativeRoot(executablePath);
    std::ifstream file(relativeRoot.parent_path().string() + std::string("/shaders_spv/") + filename, std::ios::ate | std::ios::binary);

    if(!file.is_open()) throw std::runtime_error("Failed to open shader file!");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    shaderInfo.codeSize = fileSize;

    if(vkCreateShaderModule(_device, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS) throw std::runtime_error("Failed to create shader module!");
    return shaderModule;
}

/**
 * @note For the moment, there is only one pipeline, making management easy
 */
void Renderer::createGraphicsPipeline() {
    VkShaderModule defaultShaderModule = createShaderModule("default.spv");

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = defaultShaderModule;
    vertexStageInfo.pName = "mainVert";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = defaultShaderModule;
    fragStageInfo.pName = "mainFrag";

    VkPipelineShaderStageCreateInfo stages[] = { vertexStageInfo, fragStageInfo };

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDesc.stride = (3 + 3 + 2) * sizeof(float);

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    // float3 / vec3 for position - x, y, z
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    // float3 / vec3 for normal - x, y, z
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 3 * sizeof(float);

    // float2 / vec2 for uv - u, v
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 6 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStageInfo{};
    viewportStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStageInfo.viewportCount = 1;
    viewportStageInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &renderingInfo;
    graphicsPipelineInfo.stageCount = 2;
    graphicsPipelineInfo.pStages = stages;
    graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    graphicsPipelineInfo.pViewportState = &viewportStageInfo;
    graphicsPipelineInfo.pRasterizationState = &rasterizationStateInfo;
    graphicsPipelineInfo.pMultisampleState = &multisampleState;
    graphicsPipelineInfo.pDepthStencilState = &depthStencilState;
    graphicsPipelineInfo.pColorBlendState = &colorBlendState;
    graphicsPipelineInfo.pDynamicState = &dynamicState;
    graphicsPipelineInfo.layout = _pipelineLayout;
    graphicsPipelineInfo.renderPass = VK_NULL_HANDLE; // Dynamic Rendering is required
    graphicsPipelineInfo.subpass = 0; 
    graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(_device, defaultShaderModule, nullptr);
}

Renderer::Buffer Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) {
    Buffer buf;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = flags;

    if(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, nullptr) != VK_SUCCESS){
        throw std::runtime_error("Failed to create VMA buffer!");
    }

    return buf;
}

Renderer::Buffer Renderer::uploadToNewDeviceLocalBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage) {
    Buffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    void* hostMemPtr;
    vmaMapMemory(_allocator, stagingBuffer.allocation, &hostMemPtr);
    memcpy(hostMemPtr, data, (size_t)size);
    vmaUnmapMemory(_allocator, stagingBuffer.allocation);

    Buffer deviceLocalBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, 0);

    // Copy from host visible to device local buffer
    VkFence copyFence;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    if(vkCreateFence(_device, &fenceInfo, nullptr, &copyFence) != VK_SUCCESS) throw std::runtime_error("Failed to create temp fence!");

    VkCommandBuffer buf;
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = _commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &buf) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(buf, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(buf, stagingBuffer.buffer, deviceLocalBuffer.buffer, 1, &copyRegion);

    vkEndCommandBuffer(buf);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buf;

    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, copyFence);

    vkWaitForFences(_device, 1, &copyFence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(_device, _commandPool, 1, &buf);
    vkDestroyFence(_device, copyFence, nullptr);

    vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

    return deviceLocalBuffer;
}

} // namespace tg
