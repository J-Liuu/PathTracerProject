# pt_add_slang_shaders(
#     TARGET   <name>                 custom target that builds all listed shaders
#     SOURCES  <a.slang> ...          entry-point files (all [shader(...)] entries go in one .spv)
#     PERMUTATIONS <NAME=DEFINES> ...  optional, e.g. "spectral=SPECTRAL=1"; each source is
#                                      also built as <name>_<perm>.spv with -D<DEFINES>
# )
# Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/<relative/path>/<name>[_perm].spv
function(pt_add_slang_shaders)
    cmake_parse_arguments(ARG "" "TARGET" "SOURCES;PERMUTATIONS" ${ARGN})

    if(NOT SLANGC_EXECUTABLE)
        add_custom_target(${ARG_TARGET} COMMENT "slangc not found - skipping shader compilation")
        return()
    endif()

    set(_shader_root ${PROJECT_SOURCE_DIR}/shaders)
    set(_out_root    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

    # Any change to shared code / headers rebuilds every shader (simple and correct).
    file(GLOB_RECURSE _all_shader_files CONFIGURE_DEPENDS
        ${_shader_root}/*.slang ${_shader_root}/*.slangh ${_shader_root}/*.h)

    set(_common_flags
        -target spirv
        -profile spirv_1_6
        -I ${_shader_root}
        -matrix-layout-column-major        # match GLM
        -fvk-use-scalar-layout             # match C++ structs (scalarBlockLayout)
        -fvk-use-entrypoint-name           # keep "rayGen", "closestHit", ... as SPIR-V entry names
        -g2                                # debug info for RenderDoc / Nsight source view
        -O2
        -warnings-disable 41012)          # "profile implicitly upgraded" for ray query entry points

    set(_outputs)
    foreach(_src IN LISTS ARG_SOURCES)
        get_filename_component(_abs ${_src} ABSOLUTE)
        file(RELATIVE_PATH _rel ${_shader_root} ${_abs})
        get_filename_component(_rel_dir ${_rel} DIRECTORY)
        get_filename_component(_name ${_rel} NAME_WE)

        # Base variant + permutations
        set(_variants "base=")
        list(APPEND _variants ${ARG_PERMUTATIONS})
        foreach(_variant IN LISTS _variants)
            string(FIND "${_variant}" "=" _eq)
            string(SUBSTRING "${_variant}" 0 ${_eq} _vname)
            math(EXPR _eq1 "${_eq} + 1")
            string(SUBSTRING "${_variant}" ${_eq1} -1 _vdefs)

            if(_vname STREQUAL "base")
                set(_out ${_out_root}/${_rel_dir}/${_name}.spv)
                set(_defs)
            else()
                set(_out ${_out_root}/${_rel_dir}/${_name}_${_vname}.spv)
                set(_defs -D${_vdefs})
            endif()

            add_custom_command(
                OUTPUT  ${_out}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${_out_root}/${_rel_dir}
                COMMAND ${SLANGC_EXECUTABLE} ${_abs} ${_common_flags} ${_defs} -o ${_out}
                DEPENDS ${_abs} ${_all_shader_files}
                COMMENT "slangc ${_rel} -> ${_name}${_vname}.spv"
                VERBATIM)
            list(APPEND _outputs ${_out})
        endforeach()
    endforeach()

    # add_custom_target(${ARG_TARGET} ALL DEPENDS ${_outputs} SOURCES ${_all_shader_files})
    # if(_all_shader_files)
    #     source_group(TREE ${_shader_root} FILES ${_all_shader_files})
    # endif()
    set(_ide_files ${_all_shader_files})
    list(FILTER _ide_files EXCLUDE REGEX "\\.h$")
    add_custom_target(${ARG_TARGET} ALL DEPENDS ${_outputs} SOURCES ${_ide_files})
    if(_ide_files)
        source_group(TREE ${_shader_root} FILES ${_ide_files})
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES FOLDER "PathTracer")
endfunction()
