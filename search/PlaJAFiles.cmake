set(SEARCH_SOURCES)

# information
include(${CMAKE_CURRENT_LIST_DIR}/information/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${INFORMATION_SOURCES})

# states
include(${CMAKE_CURRENT_LIST_DIR}/states/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${STATES_SOURCES})

# fd adaptions
include(${CMAKE_CURRENT_LIST_DIR}/fd_adaptions/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${FD_ADAPTIONS_SOURCES})

if (${USE_Z3})
    include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SMT_SOURCES})
endif (${USE_Z3})

if (${USE_MARABOU})
    include(${CMAKE_CURRENT_LIST_DIR}/smt_nn/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${SMT_NN_SOURCES})
endif (${USE_MARABOU})

# successor generation
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${SUCCESSOR_GENERATION_SOURCES})

# non-probabilistic search
include(${CMAKE_CURRENT_LIST_DIR}/non_prob_search/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${NON_PROB_SEARCH_SOURCES})

# predicate abstraction
if (${BUILD_PA})
    include(${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction/PlaJAFiles.cmake)
    list(APPEND SEARCH_SOURCES ${PREDICATE_ABSTRACTION_SOURCES})
endif (${BUILD_PA})

# factories
include(${CMAKE_CURRENT_LIST_DIR}/factories/PlaJAFiles.cmake)
list(APPEND SEARCH_SOURCES ${FACTORY_SOURCES})

