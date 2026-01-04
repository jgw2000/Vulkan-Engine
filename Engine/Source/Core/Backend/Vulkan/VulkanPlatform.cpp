#define VOLK_IMPLEMENTATION

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <vector>

#include "VulkanPlatform.h"

#include <Core/Debug.h>
#include <Core/Backend/Vulkan/VulkanConstants.h>
#include <Core/Backend/Vulkan/VulkanContext.h>
#include <Core/Backend/Vulkan/VulkanDriver.h>
#include <Core/Backend/Vulkan/Utils/Definitions.h>
#include <Core/Backend/Vulkan/Utils/Helper.h>
#include <Core/Backend/Vulkan/Utils/Image.h>

namespace VE::backend {

namespace {

constexpr u32 INVALID_VK_INDEX = 0xFFFFFFFF;

using ExtensionSet = VulkanPlatform::ExtensionSet;

inline bool SetContains(const ExtensionSet& set, std::string extension) {
    return set.find(extension) != set.end();
}

#if FVK_ENABLED(FVK_DEBUG_VALIDATION)
const std::string_view DESIRED_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
#if FVK_ENABLED(FVK_DEBUG_DUMP_API)
    "VK_LAYER_LUNARG_api_dump"
#endif
};

std::vector<cstring> GetEnabledLayers() {
    const std::vector<VkLayerProperties> availableLayers = VkEnumerate(vkEnumerateInstanceLayerProperties);
    std::vector<cstring> enabled_layers;

    for (const auto& desired : DESIRED_LAYERS) {
        for (const VkLayerProperties& layer : availableLayers) {
            const std::string_view availableLayer(layer.layerName);
            if (availableLayer == desired) {
                enabled_layers.push_back(desired.data());
                break;
            }
        }
    }
    return enabled_layers;
}
#endif

}

// ------------------------------------------------------------------------------------------------
// Global Functions
// ------------------------------------------------------------------------------------------------

ExtensionSet GetDeviceExtensions(VkPhysicalDevice device) {
    const ExtensionSet TARGET_EXTS = {
#if FVK_ENABLED(FVK_DEBUG_DEBUG_UTILS)
        VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
#endif
    };

    ExtensionSet exts;
    // Identify supported physical device extensions
    const auto extensions = VkEnumerate(vkEnumerateDeviceExtensionProperties, device, static_cast<cstring>(nullptr) /* pLayerName */);
    for (const auto& extension : extensions) {
        std::string name(extension.extensionName);

        if (name.size() == 0) {
            continue;
        }

        if (SetContains(TARGET_EXTS, name)) {
            exts.insert(name);
        }
    }
    return exts;
}

std::tuple<ExtensionSet, ExtensionSet> PruneExtensions(VkPhysicalDevice device,
    const Platform::DriverConfig& driverConfig, const ExtensionSet& instExts,
    const ExtensionSet& deviceExts) noexcept {
    ExtensionSet newInstExts = instExts;
    ExtensionSet newDeviceExts = deviceExts;

#if FVK_ENABLED(FVK_DEBUG_DEBUG_UTILS)
    if (SetContains(newInstExts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) &&
        SetContains(newInstExts, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
        newDeviceExts.erase(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
#endif

#if FVK_ENABLED(FVK_DEBUG_VALIDATION)
    if (SetContains(newInstExts, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) &&
        !SetContains(newInstExts, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
        newDeviceExts.erase(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
#endif

    return std::tuple(newInstExts, newDeviceExts);
}

template<typename StructA, typename StructB>
StructA* ChainStruct(StructA* structA, StructB* structB) {
    structB->pNext = const_cast<void*>(structA->pNext);
    structA->pNext = (void*)structB;
    return structA;
}

u32 IdentifyQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlags flags) {
    u32 queueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);

    if (queueFamiliesCount > 0) {
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesProperties.data());
    }

    u32 queueFamilyIndex = INVALID_VK_INDEX;
    for (u32 j = 0; j < queueFamiliesCount; ++j) {
        VkQueueFamilyProperties props = queueFamiliesProperties[j];
        if (props.queueCount != 0 && props.queueFlags & flags) {
            queueFamilyIndex = j;
            break;
        }
    }

    return queueFamilyIndex;
}

inline int DeviceTypeOrder(VkPhysicalDeviceType deviceType) {
    constexpr std::array<VkPhysicalDeviceType, 5> TYPES = {
        VK_PHYSICAL_DEVICE_TYPE_OTHER,
        VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
    };
    if (auto itr = std::find(TYPES.begin(), TYPES.end(), deviceType); itr != TYPES.end()) {
        return std::distance(TYPES.begin(), itr);
    }
    return -1;
}

VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, const VulkanPlatform::Customization::GPUPreference& gpuPreference) {
    const auto physicalDevices = VkEnumerate(vkEnumeratePhysicalDevices, instance);
    struct DeviceInfo {
        VkPhysicalDevice device = VK_NULL_HANDLE;
        VkPhysicalDeviceType device_type = VK_PHYSICAL_DEVICE_TYPE_OTHER;
        i8 index = -1;
        std::string_view name;
    };
    std::vector<DeviceInfo> deviceList(physicalDevices.size());

    for (sizet deviceInd = 0; deviceInd < physicalDevices.size(); ++deviceInd) {
        const auto candidateDevice = physicalDevices[deviceInd];
        VkPhysicalDeviceProperties targetDeviceProperties;
        vkGetPhysicalDeviceProperties(candidateDevice, &targetDeviceProperties);

        const int major = VK_VERSION_MAJOR(targetDeviceProperties.apiVersion);
        const int minor = VK_VERSION_MINOR(targetDeviceProperties.apiVersion);

        if (major < FVK_REQUIRED_VERSION_MAJOR) {
            continue;
        }
        if (major == FVK_REQUIRED_VERSION_MAJOR && minor < FVK_REQUIRED_VERSION_MINOR) {
            continue;
        }

        // Does the device have any command queues that support graphics?
        // In theory, we should also ensure that the device supports presentation of out
        // particular VkSurface, but we don't have a VkSurface yet, so we'll skip this requirement.
        if (IdentifyQueueFamilyIndex(candidateDevice, VK_QUEUE_GRAPHICS_BIT) == INVALID_VK_INDEX) {
            continue;
        }

        // Does the device support the VK_KHR_swapchain extension?
        const auto extensions = VkEnumerate(vkEnumerateDeviceExtensionProperties, candidateDevice, static_cast<cstring>(nullptr) /* pLayerName */);
        const bool supportsSwapchain = std::any_of(extensions.begin(), extensions.end(), [](const auto& ext) {
                                           return !strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                                       });
        if (!supportsSwapchain) {
            continue;
        }

        deviceList[deviceInd] = {
            .device = candidateDevice,
            .device_type = targetDeviceProperties.deviceType,
            .index = (i8)deviceInd,
            .name = targetDeviceProperties.deviceName
        };
    }

    // Sort the found devices
    std::sort(deviceList.begin(), deviceList.end(),
        [pref = gpuPreference](const DeviceInfo& a, const DeviceInfo& b) {
            if (b.device == VK_NULL_HANDLE) {
                return false;
            }
            if (a.device == VK_NULL_HANDLE) {
                return true;
            }
            if (!pref.device_name.empty()) {
                if (a.name.find(pref.device_name.c_str()) != a.name.npos) {
                    return false;
                }
                if (b.name.find(pref.device_name.c_str()) != b.name.npos) {
                    return true;
                }
            }
            if (pref.index == a.index) {
                return false;
            }
            if (pref.index == b.index) {
                return true;
            }
            return DeviceTypeOrder(a.device_type) < DeviceTypeOrder(b.device_type);
        });
    auto device = deviceList.back().device;
    assert(device != VK_NULL_HANDLE && "Unable to find suitable device.");

    return device;
}

std::vector<VkFormat> FindAttachmentDepthStencilFormats(VkPhysicalDevice device) {
    const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    constexpr VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    std::vector<VkFormat> selectedFormats;
    for (VkFormat format : formats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        if ((props.optimalTilingFeatures & features) == features) {
            selectedFormats.push_back(format);
        }
    }

    return selectedFormats;
}

std::vector<VkFormat> FindBlittableDepthStencilFormats(VkPhysicalDevice device) {
    std::vector<VkFormat> selectedFormats;
    constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;

    for (const VkFormat format : ALL_VK_FORMATS) {
        if (IsVkDepthFormat(format)) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device, format, &props);
            if ((props.optimalTilingFeatures & required) == required) {
                selectedFormats.push_back(format);
            }
        }
    }

    return selectedFormats;
}

bool HasUnifiedMemoryArchitecture(VkPhysicalDeviceMemoryProperties memoryProperties) noexcept {
    for (u32 i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        if ((memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0) {
            return false;
        }
    }
    return true;
}

// ------------------------------------------------------------------------------------------------
// VulkanPlatformPrivate
// ------------------------------------------------------------------------------------------------
struct VulkanPlatformPrivate {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    u32 graphics_queue_family_index = INVALID_VK_INDEX;
    u32 graphics_queue_index = INVALID_VK_INDEX;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    u32 protected_graphics_queue_family_index = INVALID_VK_INDEX;
    u32 protected_graphics_queue_index = INVALID_VK_INDEX;
    VkQueue protected_graphics_queue = VK_NULL_HANDLE;
    VulkanContext context = {};
    
    bool shared_context = false;
};

// ------------------------------------------------------------------------------------------------
// VulkanPlatform
// ------------------------------------------------------------------------------------------------
VulkanPlatform::VulkanPlatform() = default;

VulkanPlatform::~VulkanPlatform() = default;

// This is the main entry point for context creation.
Driver* VulkanPlatform::CreateDriver(void* sharedContext, const DriverConfig& driverConfig) {
    auto result = volkInitialize();

    if (sharedContext) {
        VulkanSharedContext const* scontext = (VulkanSharedContext const*)sharedContext;
        assert(scontext->instance != VK_NULL_HANDLE && "Client needs to provide VkInstance");
        assert(scontext->physical_device != VK_NULL_HANDLE && "Client needs to provide VkPhysicalDevice");
        assert(scontext->logical_device != VK_NULL_HANDLE && "Client needs to provide VkDevice");
        assert(scontext->graphics_queue_family_index != INVALID_VK_INDEX && "Client needs to provide graphics queue family index");
        assert(scontext->graphics_queue_index != INVALID_VK_INDEX && "Client needs to provide graphics queue index");

        impl->instance = scontext->instance;
        impl->physical_device = scontext->physical_device;
        impl->device = scontext->logical_device;
        impl->graphics_queue_family_index = scontext->graphics_queue_family_index;
        impl->graphics_queue_index = scontext->graphics_queue_index;
        impl->shared_context = true;
    }

    ExtensionSet instanceExts;
    // If using a shared context, we do not assume any extensions.
    if (!impl->shared_context) {
        // This contains instance extensions that are required for the platform, which includes
        // swapchain surface extensions.
        instanceExts = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
        instanceExts.merge(GetRequiredInstanceExtensions());
    }

    if (impl->instance == VK_NULL_HANDLE) {
        createInstance(instanceExts);
    }
    assert(impl->instance != VK_NULL_HANDLE);

    // Load all required Vulkan entrypoints, including all extensions
    volkLoadInstance(impl->instance);

    if (impl->physical_device == VK_NULL_HANDLE) {
        impl->physical_device = selectVkPhysicalDevice(impl->instance);
    }
    assert(impl->physical_device != VK_NULL_HANDLE);

    ExtensionSet deviceExts;
    // If a shared context is not used, we will use our own provided list; otherwise, we do not
    // assume any extensions.
    if (!impl->shared_context) {
        deviceExts = GetDeviceExtensions(impl->physical_device);
        auto [prunedInstExts, prunedDeviceExts] = PruneExtensions(impl->physical_device, driverConfig, instanceExts, deviceExts);
        instanceExts = prunedInstExts;
        deviceExts = prunedDeviceExts;
    }

    // Query all the supported physcial device features and enable/disable any feature as needed
    queryAndSetDeviceFeature(driverConfig, instanceExts, deviceExts, sharedContext);

    const VulkanContext& context = impl->context;

    // Inititalize the required queues
    if (impl->graphics_queue_family_index == INVALID_VK_INDEX) {
        impl->graphics_queue_family_index = IdentifyQueueFamilyIndex(impl->physical_device, VK_QUEUE_GRAPHICS_BIT);
    }

    // At this point, we should have family index that points to a family that has > 0 queues for
    // graphics. In which case, we will allocate one queue (and assumes at least one has been
    // allocated by the clinet if context was shared). If the index of the target queue within
    // the family hasn't been provided by the client, we assume it to be 0.
    if (impl->graphics_queue_index == INVALID_VK_INDEX) {
        impl->graphics_queue_index = 0;
    }

    if (context.protected_memory_supported) {
        impl->protected_graphics_queue_family_index = IdentifyQueueFamilyIndex(impl->physical_device, (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_PROTECTED_BIT));
        if (impl->protected_graphics_queue_index == INVALID_VK_INDEX) {
            impl->protected_graphics_queue_index = 0;
        }
    }

    if (impl->device == VK_NULL_HANDLE) {
        createLogicalDeviceAndQueues(
            deviceExts,
            context.physical_device_features.features,
            context.physical_device_vk11_features,
            context.protected_memory_supported
        );
    }

    assert(impl->device != VK_NULL_HANDLE);

    // Load device-related Vulkan entrypoints directly from the driver
    volkLoadDevice(impl->device);

    vkGetDeviceQueue(impl->device, impl->graphics_queue_family_index, impl->graphics_queue_index, &impl->graphics_queue);
    assert(impl->graphics_queue != VK_NULL_HANDLE);

    if (context.protected_memory_supported) {
        VkDeviceQueueInfo2 info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT,
            .queueFamilyIndex = impl->protected_graphics_queue_family_index,
            .queueIndex = impl->protected_graphics_queue_index
        };
        vkGetDeviceQueue2(impl->device, &info, &impl->protected_graphics_queue);
        assert(impl->protected_graphics_queue != VK_NULL_HANDLE);
    }

    return nullptr;
}

VkInstance VulkanPlatform::createVkInstance(const VkInstanceCreateInfo& createInfo) noexcept {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, VKALLOC, &instance);
    assert(result == VK_SUCCESS && "Unable to create Vulkan instance.");
    return instance;
}

VkPhysicalDevice VulkanPlatform::selectVkPhysicalDevice(VkInstance instance) noexcept {
    const auto pref = GetCustomization().gpu;
    return SelectPhysicalDevice(instance, pref);
}

VkDevice VulkanPlatform::createVkDevice(const VkDeviceCreateInfo& createInfo) noexcept {
    VkDevice device = VK_NULL_HANDLE;
    VkResult result = vkCreateDevice(impl->physical_device, &createInfo, VKALLOC, &device);
    assert(result == VK_SUCCESS && "Unable to create Vulkan device");
    return device;
}

void VulkanPlatform::createInstance(const ExtensionSet& requiredExts) noexcept {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pEngineName = "Vulkan Engine",
        .apiVersion = VK_MAKE_API_VERSION(0, FVK_REQUIRED_VERSION_MAJOR, FVK_REQUIRED_VERSION_MINOR, 0)
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo
    };

    bool validationFeaturesSupported = false;

#if FVK_ENABLED(FVK_DEBUG_VALIDATION)
    const auto enabledLayers = GetEnabledLayers();
    if (!enabledLayers.empty()) {
        // If layers are supported, Check if VK_EXT_validation_features is supported.
        const std::vector<VkExtensionProperties> availableValidationExts = VkEnumerate(vkEnumerateInstanceExtensionProperties, "VK_LAYER_KHRONOS_validation");
        for (const auto& extProps : availableValidationExts) {
            if (!strcmp(extProps.extensionName, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME)) {
                validationFeaturesSupported = true;
                break;
            }
        }
        instanceCreateInfo.enabledLayerCount = (u32)enabledLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    }
    else {
        LOG("Validation layer not available; did you install the Vulkan SDK?\n");
    }
#endif

    std::vector<cstring> enabledExts;
    if (validationFeaturesSupported) {
        enabledExts.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
    }

    for (const auto& requiredExt : requiredExts) {
        enabledExts.push_back(requiredExt.data());
    }

    instanceCreateInfo.enabledExtensionCount = (u32)enabledExts.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExts.data();

    if (SetContains(requiredExts, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    // Validation features
    VkValidationFeatureEnableEXT enables[] = {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

    VkValidationFeaturesEXT features = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = sizeof(enables) / sizeof(enables[0]),
        .pEnabledValidationFeatures = enables
    };

    if (validationFeaturesSupported) {
        ChainStruct(&instanceCreateInfo, &features);
    }

    impl->instance = createVkInstance(instanceCreateInfo);
}

void VulkanPlatform::queryAndSetDeviceFeature(
    const Platform::DriverConfig& driverConfig,
    const ExtensionSet& instExts, const ExtensionSet& deviceExts,
    void* sharedContext) noexcept {
    VulkanContext& context = impl->context;

    VkPhysicalDeviceProtectedMemoryFeatures queryProtectedMemoryFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES
    };

    ChainStruct(&context.physical_device_features, &queryProtectedMemoryFeatures);
    ChainStruct(&context.physical_device_features, &context.physical_device_vk11_features);

    vkGetPhysicalDeviceProperties2(impl->physical_device, &context.physical_device_properties);
    vkGetPhysicalDeviceFeatures2(impl->physical_device, &context.physical_device_features);
    vkGetPhysicalDeviceMemoryProperties(impl->physical_device, &context.memory_properties);

    if (!impl->shared_context) {
        context.debug_utils_supported = SetContains(instExts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        context.debug_markers_supported = SetContains(deviceExts, VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        context.pipeline_creation_feedback_supported = SetContains(deviceExts, VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME);
    }
    else {
        const VulkanSharedContext* scontext = (const VulkanSharedContext*)sharedContext;
        context.debug_utils_supported = scontext->debug_utils_supported;
        context.debug_markers_supported = scontext->debug_markers_supported;
    }

    context.staging_buffer_bypass_enabled = driverConfig.staging_buffer_bypass_enabled;
    context.protected_memory_supported = static_cast<bool>(queryProtectedMemoryFeatures.protectedMemory);
    context.unified_memory_architecture = HasUnifiedMemoryArchitecture(context.memory_properties);
    context.depth_stencil_formats = FindAttachmentDepthStencilFormats(impl->physical_device);
    context.blittable_depth_stencil_formats = FindBlittableDepthStencilFormats(impl->physical_device);
}

void VulkanPlatform::createLogicalDeviceAndQueues(
    const ExtensionSet& deviceExtensions,
    const VkPhysicalDeviceFeatures& features,
    const VkPhysicalDeviceVulkan11Features vk11Features,
    bool createProtectedQueue) noexcept {
    // Identify and select all the required queues
    impl->graphics_queue_family_index = IdentifyQueueFamilyIndex(impl->physical_device, VK_QUEUE_GRAPHICS_BIT);
    impl->graphics_queue_index = 0;

    if (createProtectedQueue) {
        impl->protected_graphics_queue_family_index = IdentifyQueueFamilyIndex(impl->physical_device, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_PROTECTED_BIT);
        impl->protected_graphics_queue_index = 0;
    }

    float queuePriority[] = { 1.0f };
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    };

    std::vector<cstring> requestedExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    for (const auto& ext : deviceExtensions) {
        requestedExtensions.push_back(ext.data());
    }

    VkDeviceQueueCreateInfo deviceQueueCreateInfo[2] = {};
    deviceQueueCreateInfo[0] = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = impl->graphics_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority[0]
    };
    // Protected queue
    deviceQueueCreateInfo[1] = deviceQueueCreateInfo[0];
    deviceQueueCreateInfo[1].flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;

    const bool hasProtectedQueue = impl->protected_graphics_queue_family_index != INVALID_VK_INDEX;
    deviceCreateInfo.queueCreateInfoCount = hasProtectedQueue ? 2 : 1;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;

    // We could simply enable all supported features, but since that may have performance
    // consequences let's just enable the features we need.
    VkPhysicalDeviceFeatures enabledFeatures = {
        .depthClamp = features.depthClamp,
        .samplerAnisotropy = features.samplerAnisotropy,
        .textureCompressionETC2 = features.textureCompressionETC2,
        .textureCompressionBC = features.textureCompressionBC,
        .shaderClipDistance = features.shaderClipDistance
    };

    VkPhysicalDeviceFeatures2 enabledFeatures2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = enabledFeatures
    };
    ChainStruct(&deviceCreateInfo, &enabledFeatures2);

    VkPhysicalDeviceMultiviewFeaturesKHR multiview = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR,
        .multiview = vk11Features.multiview,
        .multiviewGeometryShader = VK_FALSE,
        .multiviewTessellationShader = VK_FALSE
    };
    if (SetContains(deviceExtensions, VK_KHR_MULTIVIEW_EXTENSION_NAME)) {
        ChainStruct(&deviceCreateInfo, &multiview);
    }

    VkPhysicalDeviceProtectedMemoryFeatures protectedMemory = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES,
        .protectedMemory = VK_TRUE
    };
    if (hasProtectedQueue) {
        ChainStruct(&deviceCreateInfo, &protectedMemory);
    }

    VkPhysicalDeviceVulkan11Features enabledVk11Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .multiview = vk11Features.multiview
    };
    ChainStruct(&deviceCreateInfo, &enabledVk11Features);

    deviceCreateInfo.enabledExtensionCount = (u32)requestedExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = requestedExtensions.data();

    impl->device = createVkDevice(deviceCreateInfo);
}

} // namespace VE::backend