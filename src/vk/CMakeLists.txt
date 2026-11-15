# vk: thin RAII wrappers over Vulkan. Knows nothing about scenes, cameras or materials.
pt_add_module(pt_vk)
target_link_libraries(pt_vk PUBLIC pt_core volk::volk GPUOpen::VMA)
