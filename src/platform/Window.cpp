#include "platform/Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/Assert.h"

namespace pt {

    Window::Window(const Desc& desc) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            log::error("SDL_Init failed: {}", SDL_GetError());
            PT_ASSERT(false, "SDL_Init");
        }
        m_window = SDL_CreateWindow(desc.title.c_str(), static_cast<int>(desc.width), static_cast<int>(desc.height),
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        PT_ASSERT(m_window != nullptr, SDL_GetError());
    }

    Window::~Window() {
        if (m_window) {
            SDL_DestroyWindow(m_window);
        }
        SDL_Quit();
    }

    bool Window::pollEvents() {
        m_input.mouseDelta = vec2(0.0f);

        bool running = true;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (m_eventHook && m_eventHook(e)) {
                continue;  // UI consumed it
            }
            switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                m_resized = true;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (m_input.rotating) {
                    m_input.mouseDelta += vec2(e.motion.xrel, e.motion.yrel);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    m_input.rotating = e.button.down;
                    SDL_SetWindowRelativeMouseMode(m_window, e.button.down);
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_ESCAPE) running = false;
                break;
            default:
                break;
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        m_input.forward = keys[SDL_SCANCODE_W];
        m_input.back = keys[SDL_SCANCODE_S];
        m_input.left = keys[SDL_SCANCODE_A];
        m_input.right = keys[SDL_SCANCODE_D];
        m_input.up = keys[SDL_SCANCODE_E];
        m_input.down = keys[SDL_SCANCODE_Q];
        m_input.fast = keys[SDL_SCANCODE_LSHIFT];
        return running;
    }

    Extent2D Window::framebufferExtent() const {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(m_window, &w, &h);
        return { static_cast<u32>(w), static_cast<u32>(h) };
    }

    bool Window::isMinimized() const {
        return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) != 0;
    }

    bool Window::consumeResized() {
        const bool r = m_resized;
        m_resized = false;
        return r;
    }

    std::vector<const char*> Window::requiredVulkanInstanceExtensions() const {
        Uint32 count = 0;
        const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
        return std::vector<const char*>(names, names + count);
    }

    VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface)) {
            log::error("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
            PT_ASSERT(false, "surface creation");
        }
        return surface;
    }

    std::string executableDirectory() {
        const char* base = SDL_GetBasePath();
        return base ? std::string(base) : std::string("./");
    }

} // namespace pt