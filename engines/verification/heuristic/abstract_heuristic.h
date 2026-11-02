//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ABSTRACT_HEURISTIC_H
#define PLAJA_ABSTRACT_HEURISTIC_H

#include <memory>
#include "core/stats/stats_interface.h"
#include "core/utils/default_constructors.h"
#include "core/using_core.h"

/**********************************************************************************************************************/

class AbstractState;

class HeuristicStateQueue;

class StateQueue;

namespace PLAJA {

    class Configuration;

    class OptionParser;

    class StatsBase;

}

/**********************************************************************************************************************/

class AbstractHeuristic: public PLAJA::StatsInterface {

public:
    enum HeuristicType { NONE, CONSTANT, HAMMING, PA, };

protected:
    PLAJA::StatsBase* statsHeuristic;

public:
    AbstractHeuristic();
    virtual ~AbstractHeuristic() = 0;
    DELETE_CONSTRUCTOR(AbstractHeuristic)

    inline void set_stats_heuristic(PLAJA::StatsBase* stats_heuristic) { statsHeuristic = stats_heuristic; }

    [[nodiscard]] inline PLAJA::StatsBase* _stats_heuristic() const { return statsHeuristic; }

    [[nodiscard]] virtual HeuristicValue_type _evaluate(const AbstractState& state) = 0;
    [[nodiscard]] virtual HeuristicValue_type _evaluate(const AbstractState& state) const;
    [[nodiscard]] HeuristicValue_type evaluate(const AbstractState& state);
    [[nodiscard]] HeuristicValue_type evaluate(const AbstractState& state) const;

    virtual void increment() = 0; // incremental usage

    static std::unique_ptr<StateQueue> make_state_queue(const PLAJA::Configuration& config);
};

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    extern const std::string heuristic_search;
    extern const std::string randomize_tie_breaking;
    extern const std::string max_num_preds_heu;
    extern const std::string max_time_heu;
    extern const std::string max_time_heu_rel;

    namespace ABSTRACT_HEURISTIC {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

namespace PLAJA_OPTION_DEFAULTS {
    extern const std::string heuristic_search;
}

#endif //PLAJA_ABSTRACT_HEURISTIC_H
