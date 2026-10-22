//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_UNSIGNED_H
#define PLAJA_STATS_UNSIGNED_H

#include "../include/ct_config_const.h"

namespace PLAJA {

    enum class StatsUnsigned {

        // SearchStatisticsBasic
        EXPANDED_STATES,
        GENERATED_STATES,
        ACTION_LABELS, // nr of (applicable) action (labels) accumulated over states
        ACTION_OPS,    // nr of (applicable) action operators accumulated over states
        TRANSITIONS,    // total transitions (counting deterministic ones)
        TERMINAL_STATES, // nr states without outgoing transition
        POLICY_TERMINAL_STATES,
        START_STATES,
        REACHABLE_STATES,
        GOAL_STATES,
        DEAD_END_STATES,

        // Policy evaluation
        EPISODES,
        POLICY_ACTS, // Excluding deterministic state transitions.
        DONE_GOAL,
        DONE_AVOID,
        DONE_STALLING,
        UNDONE,
        DETECTED_CYCLES,

#ifdef BUILD_PA
        // SearchStatisticsCegar
        ITERATIONS,
        REF_DUE_TO_CONCRETIZATION, // concrete path state is not a concretization of abstract path state
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
        REF_DUE_TO_TERMINAL, // non-terminal does not hold
#endif
        REF_DUE_TO_GUARD_APP,          // guard not applicable
        REF_DUE_TO_ACTION_SEL,         // action not selected
        REF_DUE_TO_TARGET,             // final path state is not a reachability target
        GLOBAL_PREDICATES_ADDED,       // may exceed total number of preds for lazy pa
        LOCAL_PREDICATES_ADDED,        // may exceed total number of preds for lazy pa
        PREDICATES_FOR_ACTION_SEL_REF, // predicates per refinement

        WsFallBack,     // Perform witness-agnostic refinement due to missing witness.
        WsCandidates,   // Accumulated number of candidate variables for witness splitting.
        WsVars,         // Accumulated number of used split vars.
        EntailedSplits, // Accumulated number of variables split at path-prefix entailed value.
        // RefWithEntailment,              // Accumulated number of refinement using entailment.

        // SearchStatisticsPA
        PREDICATES,

    // SearchStatisticsPATest
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
        TERMINAL_TESTS_SAT,
#endif
        APP_TESTS,
        APP_TESTS_UNSAT,
        TRANSITION_TESTS,
        TRANSITION_TESTS_UNSAT,
        NN_SELECTION_TESTS,
        NN_SELECTION_TESTS_UNSAT,
        NN_APP_TESTS,
        NN_APP_TESTS_UNSAT,
        NN_TRANSITION_TESTS,
        NN_TRANSITION_TESTS_UNSAT,
        NN_STALLING_TESTS,
        NN_STALLING_TESTS_UNSAT,
        REUSED_SOLUTIONS,            // solutions successfully reused from previous tests
        REUSED_TRANSITION_EXISTENCE, // solutions successfully reused from iterations (for cegar)
        REUSED_TRANSITION_EXCLUSION,
        TRANSITION_WITHOUT_WITNESS,

        // StatisticsHeuristic (PA)
        HEURISTIC_CALLS,
        HEURISTIC_CACHE_HITS, // reusing heuristic estimates already computed
        UPDATED_ESTIMATES,    // heuristic estimates may be updated/improved

        // MATCH TREE
        MT_SIZE,
        MT_MAX_DEPTH,

#endif

#ifdef USE_Z3
        // StatisticsSMTSolver (Z3)
        Z3_QUERIES,
        Z3_QUERIES_UNSAT,
        Z3_QUERIES_UNDECIDED, // SMT queries, that could not be decided by the underlying SMT-solver
#endif

#ifdef SUPPORT_GUROBI
        MILP_QUERIES,
        MILP_QUERIES_UNSAT,
        MILP_WITNESS_CHECKS,
#endif

#ifdef USE_MARABOU
        // StatisticsSMTSolverMarabou
        MARABOU_PREPROCESSED_UNSAT, // Queries inferred to be unsat during pre-processing (not counted as marabou queries).
        MARABOU_QUERIES,
        MARABOU_QUERIES_UNSAT,
        MARABOU_QUERIES_UNDECIDED, // Queries that could not be decided.
        MARABOU_INVALID_SOLUTIONS, // Queries sat but with an invalid-solution (in release mode we only check OOB). This is possible due to numeric issues.
        MARABOU_NUM_SPLITS,          // Marabou internal
        MARABOU_NUM_POPS,            // Marabou internal
        MARABOU_MAX_SPLIT_DEPTH,     // Marabou internal
        MARABOU_LEARNED_CONFLICTS,   // PlaJA-Marabou internal
        MARABOU_CASE_SPLITS_SKIPPED, // PlaJA-Marabou internal
        MARABOU_CASE_SPLITS_FIXED,   // PlaJA-Marabou internal
        MARABOU_BB,
        MARABOU_BB_UNSAT,
        MARABOU_BB_EXACT_WITH_ARGMAX_TOLERANCE, // Marabou BB concluded with exact solution up to argmax tolerance.
        MARABOU_BB_NON_EXACT,     // Marabou in BB concluded sat with non-exact solution (due to floating point).
        MARABOU_BB_NO_SOLUTION,   // Marabou in BB concluded sat without solution.
        MARABOU_BB_NON_EXACT_INT, // Marabou queries with non-exact integer solution (during BB).
        MARABOU_BB_BRANCH_OOB,    // Branching over variable with out-of-bound value in current solution (during BB)
        MARABOU_BB_BRANCH_EXACT_INT, // Branching over variable with exact integer value (floor == ceil) in current solution (during BB)
        MARABOU_BB_RECURSIONS,
        MARABOU_BB_MAX_DEPTH,
#endif

        // SearchStatisticsNNSat
        NN_SAT_QUERIES_Z3,
        NN_SAT_QUERIES_Z3_UNSAT,
        NN_SAT_QUERIES_Z3_UNDECIDED,
        PB_QUERIES,
        PB_FRESH_QUERIES,
        PB_INCREMENTAL_QUERIES,
        PB_FORMULA_BUILDS,
        PB_CACHE_HITS,

    };

} // namespace PLAJA

#endif //PLAJA_STATS_UNSIGNED_H
