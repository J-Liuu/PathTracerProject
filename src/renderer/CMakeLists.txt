# renderer: GpuScene, integrators, passes. Bridges scene (CPU) and vk (GPU).
pt_add_module(pt_renderer)
target_link_libraries(pt_renderer PUBLIC pt_vk pt_scene)
# Shared C++/Slang structs live in shaders/shared/
target_include_directories(pt_renderer PUBLIC ${PROJECT_SOURCE_DIR}/shaders)
if(TARGET slang::runtime)
    target_link_libraries(pt_renderer PRIVATE slang::runtime)   # optional hot-reload
endif()

# Show the shared header in this project so IntelliSense parses it as C++20 with GLM.
target_sources(pt_renderer PRIVATE ${PROJECT_SOURCE_DIR}/shaders/shared/GpuStructs.h)
source_group("shared" FILES ${PROJECT_SOURCE_DIR}/shaders/shared/GpuStructs.h)