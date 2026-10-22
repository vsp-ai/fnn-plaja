# nn sat in z3
include(${CMAKE_CURRENT_LIST_DIR}/smt/PlaJAFiles.cmake)

# nn sat checker
include(${CMAKE_CURRENT_LIST_DIR}/nn_sat_checker/PlaJAFiles.cmake)

# nn sat in z3
include(${CMAKE_CURRENT_LIST_DIR}/nn_sat_in_z3/PlaJAFiles.cmake)

set(PA_NN_SOURCES
        ${SMT_PA_NN_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/search_statistics_nn_sat.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics_nn_sat.h
        ${PA_NN_SAT_CHECKER_SOURCES}
        ${PA_NN_SAT_IN_Z3_SOURCES}
        )
