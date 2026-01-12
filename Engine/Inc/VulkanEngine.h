#pragma once

#include <memory>

#include <VulkanContext.h>

namespace VE {

/**
 * Facade
 */
class VulkanEngine {
public:
    bool Initialize(void* window);

private:
    std::unique_ptr<VulkanContext> mContext;
};

}