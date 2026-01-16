#include <Graphics/VulkanContext.h>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VE::Gfx {

std::vector<const char*> deviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSynchronization2ExtensionName
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

std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

void TransitionImageLayout(
    vk::raii::CommandBuffer& cmd, vk::Image image,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    cmd.pipelineBarrier2(dependencyInfo);
}

// -----------------------------------------------------------------------------------------------
// VulkanContext
// -----------------------------------------------------------------------------------------------
VulkanContext::VulkanContext(void* window) : mWindow(static_cast<GLFWwindow*>(window)) {
    createInstance();
    selectPhysicalDevice();
    createLogicalDevice();
    createSurface();
    createSwapchain();
    allocateCommandBuffers();
    createSyncObjects();
    createGraphicsPipeline();
}

VulkanContext::~VulkanContext() {
    mDevice.waitIdle();
}

void VulkanContext::Render() {
    auto fenceResult = mDevice.waitForFences(*mDrawFences[mFrameIndex], vk::True, UINT64_MAX);
    auto [result, imageIndex] = mSwapchain.acquireNextImage(UINT64_MAX, *mPresentCompleteSemaphores[mFrameIndex]);
    
    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    mDevice.resetFences(*mDrawFences[mFrameIndex]);

    mCommandBuffers[mFrameIndex].reset();
    recordCommandBuffer(imageIndex);

    // Submit the command buffer
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*mPresentCompleteSemaphores[mFrameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*mCommandBuffers[mFrameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*mRenderFinishedSemaphores[imageIndex]
    };
    mGraphicsQueue.submit(submitInfo, *mDrawFences[mFrameIndex]);

    // Presentation
    try {
        const vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*mRenderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*mSwapchain,
            .pImageIndices = &imageIndex
        };
        result = mGraphicsQueue.presentKHR(presentInfo);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            recreateSwapchain();
            return;
        }
        else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapchain();
            return;
        }
        else {
            throw;
        }
    }

    mFrameIndex = (mFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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
        { .synchronization2 = true, .dynamicRendering = true },   // Enable synchronization2 and dynamic rendering from Vulkan 1.3
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

    // Create command pool
    vk::CommandPoolCreateInfo poolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };

    mCommandPool = vk::raii::CommandPool(mDevice, poolInfo);
}

void VulkanContext::createSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &surface)) {
        throw std::runtime_error("failed to create window surface!");
    }

    mSurface = vk::raii::SurfaceKHR(mInstance, surface);
}

void VulkanContext::createSwapchain() {
    auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(*mSurface);
    mSwapFormat = ChooseSurfaceFormat(mPhysicalDevice.getSurfaceFormatsKHR(*mSurface));
    mSwapExtent = ChooseSwapExtent(surfaceCapabilities, mWindow);

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

void VulkanContext::allocateCommandBuffers() {
    mCommandBuffers.clear();

    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool = mCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    mCommandBuffers = vk::raii::CommandBuffers(mDevice, allocInfo);
}

void VulkanContext::createSyncObjects() {
    mPresentCompleteSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mDrawFences.clear();

    for (size_t i = 0; i < mSwapchainImages.size(); ++i) {
        mRenderFinishedSemaphores.emplace_back(mDevice, vk::SemaphoreCreateInfo());
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        mPresentCompleteSemaphores.emplace_back(mDevice, vk::SemaphoreCreateInfo());
        mDrawFences.emplace_back(mDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
}

void VulkanContext::createGraphicsPipeline() {
    vk::raii::ShaderModule shaderModule = createShaderModule(ReadFile("Assets/Shader/triangle.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    /////////////////////////////// Fixed functions /////////////////////////////////////
    // Dynamic state
    std::vector dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Vertex input
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    // Viewport and scissors
    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1,
        .scissorCount = 1
    };

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    // Depth and stencil testing
    vk::PipelineDepthStencilStateCreateInfo depthStencil {

    };

    // Color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // Pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };
    mPipelineLayout = vk::raii::PipelineLayout(mDevice, pipelineLayoutInfo);

    // Dynamic rendering
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &mSwapFormat.format
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = mPipelineLayout,
        .renderPass = nullptr
    };

    mGraphicsPipeline = vk::raii::Pipeline(mDevice, nullptr, pipelineInfo);
}

[[nodiscard]] vk::raii::ShaderModule VulkanContext::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo smCreateInfo {
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    return vk::raii::ShaderModule(mDevice, smCreateInfo);
}

void VulkanContext::recordCommandBuffer(uint32_t imageIndex) {
    auto& cmd = mCommandBuffers[mFrameIndex];

    cmd.begin({});

    TransitionImageLayout(
        cmd, mSwapchainImages[imageIndex],
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        {}, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = mSwapchainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = mSwapExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    // Drawing
    cmd.beginRendering(renderingInfo);
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(mSwapExtent.width), static_cast<float>(mSwapExtent.height)));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), mSwapExtent));
    cmd.draw(3, 1, 0, 0);

    cmd.endRendering();

    TransitionImageLayout(
        cmd, mSwapchainImages[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite, {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    cmd.end();
}

void VulkanContext::recreateSwapchain() {
    // Handle window minimization
    int width = 0, height = 0;
    do {
        glfwWaitEvents();
        glfwGetFramebufferSize(mWindow, &width, &height);
    } while (width == 0 || height == 0);

    mDevice.waitIdle();
    mSwapchainImageViews.clear();
    mSwapchain = nullptr;

    createSwapchain();
}

}