#pragma once

#include <string>

#include <Platform/Types.h>

namespace VE {
struct Config {
    std::string title = "Vulkan Engine";
    sizet width = 1024;
    sizet height = 640;

    bool resizeable = true;
    bool headless = false;
};
} // namespace VE