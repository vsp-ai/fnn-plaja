//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_statistics_nn_sat.h"
#include "../../../include/ct_config_const.h"
#include "../../../stats/stats_base.h"

// SearchStatisticsNNSat::SearchStatisticsNNSat(): smtStatsZ3(nullptr), smtStatsMarabou(nullptr) {
void NN_SAT::add_stats(PLAJA::StatsBase& stats) {

    stats.add_unsigned_stats(
        std::list<PLAJA::StatsUnsigned> {
            PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3, PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNSAT, PLAJA::StatsUnsigned::NN_SAT_QUERIES_Z3_UNDECIDED,
            PLAJA::StatsUnsigned::PB_QUERIES, PLAJA::StatsUnsigned::PB_FRESH_QUERIES, PLAJA::StatsUnsigned::PB_INCREMENTAL_QUERIES,
            PLAJA::StatsUnsigned::PB_FORMULA_BUILDS, PLAJA::StatsUnsigned::PB_CACHE_HITS
        },
        std::list<std::string> {
            "NNSatQueriesZ3", "NNSatQueriesZ3Unsat", "NNSatQueriesZ3Undecided",
            "PBQueries", "PBFreshQueries", "PBIncrementalQueries", "PBFormulaBuilds", "PBCacheHits"
        },
        0
    );

}

#if 0
SearchStatisticsNNSat::~SearchStatisticsNNSat() = default;

//

void SearchStatisticsNNSat::print_statistics_specific() const {
    // auxiliary outputs:
}
#endif
