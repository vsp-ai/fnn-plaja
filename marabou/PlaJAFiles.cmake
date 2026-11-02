set(MARABOU_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/marabou_context.cpp ${CMAKE_CURRENT_LIST_DIR}/marabou_context.h
        ${CMAKE_CURRENT_LIST_DIR}/marabou_query.cpp ${CMAKE_CURRENT_LIST_DIR}/marabou_query.h
        ${CMAKE_CURRENT_LIST_DIR}/vars_in_marabou.cpp ${CMAKE_CURRENT_LIST_DIR}/vars_in_marabou.h
        ${CMAKE_CURRENT_LIST_DIR}/constraints_in_marabou.cpp ${CMAKE_CURRENT_LIST_DIR}/constraints_in_marabou.h
        ${CMAKE_CURRENT_LIST_DIR}/output_constraints_in_marabou.cpp ${CMAKE_CURRENT_LIST_DIR}/output_constraints_in_marabou.h
        ${CMAKE_CURRENT_LIST_DIR}/nn_in_marabou.cpp ${CMAKE_CURRENT_LIST_DIR}/nn_in_marabou.h
        )

# utils
include(${CMAKE_CURRENT_LIST_DIR}/utils/PlaJAFiles.cmake)
list(APPEND MARABOU_SOURCES ${MARABOU_UTILS_SOURCES})

# model
include(${CMAKE_CURRENT_LIST_DIR}/model/PlaJAFiles.cmake)
list(APPEND MARABOU_SOURCES ${MARABOU_MODEL_SOURCES})

# visitor
include(${CMAKE_CURRENT_LIST_DIR}/visitor/PlaJAFiles.cmake)
list(APPEND MARABOU_SOURCES ${MARABOU_VISITOR_SOURCES})

# successor generation
include(${CMAKE_CURRENT_LIST_DIR}/successor_generation/PlaJAFiles.cmake)
list(APPEND MARABOU_SOURCES ${MARABOU_SUCCESSOR_GENERATION_SOURCES})

# solver
include(${CMAKE_CURRENT_LIST_DIR}/solver/PlaJAFiles.cmake)
list(APPEND MARABOU_SOURCES ${MARABOU_SOLVER_SOURCES})
