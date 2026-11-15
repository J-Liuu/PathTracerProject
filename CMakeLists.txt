cmake_minimum_required(VERSION 3.25)   # FindVulkan "volk" component + $<TARGET_RUNTIME_DLLS>

project(PathTracer
    VERSION 0.1.0
    DESCRIPTION "Vulkan Monte-Carlo / ReSTIR path tracer with optional spectral rendering"
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# One output folder so the exe finds SDL3.dll and shaders/ next to it.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)   # Solution Explorer folders

option(PT_ENABLE_VALIDATION  "Enable Vulkan validation layers by default" ON)
option(PT_COMPILE_SHADERS    "Compile Slang shaders to SPIR-V at build time" ON)
option(PT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(PT_WITH_IMGUI         "Fetch and build Dear ImGui (not part of the Vulkan SDK)" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(ProjectOptions)   # pt_set_project_options(), pt_add_module(), pt_copy_runtime_dlls()
include(Dependencies)     # Vulkan, volk, VMA, GLM, SDL3, Slang - all from the Vulkan SDK
include(Shaders)          # pt_add_slang_shaders()

# ---------------------------------------------------------------------------
# Modules: each is a static library; dependencies only point downward
#   app -> renderer -> scene, vk -> platform, core
# ---------------------------------------------------------------------------
if(PT_COMPILE_SHADERS)
    add_subdirectory(shaders)   # defines PathTracerShaders before the app depends on it
endif()

add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/vk)
add_subdirectory(src/scene)
add_subdirectory(src/renderer)
add_subdirectory(src/app)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PathTracer)

