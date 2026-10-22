set(SMT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/context_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/context_z3.h
    ${CMAKE_CURRENT_LIST_DIR}/constraint_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/constraint_z3.h
    ${CMAKE_CURRENT_LIST_DIR}/vars_in_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/vars_in_z3.h
    ${CMAKE_CURRENT_LIST_DIR}/utils/to_z3_expr.cpp ${CMAKE_CURRENT_LIST_DIR}/utils/to_z3_expr.h
    ${CMAKE_CURRENT_LIST_DIR}/utils/to_z3_expr_splits.cpp ${CMAKE_CURRENT_LIST_DIR}/utils/to_z3_expr_splits.h

)

# model
include(${CMAKE_CURRENT_LIST_DIR}/base/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_BASE_SOURCES})

# model
include(${CMAKE_CURRENT_LIST_DIR}/model/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_MODEL_SOURCES})

# visitor
include(${CMAKE_CURRENT_LIST_DIR}/visitor/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_VISITOR_SOURCES})

# solver
include(${CMAKE_CURRENT_LIST_DIR}/solver/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_SOLVER_SOURCES})

# solution enumerator
include(${CMAKE_CURRENT_LIST_DIR}/solution_enumerator/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SOLUTION_ENUMERATOR_SOURCES})

# nn
include(${CMAKE_CURRENT_LIST_DIR}/nn/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_NN_Z3_SOURCES})

# fnn
include(${CMAKE_CURRENT_LIST_DIR}/fnn/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_FNN_Z3_SOURCES})

# policy
include(${CMAKE_CURRENT_LIST_DIR}/policy/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SMT_POLICY_Z3_SOURCES})

# successor generation
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND SMT_SOURCES ${SUCCESSOR_GENERATION_Z3_SOURCES})
