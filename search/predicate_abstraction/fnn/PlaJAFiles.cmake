set(PA_FNN_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_z3.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_z3.h
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_pb.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_pb.h
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_factory.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_factory.h
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_formula.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_formula.h
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_ipasir_solver.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_ipasir_solver.h
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_query_builder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_query_builder.h
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pb_constraints/pb_engine.h
)

if (${SUPPORT_GUROBI})
    list(APPEND PA_FNN_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_milp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/fnn_sat_checker_milp.h
        ${CMAKE_CURRENT_LIST_DIR}/milp/z3_query_to_gurobi.cpp
        ${CMAKE_CURRENT_LIST_DIR}/milp/z3_query_to_gurobi.h
    )
endif (${SUPPORT_GUROBI})
