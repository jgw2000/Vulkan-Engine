#pragma once

namespace VE::backend {

class Driver;

/**
 * Platform is an interface that abstracts how the backend (also referred to as Driver) is
 * created. The backend provides several common Platform concrete implementations, which are
 * selected automatically.
 */
class Platform {
public:
    struct DriverConfig {
        /**
         * Bypass the staging buffer because the device is of Unified Memory Architecture.
         * This is only supported for:
         *      - VulkanPlatform
         */
        bool staging_buffer_bypass_enabled = false;
    };

    Platform() noexcept;
    virtual ~Platform() noexcept;

    /**
     * Creates and initialize the low-level API (e.g. an OpenGL context or Vulkan instance),
     * then creates the concrete Driver.
     * The caller takes ownership of the returned Driver* and must destroy it with delete.
     *
     * @param sharedContext an optional shared context. This is not meaningful with all graphic
                            APIs and platforms.
                            For EGL platforms, this is an EGLContext.
     *
     * @param driverConfig  specifies driver initialization parameters
     *
     * @return nullptr on failure, or a pointer to the newly created driver.
     */
     virtual Driver* CreateDriver(void* sharedContext, const DriverConfig& driverConfig) = 0;
};

} // namespace VE::backend