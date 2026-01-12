#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanEngine.h"

namespace VE {

// -----------------------------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------------------------
template <>
Application* Core::Singleton<Application>::mSingleton = nullptr;

void Application::Run() {
    if (!initialize()) {
        return;
    }

    mainLoop();
    cleanup();
}

bool Application::initialize() {
    if (glfwInit() != GLFW_TRUE) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    mWindow = glfwCreateWindow(mWidth, mHeight, "Vulkan Engine", nullptr, nullptr);
    if (!mWindow) {
        return false;
    }

    mEngine = new VulkanEngine();
    if (!mEngine->Initialize(mWindow)) {
        return false;
    }

    return true;
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
    }
}

void Application::cleanup() {
    glfwDestroyWindow(mWindow);
    glfwTerminate();

    if (mEngine) {
        delete mEngine;
    }
}

}