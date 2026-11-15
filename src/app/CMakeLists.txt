file(GLOB_RECURSE _app_srcs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(PathTracer ${_app_srcs})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_app_srcs})
target_include_directories(PathTracer PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(PathTracer PRIVATE pt_renderer pt_platform pt::SDL3)   # SDL3 for SDL_main.h
if(TARGET imgui)
    target_link_libraries(PathTracer PRIVATE imgui)
    target_compile_definitions(PathTracer PRIVATE PT_WITH_IMGUI=1)
endif()
pt_set_project_options(PathTracer)

# SDL3 from the SDK is a DLL: copy it next to PathTracer.exe after every build.
pt_copy_runtime_dlls(PathTracer)

if(TARGET PathTracerShaders)
    add_dependencies(PathTracer PathTracerShaders)
endif()

# Dev builds load SPIR-V straight from the build tree; shipping builds fall back to <exe dir>/shaders.
target_compile_definitions(PathTracer PRIVATE
    PT_SHADER_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    PT_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")

# Launch from the exe's folder when debugging from Visual Studio.
set_target_properties(PathTracer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:PathTracer>")
