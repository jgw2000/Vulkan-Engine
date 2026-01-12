#include "VulkanEngine.h"

namespace VE {

// -----------------------------------------------------------------------------------------------
// VulkanEngine
// -----------------------------------------------------------------------------------------------
bool VulkanEngine::Initialize(void* window) {
    mContext = std::make_unique<VulkanContext>(window);
    return true;
}

}