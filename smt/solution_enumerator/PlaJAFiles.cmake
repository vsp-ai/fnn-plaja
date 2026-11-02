#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2024) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

# built without Z3 too
set(SOLUTION_ENUMERATOR_BASE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_base.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_base.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_naive.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_naive.h
    ${CMAKE_CURRENT_LIST_DIR}/state_values_enumerator.cpp ${CMAKE_CURRENT_LIST_DIR}/state_values_enumerator.h
)

set(SOLUTION_ENUMERATOR_Z3_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_z3.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_z3.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_bs.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_bs.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_td.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_enumerator_td.h
    ${CMAKE_CURRENT_LIST_DIR}/solution_sampler.cpp ${CMAKE_CURRENT_LIST_DIR}/solution_sampler.h

)