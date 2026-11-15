#pragma once

#include <functional>
#include <string>
#include <vector>

#include <volk.h>

#include "core/Types.h"

struct SDL_Window;
union SDL_Event;

namespace pt {

// Per-frame input snapshot.
struct InputState {
    bool  forward = false, back = false, left = false, right = false, up = false, down = false;
    bool  fast = false;        // shift
    bool  rotating = false;    // right mouse held
    vec2  mouseDelta{ 0.0f };
};

class Window : NonCopyable {
public:
    struct Desc {
        std::string title = "PathTracer";
        u32 width = 1980;
        u32 height = 1080;
    };

    explicit Window(const Desc& desc);
    ~Window();

    bool pollEvents();

    const InputState& input() const { return m_input; }
    Extent2D framebufferExtent() const;
    bool isMinimized() const;
    bool consumeResized();  // true once after a resize

    // hoook for imgui (implement later)
    void setEventHook(std::function<bool(const SDL_Event&)> hook) { m_eventHook = std::move(hook); }

    std::vector<const char*> requiredVulkanInstanceExtensions() const;
    VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

    SDL_Window* sdlHandle() const { return m_window; }

private:
    SDL_Window* m_window = nullptr;
    InputState  m_input{};
    bool        m_resized = false;
    std::function<bool(const SDL_Event&)> m_eventHook;
};

// Directory that contains the executable (with trailing separator).
std::string executableDirectory();

}