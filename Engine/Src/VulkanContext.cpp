#include "VulkanContext.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VE {

std::vector<const char*> deviceExtensions = {
    vk::KHRSwapchainExtensionName
};

// -----------------------------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------------------------
bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    auto deviceProperties = physicalDevice.getProperties();
    auto deviceFeatures = physicalDevice.getFeatures();

    if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
    }

    if (deviceProperties.apiVersion < vk::ApiVersion14) {
        return false;
    }

    return true;
}

uint32_t FindQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice, vk::QueueFlags queueFlags) {
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // Get the first index into queueFamilyProperties thich supports
    auto queueFamilyProperty = std::find_if(
        queueFamilyProperties.begin(),
        queueFamilyProperties.end(),
        [queueFlags](const auto& qfp) { return (qfp.queueFlags & queueFlags) == queueFlags; }
    );

    return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), queueFamilyProperty));

}

// -----------------------------------------------------------------------------------------------
// VulkanContext
// -----------------------------------------------------------------------------------------------
VulkanContext::VulkanContext() {
    createInstance();
    selectPhysicalDevice();
    createLogicalDevice();
}

void VulkanContext::createInstance() {
    constexpr vk::ApplicationInfo appInfo {
        .pApplicationName = "Vulkan Engine",
        .pEngineName = "VE",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    // Check if the required layers are supported by the Vulkan implementation.
    std::vector<const char*> requiredLayers;
#ifndef NDEBUG
    requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    
    auto layerProperties = mContext.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(requiredLayers, [&layerProperties](const auto& requiredLayer) {
        return std::ranges::none_of(layerProperties,
            [requiredLayer](const auto& layerProperty) {
                return strcmp(layerProperty.layerName, requiredLayer) == 0;
            });
        })) {
        throw std::runtime_error("One or more required layers are not supported!");
    }

    // Check if the required GLFW extensions are supported by the Vulkan implementation.
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto extensionProperties = mContext.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        if (std::ranges::none_of(extensionProperties,
                                 [glfwExtension = glfwExtensions[i]](const auto& extensionProperty)
                                 { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; })) {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
        }
    }

    vk::InstanceCreateInfo createInfo {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions
    };

    mInstance = vk::raii::Instance(mContext, createInfo);
}

void VulkanContext::selectPhysicalDevice() {
    auto devices = mInstance.enumeratePhysicalDevices();
    if (devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    const auto devIter = std::ranges::find_if(devices,
        [&](const auto& device) {
            bool isSuitable = IsDeviceSuitable(device);
            if (isSuitable) {
                mPhysicalDevice = device;
            }
            return isSuitable;
        }
    );

    if (devIter == devices.end()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanContext::createLogicalDevice() {
    uint32_t graphicsQueueFamilyIndex = FindQueueFamilies(mPhysicalDevice, vk::QueueFlagBits::eGraphics);
    float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    // Create a chain of feature structures
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features> featureChain = {
        {},
        { .dynamicRendering = true },   // Enable dynamic rendering from Vulkan 1.3
    };

    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    mDevice = vk::raii::Device(mPhysicalDevice, deviceCreateInfo);
    mGraphicsQueue = vk::raii::Queue(mDevice, graphicsQueueFamilyIndex, 0);
}

}