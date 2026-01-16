#include <VulkanEngine.h>

namespace VE {

// -----------------------------------------------------------------------------------------------
// VulkanEngine
// -----------------------------------------------------------------------------------------------
bool VulkanEngine::Initialize(void* window) {
    mContext = std::make_unique<Gfx::VulkanContext>(window);
    return true;
}

void VulkanEngine::Render() {
    mContext->Render();
}

}