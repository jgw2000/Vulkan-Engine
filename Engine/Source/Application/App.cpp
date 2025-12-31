#include "App.h"

#include <cassert>
#include <memory>

#include <Platform/Platform.h>

#include <Core/Backend/Vulkan/VulkanPlatform.h>

namespace VE {

// ------------------------------------------------------------------------------------------------
// App
// ------------------------------------------------------------------------------------------------
App& App::Get() {
    static App app;
    return app;
}

App::App() {
    initSDL();
}

App::~App() {
    SDL_Quit();
}

void App::Run(const Config& config) {
    std::unique_ptr<Window> window = std::make_unique<Window>(
        this, config);

    while (!is_closed) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    is_closed = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                case SDL_EVENT_MOUSE_WHEEL:
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                case SDL_EVENT_MOUSE_MOTION:
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    break;
                default:
                    break;
            }
        }
    }

#if defined(BACKEND_SUPPORTS_VULKAN)
    if (vulkan_platform) {
        delete vulkan_platform;
    }
#endif
}

void App::initSDL() {
    SDL_Init(SDL_INIT_EVENTS);
}

// ------------------------------------------------------------------------------------------------
// App::Window
// ------------------------------------------------------------------------------------------------
App::Window::Window(App* app, const Config& config)
    : app(app), config(config) {
    u32 windowFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN;
    if (config.resizeable) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    if (config.headless) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    sdl_window = SDL_CreateWindow(config.title.c_str(), config.width, config.height, windowFlags);
    assert(sdl_window  && "SDL create window failed!");

    const auto createEngine = [&config, this]() {
        backend::Platform* platform = nullptr;
#if defined(BACKEND_SUPPORTS_VULKAN)
        if (config.backend == Engine::Backend::VULKAN) {
            platform = this->app->vulkan_platform = new backend::VulkanPlatform();
        }
#endif

        Engine::Builder builder = Engine::Builder();

        return builder.Backend(config.backend)
                .FeatureLevel(config.feature_level)
                .Platform(platform)
                .Build();
    };

    if (config.headless) {

    }
    else {
        app->engine = createEngine();
        width = config.width;
        height = config.height;
    }
}

App::Window::~Window() {
    SDL_DestroyWindow(sdl_window);
}

}