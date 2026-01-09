#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace VE {

class VulkanContext {
public:
    VulkanContext();

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();

private:
    vk::raii::Context mContext;
    vk::raii::Instance mInstance = nullptr;
    vk::raii::PhysicalDevice mPhysicalDevice = nullptr;
    vk::raii::Device mDevice = nullptr;
    vk::raii::Queue mGraphicsQueue = nullptr;
};

}