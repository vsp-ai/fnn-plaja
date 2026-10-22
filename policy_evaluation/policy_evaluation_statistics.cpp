#include "policy_evaluation_statistics.h"

#include "../search/fd_adaptions/search_statistics.h"  // SEARCH_STATISTICS::add_basic_stats

void POLICY_EVALUATION_STATS::add_basic_stats(PLAJA::StatsBase& stats) {

    // StartStates, ExpandedStates, GeneratedStates, Transitions, TerminalStates, GoalStates, etc.
    SEARCH_STATISTICS::add_basic_stats(stats);

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned>{
            PLAJA::StatsUnsigned::EPISODES,
            PLAJA::StatsUnsigned::POLICY_ACTS,
            PLAJA::StatsUnsigned::DONE_GOAL,
            PLAJA::StatsUnsigned::DONE_AVOID,
            PLAJA::StatsUnsigned::UNDONE,
            PLAJA::StatsUnsigned::DETECTED_CYCLES,
            PLAJA::StatsUnsigned::DONE_STALLING,
            PLAJA::StatsUnsigned::TERMINAL_STATES,
            PLAJA::StatsUnsigned::TRANSITIONS
        },
        std::list<std::string>{
            "Episodes",
            "PolicyActs",
            "Goal",
            "Failure",
            "Undone",
            "DetectedCycles",
            "DeadEnd",
            "TerminalStates",
            "Transitions (total steps)"
        },
        0
    );

}
