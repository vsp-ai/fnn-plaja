//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_INT_H
#define PLAJA_STATS_INT_H


namespace PLAJA {

enum class StatsInt {

    // SearchStatistics
    STATUS,

    // SearchStatisticsCegar
    HAS_GOAL_PATH,
    SPURIOUS_PREFIX_LENGTH,

    // SearchStatisticsPA
    PATH_LENGTH,

};

}


#endif //PLAJA_STATS_INT_H
