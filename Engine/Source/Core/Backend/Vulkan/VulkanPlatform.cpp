#define VOLK_IMPLEMENTATION

#include <cassert>
#include <string>
#include <vector>

#include "VulkanPlatform.h"

#include <Core/Debug.h>
#include <Core/Backend/Vulkan/VulkanConstants.h>

namespace VE::backend {

namespace {

constexpr u32 INVALID_VK_INDEX = 0xFFFFFFFF;

using ExtensionSet = VulkanPlatform::ExtensionSet;

inline bool SetContains(const ExtensionSet& set, cstring extension) {
    return set.find(extension) != set.end();
}

#if FVK_ENABLED(FVK_DEBUG_VALIDATION)
const std::string_view DESIRED_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
#if FVK_ENABLED(FVK_DEBUG_DUMP_API)
    "VK_LAYER_LUNARG_api_dump"
#endif
};
#endif

}

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

template<typename StructA, typename StructB>
StructA* ChainStruct(StructA* structA, StructB* structB) {
    structB->pNext = const_cast<void*>(structA->pNext);
    structA->pNext = (void*)structB;
    return structA;
}

VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, const VulkanPlatform::Customization::GPUPreference& gpuPreference) {
    return VK_NULL_HANDLE;
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

} // namespace VE::backend