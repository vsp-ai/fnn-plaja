//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPANSION_PA_BASE_H
#define PLAJA_EXPANSION_PA_BASE_H

#include <unordered_set>
#include <list>
#include "core/option_parser/forward_option_parser.h"
#include "core/stats/forward_stats.h"
#include "assertions.h"
#include "engines/verification/forward_factories_pa.h"
#include "core/search_engine.h"
#include "model/forward_information.h"
#include "smt/base/forward_solution_check.h"
#include "smt/forward_smt_z3.h"
#include "successor_generation/forward_successor_generation.h"
#include "engines/verification/match_tree/forward_match_tree_pa.h"
#include "engines/verification/optimization/forward_optimization_pa.h"
#include "engines/verification/smt/forward_smt_pa.h"
#include "engines/verification/successor_generation/forward_succ_gen_pa.h"
#include "engines/verification/smt/state_condition_checker_pa.h"
#include "engines/verification/pa_transition_structure.h"
#include "engines/verification/using_predicate_abstraction.h"
#include "engines/verification/search_space/forward_search_space.h"
#include "model/property_information.h"
#include "engines/verification/solution_checker_pa.h"

class PolicyChecker;

class SearchPABase;

class ExpansionPABase {

protected:
    PLAJA::StatsBase* sharedStats;
    SearchSpacePABase* searchSpace;
    std::unique_ptr<const SolutionCheckerPa> solutionChecker;

    const ModelZ3PA* modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solverOwn;
    Z3_IN_PLAJA::SMTSolver* solver;
    std::unique_ptr<StateConditionCheckerPa> reachChecker; // To check the reach condition.
    std::unique_ptr<StateConditionCheckerPa> terminalChecker; // To check whether an abstract state fulfills terminal state condition.

    std::shared_ptr<PredicateRelations> predicateRelations; // Prune branches based on binary entailment relations of predicates
    std::unique_ptr<SuccessorGeneratorPA> successorGenerator;
    std::shared_ptr<OpApplicability> opApp; // Optimization for applicability filtering.
    std::unique_ptr<PolicyChecker> policy;


/** solution cache ****************************************************************************************************/

    /* Search config. */
    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> paStateReachability;
    bool goalPathSearch;
    bool optimalSearch;
    bool incOpAppTest;

    bool mayBeTerminal;
    bool intervalRefinement = false;
    FIELD_IF_DEBUG(bool expectExactTerminalCheck;)

    [[nodiscard]] bool do_global_reachability_search() const;
    void register_src_bounds(); // of start, reach and predicates

    /* Expansion cache. */

    std::unique_ptr<struct ExpansionCachePa> cache;

    [[nodiscard]] PATransitionStructure& access_transition_struct();
    [[nodiscard]] std::list <StateID_type>& access_newly_reached();
    [[nodiscard]] bool cache_current_successors() const;
    [[nodiscard]] std::unordered_set <StateID_type>& access_current_successors();
    [[nodiscard]] bool is_current_successor(StateID_type id) const;
    [[nodiscard]] bool& access_found_transition();
    [[nodiscard]] bool& access_hit_return();
    [[nodiscard]] SearchDepth_type get_current_search_depth() const;
    [[nodiscard]] SearchDepth_type get_upper_bound_goal_distance() const;
    void min_upper_bound_goal_distance(SearchDepth_type value);

    /** solution cache ************************************************************************************************/

    void compute_transition_for_label(const StateValues& solution);
    void compute_transition_for_operator(const StateValues& solution, bool under_policy);
    void compute_transition_for_update(const StateValues& solution, bool under_policy);
    void handle_transition_existence(const StateValues& solution, const StateValues& successor, bool under_policy);

    /** incremental ***************************************************************************************************/

    void push_source(std::unique_ptr<AbstractState>&& source);
    void pop_source();

    [[nodiscard]] bool push_action(ActionLabel_type action_label);
    void pop_action();

    void set_action_op(const ActionOpPA& action_op);
    void unset_action_op();

    void push_update(UpdateIndex_type update_index, const UpdatePA& update_pa);
    void pop_update();

    void policy_push_action_op();
    void policy_pop_action_op();

    /** smt checks ****************************************************************************************************/

    bool applicability_test();
    bool transition_test();

    bool policy_selection_test();
    bool policy_applicability_test();
    bool policy_transition_test();

    /******************************************************************************************************************/

    [[nodiscard]] bool step_per_op();

    void enumerate_pa_states(PredicateState& pred_state);
    void enumerate_pa_states(PredicateState& pred_state, PredicateTraverser& predicate_traverser);

    void compute_transition(PredicateState& pred_state);
    [[nodiscard]] bool pre_check_node(const SEARCH_SPACE_PA::SearchNode& target_node) const;
    void handle_transition_existence(SEARCH_SPACE_PA::SearchNode& target_node);

public:
    explicit ExpansionPABase(const SearchEngineConfigPA& config, bool goal_path_search, bool optimal_search, SearchSpacePABase& search_space, const ModelZ3PA& model_z3, bool needs_interval = false);
    virtual ~ExpansionPABase();
    DELETE_CONSTRUCTOR(ExpansionPABase)

    // auxiliaries
    inline bool check_reach(const AbstractState& state) { return reachChecker->check(state); }

    [[nodiscard]] const SuccessorGeneratorC& get_concrete_suc_gen();

    void print_stats() const;

    FCT_IF_DEBUG(PolicyChecker* get_policy() { return policy.get(); })

    [[nodiscard]] const PolicyChecker* get_policy() const { return policy.get(); }

    void increment();
    void reset();
    [[nodiscard]] std::list<StateID_type> step(StateID_type src_id);
};

namespace PLAJA_OPTION { extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_pa_state_reachability_enum(); }

#endif //PLAJA_EXPANSION_PA_BASE_H
