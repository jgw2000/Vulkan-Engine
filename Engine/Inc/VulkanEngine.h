#pragma once

#include <memory>

#include <Graphics/VulkanContext.h>

namespace VE {

/**
 * Facade
 */
class VulkanEngine {
public:
    bool Initialize(void* window);
    void Render();

private:
    std::unique_ptr<Gfx::VulkanContext> mContext;
};

}