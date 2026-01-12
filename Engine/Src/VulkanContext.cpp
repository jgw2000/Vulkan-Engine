#include "VulkanContext.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
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

vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& mode : availablePresentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, void* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(window), &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

// -----------------------------------------------------------------------------------------------
// VulkanContext
// -----------------------------------------------------------------------------------------------
VulkanContext::VulkanContext(void* window) {
    createInstance();
    selectPhysicalDevice();
    createLogicalDevice();
    createSurface(window);
    createSwapchain(window);
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

void VulkanContext::createSurface(void* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*mInstance, static_cast<GLFWwindow*>(window), nullptr, &surface)) {
        throw std::runtime_error("failed to create window surface!");
    }

    mSurface = vk::raii::SurfaceKHR(mInstance, surface);
}

void VulkanContext::createSwapchain(void* window) {
    auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(*mSurface);
    mSwapFormat = ChooseSurfaceFormat(mPhysicalDevice.getSurfaceFormatsKHR(*mSurface));
    mSwapExtent = ChooseSwapExtent(surfaceCapabilities, window);

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < imageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo {
        .flags = {},
        .surface = *mSurface,
        .minImageCount = imageCount,
        .imageFormat = mSwapFormat.format,
        .imageColorSpace = mSwapFormat.colorSpace,
        .imageExtent = mSwapExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = ChooseSwapPresentMode(mPhysicalDevice.getSurfacePresentModesKHR(*mSurface)),
        .clipped = true,
        .oldSwapchain = nullptr
    };

    mSwapchain = vk::raii::SwapchainKHR(mDevice, swapchainCreateInfo);
    mSwapchainImages = mSwapchain.getImages();

    // Create image views
    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType = vk::ImageViewType::e2D,
        .format = mSwapFormat.format,
        .components = {},
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    for (auto image : mSwapchainImages) {
        imageViewCreateInfo.image = image;
        mSwapchainImageViews.emplace_back(mDevice, imageViewCreateInfo);
    }
}

}