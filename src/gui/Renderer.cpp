#include "tg/Renderer.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

namespace tg
{

void Renderer::init(SDL_Window* window) {
    _window = window;

    initVulkan();
    createSwapchain();
    initGUI();
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

    ImGui::ShowDemoWindow();
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
    if(vkQueuePresentKHR(_presentQueue, &presentInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    _frameCount++;
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(_device);

    // Cleanup ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    for(int i=0; i < NUM_FRAME_OVERLAP; i++) {
        vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
        vkDestroyFence(_device, _frames[i].renderFence, nullptr);
        vkDestroySemaphore(_device, _frames[i].imageAcquireToRenderSemaphore, nullptr);
    }

    for(int i=0; i < _renderToPresentSemaphores.size(); i++) {
        vkDestroySemaphore(_device, _renderToPresentSemaphores[i], nullptr);
    }

    destroySwapchain();

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

    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    // Now I want to select a physical device and create a logical device
    // And for now, I want dynamic rendering and sync2, maybe more later
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

    init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;

    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = renderingCreateInfo;

    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.MinAllocationSize = 1024*1024;

    ImGui_ImplVulkan_Init(&init_info);
}

void Renderer::recordMainCommands(VkCommandBuffer& commandBuffer, int imageIndex) {
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

} // namespace tg
