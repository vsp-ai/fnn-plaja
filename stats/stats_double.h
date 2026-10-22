//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_DOUBLE_H
#define PLAJA_STATS_DOUBLE_H

#include "../include/ct_config_const.h"

namespace PLAJA {

    enum class StatsDouble {

        // SearchStatistics
        PREPROCESSING_TIME,  // parsing + checks
        CONSTRUCTION_TIME,   // time to construct engine
        INITIALIZATION_TIME, // initialization time of the engine
        SEARCH_TIME,         // (primary) computational time spent by the engine
        FINALIZATION_TIME,   // time spent by the engine "finalizing", e.g., analyzing main results

        // Policy evaluation
        LAST_SCORE,
        LAST_AVERAGE_LENGTH,

#ifdef BUILD_PA
        // SearchStatisticsCegar
        INCREMENTATION_TIME,
        TIME_FOR_PA,       // time spent in sub-engines computing PA
        AvDistanceWsState, // Average of normalized distance between witness and concretization value on all variables.
        AvDistanceWsStateCandidates, // Average of normalized distance between witness and concretization value on distinct/candidate variables.
        // AvDistanceWs,                // Average of normalized size of witness split.
        AvPertBound,  // Average perturbation for variable selection
        RelevantVars, // Average fraction of relevant/irrelevant variable for variable selection.
        IrrelevantVars,

        // SearchStatisticsPA
        PA_SS_REFINE_TIME,
        PA_SUC_GEN_EXPLORE_TIME,
        PA_ACTION_LABEL_IT_TIME,
        PA_ACTION_OP_IT_TIME,
        PA_UPDATE_IT_TIME,
        PA_UPD_ENTAILMENT_TIME,
        PA_SUCCESSOR_ENUM_TIME,
        //
        TIME_APP_TEST,
        TIME_TRANSITION_TEST,
        TIME_NN_SELECTION_TEST,
        TIME_NN_APP_TEST,
        TIME_NN_TRANSITION_TEST,
        //
        PA_PUSH_POP_SRC_TIME,
        PA_PUSH_POP_ACTION_LABEL_TIME,
        PA_PUSH_POP_ACTION_OP_TIME,
        PA_PUSH_POP_UPD_TIME,

        // StatisticsHeuristic (PA)
        HEURISTIC_COMPUTATION_TIME,

        // MATCH TREE
        MT_AV_DEPTH,

#endif

#ifdef USE_Z3
        // StatisticsSMTSolver (Z3)
        TIME_Z3,
        TIME_Z3_UNSAT,
        TIME_Z3_PUSH,
        TIME_Z3_POP,
        TIME_Z3_RESET,
#endif

#ifdef SUPPORT_GUROBI
        TIME_MILP,
#endif

#ifdef USE_MARABOU
        // StatisticsSMTSolverMarabou
        TIME_MARABOU_PREPROCESS,
        TIME_MARABOU_PREPROCESS_MAX,
        TIME_MARABOU_FOR_PREPROCESSED_UNSAT,
        TIME_MARABOU_PREPROCESS_UNSAT,
        TIME_MARABOU,
        TIME_MARABOU_MAX,
        TIME_MARABOU_UNSAT,
        TIME_MARABOU_BB,
        TIME_MARABOU_BB_UNSAT,
        TIME_MARABOU_PUSH,
        TIME_MARABOU_POP,
        //
        MARABOU_BB_NON_EXACT_INT_DELTA,
#endif


    };

} // namespace PLAJA

#endif //PLAJA_STATS_DOUBLE_H
