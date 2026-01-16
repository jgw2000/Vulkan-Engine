#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

class GLFWwindow;

namespace VE::Gfx {

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

class VulkanContext {
public:
    VulkanContext(void* window);
    ~VulkanContext();

    void Render();

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createSwapchain();
    void allocateCommandBuffers();
    void createSyncObjects();
    void createGraphicsPipeline();
    
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

    void recordCommandBuffer(uint32_t imageIndex);

    void recreateSwapchain();

private:
    vk::raii::Context mContext;
    vk::raii::Instance mInstance = nullptr;
    vk::raii::PhysicalDevice mPhysicalDevice = nullptr;
    vk::raii::Device mDevice = nullptr;
    vk::raii::Queue mGraphicsQueue = nullptr;
    vk::raii::SurfaceKHR mSurface = nullptr;
    vk::raii::SwapchainKHR mSwapchain = nullptr;
    vk::raii::CommandPool mCommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> mCommandBuffers;
    std::vector<vk::raii::Semaphore> mRenderFinishedSemaphores;
    std::vector<vk::raii::Semaphore> mPresentCompleteSemaphores;
    std::vector<vk::raii::Fence> mDrawFences;
    
    GLFWwindow* mWindow = nullptr;
    vk::SurfaceFormatKHR mSwapFormat;
    vk::Extent2D mSwapExtent;
    std::vector<vk::Image> mSwapchainImages;
    std::vector<vk::raii::ImageView> mSwapchainImageViews;

    uint32_t mFrameIndex = 0;

    vk::raii::Pipeline mGraphicsPipeline = nullptr;
    vk::raii::PipelineLayout mPipelineLayout = nullptr;
};

}