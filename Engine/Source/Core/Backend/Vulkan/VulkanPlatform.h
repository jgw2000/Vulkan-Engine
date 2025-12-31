#pragma once

#include <string>
#include <unordered_set>

#include <Volk/volk.h>

#include <Platform/Types.h>
#include <Platform/Platform.h>

#include <Core/PrivateImplementation-impl.h>

namespace VE::backend {

struct VulkanPlatformPrivate;

/**
 * A Platform interface that creates a Vulkan backend.
 */
class VulkanPlatform : public Platform, PrivateImplementation<VulkanPlatformPrivate> {
public:

    // Utility for managing device or instance extensions during initialization.
    using ExtensionSet = std::unordered_set<std::string>;

    /**
     * A collection of handles to objects and metadata that comprises a Vulkan context. The client
     * can instantiate this struct and pass to Engine::Builder::SharedContext if they wishes to
     * share their vulkan context. This is specifically necessary if the client wishes to override
     * the swapchain API.
     */
    struct VulkanSharedContext {
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice logical_device = VK_NULL_HANDLE;
        u32 graphics_queue_family_index = 0xFFFFFFFF;
        // In the usual case, the client needs to allocate at least one more graphics queue and this
        // index is the param to pass into vkGetDeviceQueue. In the case where the gpu only has one
        // graphics queue, then the client needs to ensure that no concurrent access can occur.
        u32 graphics_queue_index = 0xFFFFFFFF;
        bool debug_utils_supported = false;
        bool debug_markers_supported = false;
        bool multi_view_supported = false;
    };

    VulkanPlatform();
    ~VulkanPlatform() override;

    Driver* CreateDriver(void* sharedContext, const DriverConfig& driverConfig) override;

    struct Customization {
        /**
         * The client can specify the GPU (i.e. VkDevice) for the platform. We allow the
         * following preferences:
         *     1) A substring to match against `VkPhysicalDeviceProperties.deviceName`. Empty string
         *        by default.
         *     2) Index of the device in the list as returned by
         *        `vkEnumeratePhysicalDevices`. -1 by default to indicate no preference.
         */
        struct GPUPreference {
            cstring device_name;
            i8 index = -1;
        } gpu;

        /**
         * Whether the platform supports sRGB swapchain. Default is true.
         */
        bool is_srgb_swapchain_supported = true;

        /**
         * When the platform window is resized, we will flush and wait on the command queues
         * before recreating the swapchain. Default is true.
         */
        bool flush_and_wait_window_resize = true;

        /**
         * Whether the swapchain image should be transitioned to a layout suitable for
         * presentation. Default is true.
         */
        bool transition_swapchain_image_layout_for_present = true;
    };

    virtual Customization GetCustomization() const noexcept {
        return {};
    }

protected:
    /**
     * Creates the VkInstance used by Vulkan backend.
     *
     * This method can be overridden in subclasses to customize VkInstance creation, such as
     * adding application-specific layers or extensions.
     *
     * @param createInfo The VkInstanceCreateInfo prepared by Engine.
     * @return The created VkInstance, or VK_NULL_HANDLE on failure.
     */
    virtual VkInstance createVkInstance(const VkInstanceCreateInfo& createInfo) noexcept;

    /**
     * Selects a VkPhysicalDevice (GPU) for Vulkan backend to use.
     *
     * This method can be overridden in subclasses to implement custom GPU selection logic.
     * For example, an application might override this to prefer a discrete GPU over an
     * integrated one based on device properties.
     *
     * The default implementation selects the first device that meets Engine's requirements.
     *
     * @param instance The VkInstance to enumerate devices from.
     * @return The selected VkPhysicalDevice, or VK_NULL_HANDLE if no suitable device is found.
     */
    virtual VkPhysicalDevice selectVkPhysicalDevice(VkInstance instance) noexcept;

private:
    void createInstance(const ExtensionSet& requiredExts) noexcept;
};

} // namespace VE::backend