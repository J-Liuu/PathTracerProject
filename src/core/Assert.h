// #################
// AI GENERATED FILE
// #################

#pragma once

#include <cstdlib>

#include "core/Log.h"

#if defined(_MSC_VER)
#define PT_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define PT_DEBUG_BREAK() __builtin_trap()
#else
#define PT_DEBUG_BREAK() std::abort()
#endif



// Always-on assert: a renderer that silently continues with bad state wastes hours.
#define PT_ASSERT(cond, msg)                                                            \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            ::pt::log::error("Assertion failed: {} ({}) at {}:{}", #cond, msg, __FILE__, \
                             __LINE__);                                                 \
            PT_DEBUG_BREAK();                                                           \
        }                                                                               \
    } while (0)

#define PT_UNIMPLEMENTED(what) ::pt::log::warn("Not implemented yet: {} ({}:{})", what, __FILE__, __LINE__)