set(ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.h
)

include(${CMAKE_CURRENT_LIST_DIR}/evaluation/PlaJAFiles.cmake)
list(APPEND ENGINE_SOURCES ${EVALUATION_SOURCES})

if (${BUILD_PA})
    include(${CMAKE_CURRENT_LIST_DIR}/verification/PlaJAFiles.cmake)
    list(APPEND ENGINE_SOURCES ${VERIFICATION_SOURCES})
endif ()
