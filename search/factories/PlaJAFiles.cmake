set(FACTORY_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.cpp ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.h
        ${CMAKE_CURRENT_LIST_DIR}/configuration.cpp ${CMAKE_CURRENT_LIST_DIR}/configuration.h
        ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.cpp ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.h
        )

include(${CMAKE_CURRENT_LIST_DIR}/policy_evaluation/PlaJAFiles.cmake)
list(APPEND FACTORY_SOURCES ${POLICY_EVALUATION_FACTORY_SOURCES})

if (${USE_Z3} OR ${USE_MARABOU})
include(${CMAKE_CURRENT_LIST_DIR}/smt_base/PlaJAFiles.cmake)
list(APPEND FACTORY_SOURCES ${SMT_FACTORY_SOURCES})
endif ()

if (${BUILD_PA})
    include(${CMAKE_CURRENT_LIST_DIR}/predicate_abstraction/PlaJAFiles.cmake)
    list(APPEND FACTORY_SOURCES ${PA_FACTORY_SOURCES})
endif (${BUILD_PA})

