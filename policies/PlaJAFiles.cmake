set(POLICY_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/policy.h
        ${CMAKE_CURRENT_LIST_DIR}/policy_restriction.cpp ${CMAKE_CURRENT_LIST_DIR}/policy_restriction.h
        )

include(${CMAKE_CURRENT_LIST_DIR}/interface/PlaJAFiles.cmake)
list(APPEND POLICY_SOURCES ${POLICY_INTERFACE_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/nn/PlaJAFiles.cmake)
list(APPEND POLICY_SOURCES ${NN_POLICY_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/fnn/PlaJAFiles.cmake)
list(APPEND POLICY_SOURCES ${FNN_POLICY_SOURCES})

include(${CMAKE_CURRENT_LIST_DIR}/expert/PlaJAFiles.cmake)
list(APPEND POLICY_SOURCES ${EXPERT_POLICY_SOURCES})
