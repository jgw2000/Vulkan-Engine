#include "App.h"

#include <memory>

namespace VE {

// ------------------------------------------------------------------------------------------------
// App
// ------------------------------------------------------------------------------------------------
App& App::Get() {
    static App app;
    return app;
}

App::App() {
    InitSDL();
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
}

void App::InitSDL() {
    SDL_Init(SDL_INIT_EVENTS);
}

// ------------------------------------------------------------------------------------------------
// App::Window
// ------------------------------------------------------------------------------------------------
App::Window::Window(App* app, const Config& config)
    : app(app), config(config) {
    u32 window_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN;
    if (config.resizeable) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }

    if (config.headless) {
        window_flags |= SDL_WINDOW_HIDDEN;
    }

    sdl_window = SDL_CreateWindow(config.title.c_str(), config.width, config.height, window_flags);
}

App::Window::~Window() {
    SDL_DestroyWindow(sdl_window);
}

}