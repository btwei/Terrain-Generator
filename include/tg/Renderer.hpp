#ifndef TG_RENDERER_HPP
#define TG_RENDERER_HPP

#include <vector>

#include <SDL3/SDL.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "tg/generator.hpp"

namespace tg {

constexpr unsigned int NUM_FRAME_OVERLAP = 2;

/**
 * @class Renderer
 * @brief Manages Vulkan rendering for the application, including swapchain, command buffers, and GUI integration.
 */
class Renderer {
public:
    void init(SDL_Window* window, char* argv0);
    bool isRunning() const { return _isRunning; }
    void handleEvent(const SDL_Event& event);
    void update();
    void render();
    void cleanup();

private:
    struct DataPerFrame {
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;

        VkFence renderFence;
        VkSemaphore imageAcquireToRenderSemaphore;

        VkImage mainViewportImage;
        VkImageView mainViewportImageView;
        VkDeviceMemory mainViewportImageMemory;

        VkImage mainViewportDepthImage;
        VkImageView mainViewportDepthImageView;
        VkDeviceMemory mainViewportDepthImageMemory;
    };

    struct Buffer {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    SDL_Window* _window;
    bool _isRunning = true;
    char* executablePath = nullptr;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;

    VkSurfaceKHR _surface;

    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    uint32_t _graphicsQueueFamily;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    VkCommandPool _commandPool;
    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool _descriptorPool;

    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    VmaAllocator _allocator;
    Buffer _vertexBuffer;
    Buffer _indexBuffer;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    uint32_t _mainViewportWidth;
    Heightmap _currentHeightmap;

    uint32_t _frameCount = 0;
    DataPerFrame _frames[NUM_FRAME_OVERLAP];
    std::vector<VkSemaphore> _renderToPresentSemaphores;

    void initVulkan();
    void createSwapchain();
    void destroySwapchain();
    void createViewportResources();
    void destroyViewportResources();
    void initGUI();
    void initDefaultGeometry();
    void generateUserGeometry();
    void recordMainCommands(VkCommandBuffer& commandBuffer, int imageIndex);

    VkShaderModule createShaderModule(const char* filename);
    void createGraphicsPipeline();
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);
    Buffer uploadToNewDeviceLocalBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage);

    DataPerFrame& getCurrentFrame() { return _frames[_frameCount % NUM_FRAME_OVERLAP]; }

    int selectedSize = 512;
    int selectedMethod = 0;
    const char* methodNames[3] = { "Perlin Noise", "Diamond-Square", "Fault Formation" };
};

} // namespace tg

#endif // TG_RENDERER_HPP