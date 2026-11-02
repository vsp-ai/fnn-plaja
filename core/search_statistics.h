//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_STATISTICS_H
#define PLAJA_SEARCH_STATISTICS_H

/*
 * PlaJA: Developed out of a FastDownward class (SearchProgress, search_progress.{h,cc}).
 * Placed here for legacy reasons.
 */

namespace PLAJA { class StatsBase; }

namespace SEARCH_STATISTICS {

    /**
     * Add minimal statistics used by the parent search engine.
     * @param stats
     */
    extern void add_stats(PLAJA::StatsBase& stats);

    /**
     * This add basic stats attributes, e.g., for expanded/generated states, ...
     * (Formally contained in SearchProgress, res. search_progress.{h,cc}.)
     */
    extern void add_basic_stats(PLAJA::StatsBase& stats);

}

#endif //PLAJA_SEARCH_STATISTICS_H
