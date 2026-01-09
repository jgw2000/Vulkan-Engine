#pragma once

#include <cstdint>

#include "Core/Singleton.h"

class GLFWwindow;

namespace VE {

class VulkanEngine;

/**
 * Facade | Singleton
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
    uint16_t mWidth = 800;
    uint16_t mHeight = 600;

    VulkanEngine* mEngine = nullptr;
};

}