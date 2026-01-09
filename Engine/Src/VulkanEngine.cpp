#include "VulkanEngine.h"

namespace VE {

// -----------------------------------------------------------------------------------------------
// VulkanEngine
// -----------------------------------------------------------------------------------------------
bool VulkanEngine::Initialize() {
    mContext = std::make_unique<VulkanContext>();
    return true;
}

}