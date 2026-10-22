set(PLAJA_SOURCES)

set(PLAJA_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/globals.cpp ${CMAKE_CURRENT_LIST_DIR}/globals.h
)

# policy evaluation
include(${CMAKE_CURRENT_LIST_DIR}/policy_evaluation/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${POLICY_EVALUATION_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/utils/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${UTILS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/stats/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${STATS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/exception/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${EXCEPTION_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/option_parser/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${OPTION_PARSER_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/parser/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${PARSER_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/search/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${SEARCH_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/domain_specific/PlaJAFiles.cmake)
list(APPEND PLAJA_SOURCES ${DOMAIN_SPECIFIC_SOURCES})

# Information taken from a Fast Downward code base (http://gogs.cs.uni-saarland.de:3000/fai_group/FD-Stripped/src/master/src/search/DownwardFiles.cmake [October 2018]) and freely adapted to PLAJA:
# The order in PLAJA_SOURCES influences the order in which object files are given to the linker.
# This can have a significant influence on the performance (cf. issue67 FastDownward).
# According to FastDownward, the general recommendation is to list files that define functions after files that use them.
# We approximate this recommendation by reversing PLAJA_SOURCES.
# Thereby, in particular, the "core" implementation files in SEARCH_SOURCES will be listed first, followed by the "supporting" files in, e.g., PARSER_SOURCES.
# Also according to FastDownward and in accordance with our observations, this approximation works well enough in practice.
list(REVERSE PLAJA_SOURCES)

