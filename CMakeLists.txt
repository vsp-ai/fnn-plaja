cmake_minimum_required(VERSION 3.10)
project(PlaJA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Modules ##############################################################################################################

SET(BUILD_PA ON) # PREDICATE ABSTRACTION
option(BUILD_TESTS "Build the PB unit tests" OFF)

option(BUILD_PB_CONSTRAINTS "Build support for FNN verification with pseudo-boolean constraints" OFF)

# Dependencies #########################################################################################################
# (set non-existing path to disable)
# (conversely may also try set to any valid path to enable flag while relying on cmake to find actual dependency path) #

set(DEPENDENCIES_PATH /workspace/dependencies/)

if (NOT DEFINED DEPENDENCIES_PATH OR NOT EXISTS ${DEPENDENCIES_PATH})
    if (DEFINED DEPENDENCIES_PATH AND NOT EXISTS ${DEPENDENCIES_PATH})
        message(WARNING "Dependencies path does not exist or not specified (${DEPENDENCIES_PATH}) ... using default.")
        message(WARNING "Hint: You must specify total path.")
    endif ()
    set(DEPENDENCIES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/)
    # external path as default for convenience
    # local fall-back default installation path (${CMAKE_CURRENT_SOURCE_DIR}/dependencies) would ne more naturally
endif ()


set(NLOHMANN_PATH ${DEPENDENCIES_PATH}/json/single_include/nlohmann)
set(CXXTEST_PATH ${DEPENDENCIES_PATH}/cxxtest) # Warning: This is required for includes of Marabou!
set(Z3_DIR ${DEPENDENCIES_PATH}/z3)
set(Z3_BUILD_DIR ${Z3_DIR}/build)
# Marabou-related
set(Marabou_DIR ${DEPENDENCIES_PATH}/Marabou)
set(Marabou_BUILD_DIR ${Marabou_DIR}/build)
#
set(Openblas_DIR ${Marabou_DIR}/tools/OpenBLAS-0.3.19/installed/lib) # Marabou default installation [June 2022] (local)

#

if (NOT DEFINED ROUNDINGSAT_DIR)
    set(ROUNDINGSAT_DIR ${DEPENDENCIES_PATH}/roundingsat)
endif ()

if (NOT DEFINED ROUNDINGSAT_BUILD_DIR)
    set(ROUNDINGSAT_BUILD_DIR ${ROUNDINGSAT_DIR}/build)
endif ()

set(GUROBI_DIR /workspace/dependencies/gurobi/linux)

set(SUPPORT_GUROBI TRUE)
#
set(Boost_NO_SYSTEM_PATHS TRUE)
set(Boost_DIR ${Marabou_DIR}/tools/boost_1_81_0/installed/lib/cmake/Boost-1.81.0)
#

# Compile time options #################################################################################################

include(${CMAKE_CURRENT_SOURCE_DIR}/COptionsConfig.cmake)

########################################################################################################################

# Allow to specify modules, dependencies paths and compile time options via external file.
# Simplifies developer usage in combination with git ...
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external_config.cmake)
    set(DEPENDENCIES_PATH_COPY ${DEPENDENCIES_PATH})
    include(${CMAKE_CURRENT_SOURCE_DIR}/external_config.cmake)
    if (${DEPENDENCIES_PATH_COPY} STREQUAL ${DEPENDENCIES_PATH})
    else ()
        message(WARNING "Updated DEPENDENCIES_PATH via external_config.cmake (\"${DEPENDENCIES_PATH_COPY}\" -> \"${DEPENDENCIES_PATH}\"). This may cause unintended behavior. DEPENDENCIES_PATH should be set as CMake option via commandline.")
    endif ()
endif ()

# Targets
set(PLAJA PlaJA) # the main executable

# Dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/DependenciesConfig.cmake)
# set preprocessor flags for source files:
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/compiler_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler_config.h) # compiler flags
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/factory_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/factory_config.h) # submodules
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/marabou_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/marabou_config.h) # compile time options (PA)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/pa_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/pa_config.h) # compile time options (PA)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/ct_config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/ct_config.h) # compile time options (PA)

# Sources
include(${CMAKE_CURRENT_SOURCE_DIR}/PlaJAFiles.cmake)



# Only Release builds are supported.
if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(FATAL_ERROR "Only CMAKE_BUILD_TYPE=Release is supported.")
endif ()

# PlaJA
add_executable(${PLAJA} ${PLAJA_SOURCES} main.cpp)
target_link_libraries(${PLAJA} ${LIBS})
target_include_directories(${PLAJA} PRIVATE ${INCLUDES})

# PB unit tests (standalone, only the PB sources they test).
if (${BUILD_TESTS} AND ${USE_ROUNDINGSAT})
    enable_testing()
    add_executable(pb_ipasir_test
        tests/predicate_abstraction/pb_ipasir_test.cpp
        search/predicate_abstraction/fnn/pb_constraints/pb_formula.cpp
        search/predicate_abstraction/fnn/pb_constraints/pb_ipasir_solver.cpp)
    target_link_libraries(pb_ipasir_test ${ROUNDINGSAT_LIBRARY})
    target_include_directories(pb_ipasir_test PRIVATE ${INCLUDES})
    add_test(NAME pb_ipasir_test COMMAND pb_ipasir_test)
endif ()

if (${BUILD_TESTS})
    enable_testing()
    add_executable(pb_formula_test
        tests/predicate_abstraction/pb_formula_test.cpp
        search/predicate_abstraction/fnn/pb_constraints/pb_formula.cpp)
    target_include_directories(pb_formula_test PRIVATE ${INCLUDES})
    add_test(NAME pb_formula_test COMMAND pb_formula_test)
endif ()
