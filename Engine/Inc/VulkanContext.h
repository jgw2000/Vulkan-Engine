#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace VE {

class VulkanContext {
public:
    VulkanContext(void* window);

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSurface(void* window);
    void createSwapchain(void* window);

private:
    vk::raii::Context mContext;
    vk::raii::Instance mInstance = nullptr;
    vk::raii::PhysicalDevice mPhysicalDevice = nullptr;
    vk::raii::Device mDevice = nullptr;
    vk::raii::Queue mGraphicsQueue = nullptr;
    vk::raii::SurfaceKHR mSurface = nullptr;
    vk::raii::SwapchainKHR mSwapchain = nullptr;
    
    vk::SurfaceFormatKHR mSwapFormat;
    vk::Extent2D mSwapExtent;
    std::vector<vk::Image> mSwapchainImages;
    std::vector<vk::raii::ImageView> mSwapchainImageViews;
};

}