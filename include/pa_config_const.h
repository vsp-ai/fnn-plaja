//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_CONFIG_CONST_H
#define PLAJA_PA_CONFIG_CONST_H

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include "pa_config.h"

/** const expr in C++ for easier usage */

namespace PLAJA_GLOBAL {

#ifdef LAZY_PA

    constexpr bool lazyPA = true;

#define FIELD_IF_LAZY_PA(FCT) FCT
#define FCT_IF_LAZY_PA(FCT) FCT
#define STMT_IF_LAZY_PA(STMT) STMT;
#define CONSTRUCT_IF_LAZY_PA(CONSTRUCT) ,CONSTRUCT

#else
    constexpr bool lazyPA = false;

#define FIELD_IF_LAZY_PA(FCT)
#define FCT_IF_LAZY_PA(FCT)
#define STMT_IF_LAZY_PA(STMT)
#define CONSTRUCT_IF_LAZY_PA(CONSTRUCT)

#endif

#ifdef ENABLE_HEURISTIC_SEARCH_PA
    constexpr bool enableHeuristicSearchPA = true;
#else
    constexpr bool enableHeuristicSearchPA = false;
#endif

#ifdef QUERY_PER_BRANCH_INIT
    constexpr bool queryPerBranchInit = true;
#else
    constexpr bool queryPerBranchInit = false;
#endif

#ifdef QUERY_PER_BRANCH_EXPANSION
    constexpr bool queryPerBranchExpansion = true;
#else
    constexpr bool queryPerBranchExpansion = false;
#endif

    /******************************************************************************************************************/

#ifdef PA_ONLY_REACHABILITY
    constexpr bool paOnlyReachability = true;
#else
    constexpr bool paOnlyReachability = false;
#endif

#ifdef PA_ONLY_STEP_REACHABILITY
    constexpr bool paOnlyStepReachability = true;
#else
    constexpr bool paOnlyStepReachability = false;
#endif

#ifdef PA_ONLY_STEP_REACHABILITY_PER_LABEL
    constexpr bool paOnlyStepReachabilityPerLabel = true;
#else
    constexpr bool paOnlyStepReachabilityPerLabel = false;
#endif

#ifdef PA_ONLY_STEP_REACHABILITY_PER_OPERATOR
    constexpr bool paOnlyStepReachabilityPerOperator = true;
#else
    constexpr bool paOnlyStepReachabilityPerOperator = false;
#endif

    static_assert(paOnlyReachability + paOnlyStepReachability + paOnlyStepReachabilityPerLabel + paOnlyStepReachabilityPerOperator == 1);
    constexpr bool paCacheSuccessors = paOnlyStepReachability || paOnlyStepReachabilityPerLabel || paOnlyStepReachabilityPerOperator;

    /******************************************************************************************************************/

#ifdef REUSE_SOLUTIONS
    constexpr bool reuseSolutions = true;
#else
    constexpr bool reuseSolutions = false;
#endif
    static_assert(reuseSolutions); // assumption for now, related to incremental search option

#ifdef ROUND_RELAXED_SOLUTIONS
    constexpr bool roundRelaxedSolutions = true;
#else
    constexpr bool roundRelaxedSolutions = false;
#endif

#ifdef DO_SELECTION_TEST
    constexpr bool doSelectionTest = true;
#else
    constexpr bool doSelectionTest = false;
#endif

#ifdef DO_NN_APPLICABILITY_TEST
    constexpr bool doNNApplicabilityTest = true;
#else
    constexpr bool doNNApplicabilityTest = false;
#endif

#ifdef DO_RELAXED_SELECTION_TEST_ONLY
    constexpr bool doRelaxedSelectionTestOnly = true;
#else
    constexpr bool doRelaxedSelectionTestOnly = false;
#endif

#ifdef DO_RELAXED_NN_APPLICABILITY_TEST_ONLY
    constexpr bool doRelaxedNNApplicabilityTestOnly = true;
#else
    constexpr bool doRelaxedNNApplicabilityTestOnly = false;
#endif

    // maintain solutions checker if reusing solutions or rounding solutions of relaxed queries
    constexpr bool maintainSolutionsChecker = reuseSolutions or roundRelaxedSolutions;

    // always rounding solutions of relaxed queries if reusing solutions
    static_assert(not reuseSolutions or roundRelaxedSolutions);

    /******************************************************************************************************************/

    // not set via cmake
#define USE_MATCH_TREE

#ifdef USE_MATCH_TREE
    constexpr bool useIncMt = true;
#else
    constexpr bool useIncMt = false;
#endif

    constexpr bool useRefReg = false;

    static_assert(useIncMt);
    static_assert(not useRefReg or not lazyPA);

}

#pragma GCC diagnostic pop

#endif //PLAJA_PA_CONFIG_CONST_H