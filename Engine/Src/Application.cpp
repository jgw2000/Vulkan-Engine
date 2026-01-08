#include "Application.h"

#include <cstdint>

namespace VE {

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;

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

    mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
    if (!mWindow) {
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
}

}