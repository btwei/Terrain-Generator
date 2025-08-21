#ifndef TG_RENDERER_HPP
#define TG_RENDERER_HPP

#include <vector>

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

namespace tg {

constexpr unsigned int NUM_FRAME_OVERLAP = 2;

class Renderer {
public:
    void init(SDL_Window* window);
    bool isRunning() const { return _isRunning; }
    void handleEvent(const SDL_Event& event);
    void update();
    void render();
    void cleanup();

private:
    struct DataPerFrame {
        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;
    };

    SDL_Window* _window;
    bool _isRunning = true;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;

    VkSurfaceKHR _surface;

    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    uint32_t _graphicsQueueFamily;
    VkQueue _graphicsQueue;
    
    VkQueue presentQueue;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    uint32_t _frameCount = 0;
    DataPerFrame _frames[NUM_FRAME_OVERLAP];
    std::vector<VkSemaphore> _renderToPresentSemaphores;

    void initVulkan();
    void createSwapchain();
    void destroySwapchain();

    DataPerFrame& getCurrentFrame() { return _frames[_frameCount % NUM_FRAME_OVERLAP]; }

};

} // namespace tg

#endif // TG_RENDERER_HPP