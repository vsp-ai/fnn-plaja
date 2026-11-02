//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_PA_BASE_H
#define PLAJA_SEARCH_PA_BASE_H

#include <list>
#include "core/stats/forward_stats.h"
#include "assertions.h"
#include "engines/verification/forward_factories_pa.h"
#include "core/search_engine.h"
#include "model/using_parser.h"
#include "core/using_core.h"
#include "engines/verification/heuristic/forward_heuristic_pa.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/search_space/forward_search_space.h"
#include "engines/verification/smt/forward_smt_pa.h"
#include "engines/verification/using_predicate_abstraction.h"

class ExpansionPABase;

class PolicyChecker; 

class SearchPABase: public SearchEngine {

protected:
    PLAJA::StatsBase* statsHeuristic; // In case engine is used within heuristic construct.
    std::shared_ptr<ModelZ3PA> modelZ3;
    std::unique_ptr<SearchSpacePABase> searchSpace;
    std::unique_ptr<ExpansionPABase> expansionPa;

    // Frontier
#ifdef ENABLE_HEURISTIC_SEARCH_PA
    std::unique_ptr<StateQueue> frontier;
#else
    std::unique_ptr<FIFOStateQueue> frontier;
#endif

    // Start states:
    std::unique_ptr<std::list<StateID_type>> cachedStartStates;
    bool add_start_state();

    // Goal:
    mutable int pathLength;

    // Search config.
    bool goalPathSearch;
    bool optimalSearch;

    void construct_heuristic(const SearchEngineConfigPA& config);

public:
    explicit SearchPABase(const SearchEngineConfigPA& config, bool goal_path_search, bool optimal_search, bool witness_interval = false);
    ~SearchPABase() override = 0;
    DELETE_CONSTRUCTOR(SearchPABase)

    virtual void increment(); // For incremental usage.
    SearchStatus initialize() override;
    SearchStatus step() override;

    [[nodiscard]] bool has_goal_path() const;
    [[nodiscard]] AbstractPath extract_goal_path() const;
    [[nodiscard]] virtual HeuristicValue_type get_goal_distance(const AbstractState& pa_state) = 0;

    FCT_IF_DEBUG(PolicyChecker* get_policy();)

    // stats:
    void update_statistics() const override;
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

};

#endif //PLAJA_SEARCH_PA_BASE_H
