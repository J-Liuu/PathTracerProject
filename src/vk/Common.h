#pragma once


// This will be our single include for Vulkan in this project.
// Volk provides all function pointers

#include <volk.h>
#include <vk_mem_alloc.h>

#include "../core/Types.h"
#include "../core/Log.h"
#include "../core/Assert.h"

#define VK_CHECK(expr)                                                                         \
    do {                                                                                       \
        const VkResult _vk_result = (expr);                                                    \
        if (_vk_result != VK_SUCCESS) {                                                        \
            ::pt::log::error("Vulkan call failed ({}): {} at {}:{}", static_cast<int>(_vk_result), \
                             #expr, __FILE__, __LINE__);                                       \
            PT_DEBUG_BREAK();                                                                  \
        }                                                                                      \
    } while (0)


namespace pt::vk {

    constexpr u32 kMaxFramesInFlight = 2;

} // namespace pt::vk