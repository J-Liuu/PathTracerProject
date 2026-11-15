# platform: window + input + filesystem. Knows SDL, and only Vulkan *handle types* (for the surface).
pt_add_module(pt_platform)
target_link_libraries(pt_platform
    PUBLIC  pt_core volk::volk
    PRIVATE pt::SDL3)          # SDL stays an implementation detail of this module
