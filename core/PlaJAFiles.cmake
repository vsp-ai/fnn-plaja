set(CORE_SOURCES)

include(${CMAKE_CURRENT_LIST_DIR}/utils/PlaJAFiles.cmake)
list(APPEND CORE_SOURCES ${UTILS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/stats/PlaJAFiles.cmake)
list(APPEND CORE_SOURCES ${STATS_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/exception/PlaJAFiles.cmake)
list(APPEND CORE_SOURCES ${EXCEPTION_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/option_parser/PlaJAFiles.cmake)
list(APPEND CORE_SOURCES ${OPTION_PARSER_SOURCES})

list(APPEND CORE_SOURCES
    # states
    ${CMAKE_CURRENT_LIST_DIR}/state_base.cpp ${CMAKE_CURRENT_LIST_DIR}/state_base.h
    ${CMAKE_CURRENT_LIST_DIR}/state_values.cpp ${CMAKE_CURRENT_LIST_DIR}/state_values.h
    # Fast Downward; non-alphabetic order is intended (see PlaJAFiles.cmake in the root folder)
    ${CMAKE_CURRENT_LIST_DIR}/timer.cpp ${CMAKE_CURRENT_LIST_DIR}/timer.h
    ${CMAKE_CURRENT_LIST_DIR}/search_statistics.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics.h
    ${CMAKE_CURRENT_LIST_DIR}/int_packer.cc ${CMAKE_CURRENT_LIST_DIR}/int_packer.h
    ${CMAKE_CURRENT_LIST_DIR}/state_registry.cc ${CMAKE_CURRENT_LIST_DIR}/state_registry.h
    ${CMAKE_CURRENT_LIST_DIR}/state.cc ${CMAKE_CURRENT_LIST_DIR}/state.h
    ${CMAKE_CURRENT_LIST_DIR}/search_engine.cc ${CMAKE_CURRENT_LIST_DIR}/search_engine.h
    # factories
    ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.cpp ${CMAKE_CURRENT_LIST_DIR}/sharable_structure.h
    ${CMAKE_CURRENT_LIST_DIR}/configuration.cpp ${CMAKE_CURRENT_LIST_DIR}/configuration.h
    ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.cpp ${CMAKE_CURRENT_LIST_DIR}/search_engine_factory.h
)
