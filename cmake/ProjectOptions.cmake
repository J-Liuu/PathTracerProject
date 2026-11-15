# Common compile options applied to every first-party target.
function(pt_set_project_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8 /MP)
        if(PT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
        if(PT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:PT_DEBUG=1>
        $<$<BOOL:${PT_ENABLE_VALIDATION}>:PT_ENABLE_VALIDATION=1>
        $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN>)

    set_target_properties(${target} PROPERTIES FOLDER "PathTracer")
endfunction()

# Adds a module library whose sources are every .h/.cpp in the calling folder.
# CONFIGURE_DEPENDS: new files are picked up on the next build (no CMake edit needed).
#
# While a module is still empty (or header-only), CMake can't build a library from it,
# so a generated placeholder .cpp is added. It disappears automatically once the
# folder contains a real .cpp. This lets you use target_link_libraries(... PUBLIC/PRIVATE)
# from day one, before you've written any code.
function(pt_add_module target)
    file(GLOB_RECURSE _srcs CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    list(FILTER _srcs EXCLUDE REGEX "/CMakeFiles/")

    set(_cpp ${_srcs})
    list(FILTER _cpp INCLUDE REGEX "\\.cpp$")
    set(_placeholder)
    if(NOT _cpp)
        set(_placeholder ${CMAKE_CURRENT_BINARY_DIR}/${target}_placeholder.cpp)
        file(CONFIGURE OUTPUT ${_placeholder}
             CONTENT "// Generated: ${target} has no .cpp files yet. Removed once you add one.\n")
    endif()

    add_library(${target} STATIC ${_srcs} ${_placeholder})
    # Include root is src/, so code includes "vk/Context.h", "scene/Scene.h", ...
    target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/src)
    if(_srcs)
        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_srcs})
    endif()
    if(_placeholder)
        source_group("Generated" FILES ${_placeholder})
        if(MSVC)
            # LNK4221: "object file does not define any public symbols" - expected for the placeholder
            set_target_properties(${target} PROPERTIES STATIC_LIBRARY_OPTIONS "/IGNORE:4221")
        endif()
    endif()
    pt_set_project_options(${target})
endfunction()

# Copies every DLL the target depends on (e.g. SDL3.dll from the Vulkan SDK) next to the
# executable after each build. No-op when there are none (static builds, Linux).
function(pt_copy_runtime_dlls target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E
                $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,copy_if_different,true>
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to ${target}")
    endif()
endfunction()
