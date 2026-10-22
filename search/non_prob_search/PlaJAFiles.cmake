set(NON_PROB_SEARCH_SOURCES)

# policy
include(${CMAKE_CURRENT_LIST_DIR}/policy/PlaJAFiles.cmake)
list(APPEND NON_PROB_SEARCH_SOURCES ${POLICY_SOURCES})

list(APPEND NON_PROB_SEARCH_SOURCES
    # Added here on purpose:
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_base.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_base.h
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_naive.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/solution_enumerator_naive.h
    ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/state_values_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/../smt/solution_enumerator/state_values_enumerator.h
    #
    ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/initial_states_enumerator.h
)
