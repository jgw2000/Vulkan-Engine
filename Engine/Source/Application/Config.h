#pragma once

#include <string>

#include <Platform/Types.h>

#include <Function/Engine.h>

namespace VE {
struct Config {
    std::string title = "Vulkan Engine";
    sizet width = 1024;
    sizet height = 640;

    mutable Engine::Backend backend = Engine::Backend::VULKAN;
    mutable backend::FeatureLevel feature_level = backend::FeatureLevel::FEATURE_LEVEL_3;
    bool resizeable = true;
    bool headless = false;
};
} // namespace VE