#pragma once

#include <array>
#include <string_view>

#include <Platform/Types.h>

namespace VE::backend {

enum class FeatureLevel : u8 {
    FEATURE_LEVEL_0 = 0,  //!< OpenGL ES 2.0 features
    FEATURE_LEVEL_1,      //!< OpenGL ES 3.0 features (default)
    FEATURE_LEVEL_2,      //!< OpenGL ES 3.1 features + 16 textures units + cubemap arrays
    FEATURE_LEVEL_3       //!< OpenGL ES 3.1 features + 31 textures units + cubemap arrays
};

enum class Backend : u8 {
    DEFAULT = 0,
    VULKAN = 1
};

constexpr std::string_view ToString(const Backend backend) noexcept {
    switch (backend) {
        case Backend::VULKAN:
            return "Vulkan";
        case Backend::DEFAULT:
            return "Default";
    }
    return "Unknown";
}

enum class ShaderLanguage {
    UNSPECIFIED = -1
};

constexpr cstring ShaderLanguageToString(ShaderLanguage lang) noexcept {
    switch (lang) {
        case ShaderLanguage::UNSPECIFIED:
            return "Unspecified";
    }
    return "Unknown";
}

enum class ShaderStage : u8 {
    VERTEX = 0,
    FRAGMENT = 1,
    COMPUTE = 2
};

static constexpr sizet PIPELINE_STAGE_COUNT = 2;
enum class ShaderStageFlags : u8 {
    NONE = 0,
    VERTEX = 0x1,
    FRAGMENT = 0x2,
    COMPUTE = 0x4,
    ALL_SHADER_STAGE_FLAGS = VERTEX | FRAGMENT | COMPUTE
};

constexpr bool HasShaderType(ShaderStageFlags flags, ShaderStage type) noexcept {
    switch (type) {
        case ShaderStage::VERTEX:
            return bool(u8(flags) & u8(ShaderStageFlags::VERTEX));
        case ShaderStage::FRAGMENT:
            return bool(u8(flags) & u8(ShaderStageFlags::FRAGMENT));
        case ShaderStage::COMPUTE:
            return bool(u8(flags) & u8(ShaderStageFlags::COMPUTE));
    }
}

} // namespace VE::backend