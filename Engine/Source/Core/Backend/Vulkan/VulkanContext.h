#pragma once

#include <vector>

#include <Volk/volk.h>

namespace VE::backend {

// This is a collection of immutable data about the vulkan context. This actual handles to the
// context are stored in VulkanPlatform.
struct VulkanContext {
private:
    VkPhysicalDeviceMemoryProperties memory_properties = {};
    VkPhysicalDeviceProperties2 physical_device_properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    VkPhysicalDeviceVulkan11Features physical_device_vk11_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES
    };
    VkPhysicalDeviceFeatures2 physical_device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    
    bool debug_markers_supported = false;
    bool debug_utils_supported = false;
    bool lazily_allocated_memory_supported = false;
    bool protected_memory_supported = false;
    bool unified_memory_architecture = false;
    bool staging_buffer_bypass_enabled = false;
    bool pipeline_creation_feedback_supported = false;

    std::vector<VkFormat> depth_stencil_formats;
    std::vector<VkFormat> blittable_depth_stencil_formats;

    friend class VulkanPlatform;
};

} // namespace VE::backend