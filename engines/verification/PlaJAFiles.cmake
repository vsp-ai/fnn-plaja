# Statistics and predicate dependencies:
set(VERIFICATION_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/search_statistics_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics_pa.h
    ${CMAKE_CURRENT_LIST_DIR}/predicate_dependency_graph.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_dependency_graph.h
)

# Abstract states:
include(${CMAKE_CURRENT_LIST_DIR}/pa_states/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_STATES_SOURCES})

# Transition structure:
list(APPEND VERIFICATION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/pa_transition_structure.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_transition_structure.h)

# Solution checker:
list(APPEND VERIFICATION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/solution_checker_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_pa.h)
list(APPEND VERIFICATION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_checker_instance_pa.h)

# Z3 and Marabou encodings:
include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_SMT_SOURCES})

# Optimization:
include(${CMAKE_CURRENT_LIST_DIR}/optimization/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_OPTIMIZATION_SOURCES})

# Successor generation:
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_SUCCESSOR_GENERATION_SOURCES})

# NN checkers:
include(${CMAKE_CURRENT_LIST_DIR}/checkers/nn/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_NN_SOURCES})

# FNN checkers:
include(${CMAKE_CURRENT_LIST_DIR}/checkers/fnn/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_FNN_SOURCES})

# Policy checker base:
list(APPEND VERIFICATION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/checkers/policy_checker.cpp ${CMAKE_CURRENT_LIST_DIR}/checkers/policy_checker.h)

# Heuristic:
include(${CMAKE_CURRENT_LIST_DIR}/heuristic/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_HEURISTIC_SOURCES})

# match tree
include(${CMAKE_CURRENT_LIST_DIR}/match_tree/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_MATCH_TREE_SOURCES})

# Inc state space:
include(${CMAKE_CURRENT_LIST_DIR}/inc_state_space/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_INC_STATE_SPACE_SOURCES})

# Search space:
include(${CMAKE_CURRENT_LIST_DIR}/search_space/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${PA_SEARCH_SPACE_SOURCES})

list(APPEND VERIFICATION_SOURCES
    # main classes
    ${CMAKE_CURRENT_LIST_DIR}/base/expansion_pa_base.cpp ${CMAKE_CURRENT_LIST_DIR}/base/expansion_pa_base.h
    ${CMAKE_CURRENT_LIST_DIR}/base/search_pa_base.cpp ${CMAKE_CURRENT_LIST_DIR}/base/search_pa_base.h
    ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction.cpp ${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction.h
    ${CMAKE_CURRENT_LIST_DIR}/search_pa_path.cpp ${CMAKE_CURRENT_LIST_DIR}/search_pa_path.h
)

# cegar
include(${CMAKE_CURRENT_LIST_DIR}/cegar/PlaJAFiles.cmake)
list(APPEND VERIFICATION_SOURCES ${CEGAR_SOURCES})

# factory
list(APPEND VERIFICATION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/search_engine_config_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/search_engine_config_pa.h
        ${CMAKE_CURRENT_LIST_DIR}/pa_options.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_options.h
        ${CMAKE_CURRENT_LIST_DIR}/pa_factory.cpp ${CMAKE_CURRENT_LIST_DIR}/pa_factory.h
        )

