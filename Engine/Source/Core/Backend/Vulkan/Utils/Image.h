#pragma once

#include <Volk/volk.h>

namespace VE::backend {

bool IsVkDepthFormat(VkFormat format);

VkImageAspectFlags GetImageAspect(VkFormat format);

} // namespace VK::backend