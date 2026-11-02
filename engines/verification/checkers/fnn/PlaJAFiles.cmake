set(PA_FNN_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_z3.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_z3.h
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_pb.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_pb.h
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_factory.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_factory.h
)

if (${SUPPORT_GUROBI})
    list(APPEND PA_FNN_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_milp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_checker_milp.h
    )
endif ()
