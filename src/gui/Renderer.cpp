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

void Renderer::render() {
    
}

void Renderer::cleanup() {
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
}

void Renderer::destroySwapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    for(int i=0; i < _swapchainImageViews.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}

} // namespace tg
