#pragma once

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Core/Singleton.h"

namespace VE {

/**
 * Farcade | Singleton
 *
 * Manage the application's main loop logic
 */
class Application : public Core::Singleton<Application> {
public:
    void Run();

private:
    bool initialize();
    void mainLoop();
    void cleanup();

private:
    GLFWwindow* mWindow = nullptr;
};

}