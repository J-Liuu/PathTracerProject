# List shader *entry-point* files here (files with [shader("...")] functions).
# Shared code (.slang modules, .slangh / .h includes) does not need to be listed:
# every file under shaders/ is tracked, and editing one rebuilds the shaders.
#
# Output: <build>/bin/shaders/<same relative path>/<name>.spv

pt_add_slang_shaders(TARGET PathTracerShaders_Integrators
    SOURCES
        integrators/pathtrace_mc.slang
        # integrators/restir_di.slang
    )           

pt_add_slang_shaders(TARGET PathTracerShaders_Passes
    SOURCES
        passes/tonemap.slang
        # passes/testpattern.slang
    )

add_custom_target(PathTracerShaders ALL)
add_dependencies(PathTracerShaders PathTracerShaders_Integrators PathTracerShaders_Passes)
set_target_properties(PathTracerShaders PROPERTIES FOLDER "PathTracer")
