#pragma once

#include <memory>

#include <VulkanContext.h>

namespace VE {

/**
 * Facade
 */
class VulkanEngine {
public:
    bool Initialize();

private:
    std::unique_ptr<VulkanContext> mContext;
};

}