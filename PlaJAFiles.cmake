set(PLAJA_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/globals.cpp ${CMAKE_CURRENT_LIST_DIR}/globals.h
)

include(${CMAKE_CURRENT_LIST_DIR}/core/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${CORE_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/model/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${MODEL_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${SUCCESSOR_GENERATION_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/policies/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${POLICY_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${SOLUTION_ENUMERATOR_BASE_SOURCES})
if (${USE_Z3})
    list(APPEND PLAJA_SOURCES ${SMT_SOURCES})
endif ()

if (${USE_MARABOU})
    include(${CMAKE_CURRENT_LIST_DIR}/marabou/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${MARABOU_SOURCES})
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/pseudo_boolean/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${PSEUDO_BOOLEAN_SOURCES})

if (${SUPPORT_GUROBI} AND ${USE_Z3})
    include(${CMAKE_CURRENT_LIST_DIR}/milp/PlaJAFiles.cmake)
    list(APPEND PLAJA_SOURCES ${MILP_SOURCES})
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/engines/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${ENGINE_SOURCES})

# Information taken from a Fast Downward code base (http://gogs.cs.uni-saarland.de:3000/fai_group/FD-Stripped/src/master/src/search/DownwardFiles.cmake [October 2018]) and freely adapted to PLAJA:
# The order in PLAJA_SOURCES influences the order in which object files are given to the linker.
# This can have a significant influence on the performance (cf. issue67 FastDownward).
# According to FastDownward, the general recommendation is to list files that define functions after files that use them.
# We approximate this recommendation by reversing PLAJA_SOURCES.
# Thereby, in particular, the engine files in ENGINE_SOURCES will be listed first, followed by the "supporting" files in, e.g., MODEL_SOURCES.
# Also according to FastDownward and in accordance with our observations, this approximation works well enough in practice.
list(REVERSE PLAJA_SOURCES)

