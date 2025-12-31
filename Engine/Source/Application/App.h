#pragma once

#include <SDL3/SDL.h>

#include <Platform/Types.h>

#include <Function/Engine.h>

#include <Application/Config.h>

namespace VE::backend {

#if defined(BACKEND_SUPPORTS_VULKAN)
class VulkanPlatform;
#endif

}

namespace VE {

class App {
public:
    static App& Get();

    ~App();

    void Run(const Config& config);

private:
    App();

    class Window {
        friend class App;
    public:
        Window(App* app, const Config& config);
        virtual ~Window();

        SDL_Window* GetSDLWindow() {
            return sdl_window;
        }

    private:
        App* const app = nullptr;
        Config config;
        
        SDL_Window* sdl_window = nullptr;

        sizet width = 0;
        sizet height = 0;
        sizet last_x = 0;
        sizet last_y = 0;
    };

    friend class Window;
    void initSDL();

    Engine* engine = nullptr;

    bool is_closed = false;

#if defined(BACKEND_SUPPORTS_VULKAN)
    backend::VulkanPlatform* vulkan_platform = nullptr;
#endif
};

} // namespace VE