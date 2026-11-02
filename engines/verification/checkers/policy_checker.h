//
// This file is part of the PlaJA code base (2019 - 2020).
// Generic PolicyChecker class
//

#ifndef PLAJA_POLICY_CHECKER_H
#define PLAJA_POLICY_CHECKER_H

#include <memory>
#include "include/pa_config_const.h"
#include "core/stats/forward_stats.h"
#include "assertions.h"
#include "core/forward_factories.h"
#include "policies/using_nn.h"
#include "policies/interface/jani_2_interface.h"
#include "core/using_core.h"
#include "smt/base/forward_solution_check.h"
#include "core/forward_states.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/solution_checker_pa.h"

class OpApplicability;

/** Policy checking for PPA. */
class PolicyChecker {

protected:
    // construction data
    const Jani2Interface* jani2Interface;
    const SolutionCheckerPa* solutionChecker;

    // checking modes
    bool lazyNonTerminalPush; // Push non-terminal state condition lazily, i.e., after selection and applicability condition.
    bool lazyInterfacePush;

    // local data
    PLAJA::StatsBase* sharedStatistics;
    std::shared_ptr<OpApplicability> opApp;

    // cache
    ActionLabel_type actionLabel;
    OutputIndex_type outputIndex;
    ActionOpID_type actionOpId;
    UpdateIndex_type updateIndex;
    std::unique_ptr<const AbstractState> sourceState;
    std::unique_ptr<const AbstractState> targetState;

    std::unique_ptr<SolutionCheckerInstance> lastSolution;

    virtual void save_solution_if(bool rlt) = 0;

    virtual bool check_() = 0; // internal auxiliary function to reduce code duplication

    bool push_output_interface();

public:
    PolicyChecker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config);
    virtual ~PolicyChecker();
    DELETE_CONSTRUCTOR(PolicyChecker)

    static void check_linear(const PredicatesExpression& predicates); // Throws if some predicate is not linear.

    inline void set_solution_checker(const SolutionCheckerPa* solution_checker) { solutionChecker = solution_checker; }

    virtual void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app);


    virtual void increment() = 0;

    [[nodiscard]] inline bool lazy_non_terminal() const { return lazyNonTerminalPush; }

    [[nodiscard]] inline bool has_label() const { return actionLabel != ACTION::noneLabel; }

    void push_source(const AbstractState& source);
    void pop_source();
    bool push_label(ActionLabel_type action_label); // No-op for labels that are not learned; false if the query has become unsat.
    void pop_label();
    void push_operator(ActionOpID_type action_op_id);
    void pop_operator();
    void push_update(UpdateIndex_type update_index);
    void pop_update();
    void push_target(const AbstractState& target);
    void pop_target();

    virtual void push() = 0;
    virtual void pop() = 0;

    virtual void set_operator(ActionOpID_type action_op_id) = 0;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    virtual void add_non_terminal_condition() = 0;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif

    /**
     * Add constraints res. predicates of the source state.
     * In particular also location values if in usage.
     * @param source
     */
    virtual void add_source_state(const AbstractState& source) = 0;

    /**
     * @return true if the guard is not trivially true (and thus the query has been modified).
     */
    virtual bool add_guard() = 0;

    /**
     * @return true if guard is not trivially true (and thus the query has *not* become trivially unsat).
     */
    virtual bool add_guard_negation() = 0;

    virtual void add_update() = 0;
    virtual void add_target_state(const AbstractState& target) = 0;

    /**
     * @return false if the query has become unsat.
     */
    virtual bool add_output_interface() = 0; // internally prior to each check

    [[nodiscard]] inline bool has_solution() const { return lastSolution != nullptr; }

    [[nodiscard]] const StateValues& get_last_solution() const;

    [[nodiscard]] std::unique_ptr<StateValues> release_solution();

    [[nodiscard]] const StateValues* get_target_solution() const;

    [[nodiscard]] std::unique_ptr<StateValues> release_target_solution();

    /**
     * Check whether the current input query is sat (true) or unsat (false).
     * @param unknown, value to return in case input query could not be solved.
     * @return
     */
    bool check();
    virtual bool check_relaxed() = 0;
    bool check_selection();
    bool check_applicability();

    void print_statistics();

};

namespace POLICY_CHECKER { extern std::unique_ptr<PolicyChecker> construct_checker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config); }

#endif //PLAJA_POLICY_CHECKER_H
