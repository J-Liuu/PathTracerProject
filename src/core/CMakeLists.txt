# core: types, logging, asserts, GLM aliases. Depends on nothing but GLM.
pt_add_module(pt_core)
target_link_libraries(pt_core PUBLIC glm::glm)
