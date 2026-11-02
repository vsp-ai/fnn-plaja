//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "core/search_statistics.h"
#include "core/stats/stats_base.h"

void SEARCH_STATISTICS::add_stats(PLAJA::StatsBase& stats) {

    stats.add_int_stats(PLAJA::StatsInt::STATUS, "Status", -1);

    stats.add_double_stats(
        std::list<PLAJA::StatsDouble> { PLAJA::StatsDouble::PREPROCESSING_TIME, PLAJA::StatsDouble::CONSTRUCTION_TIME, PLAJA::StatsDouble::INITIALIZATION_TIME, PLAJA::StatsDouble::SEARCH_TIME, PLAJA::StatsDouble::FINALIZATION_TIME },
        std::list<std::string> { "PreprocessingTime", "ConstructionTime", "InitializationTime", "SearchTime", "FinalizationTime" },
        0
    );

}

void SEARCH_STATISTICS::add_basic_stats(PLAJA::StatsBase& stats) {

    add_stats(stats);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> { PLAJA::StatsUnsigned::EXPANDED_STATES, PLAJA::StatsUnsigned::GENERATED_STATES,
                                          PLAJA::StatsUnsigned::ACTION_LABELS, PLAJA::StatsUnsigned::ACTION_OPS, PLAJA::StatsUnsigned::TRANSITIONS,
                                          PLAJA::StatsUnsigned::TERMINAL_STATES, PLAJA::StatsUnsigned::POLICY_TERMINAL_STATES,
                                          PLAJA::StatsUnsigned::START_STATES, PLAJA::StatsUnsigned::REACHABLE_STATES, PLAJA::StatsUnsigned::GOAL_STATES, PLAJA::StatsUnsigned::DEAD_END_STATES },
        std::list<std::string> { "ExpandedStates", "GeneratedStates", "ActionLabels", "ActionOps", "Transitions", "TerminalStates", "PolicyTerminalStates", "StartStates", "ReachableStates", "GoalStates", "DeadEndStates" },
        0
    );

}
