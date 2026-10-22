//
// This file is part of the PlaJA code base.
//

#ifndef PLAJA_FNN_SAT_CHECKER_Z3_H
#define PLAJA_FNN_SAT_CHECKER_Z3_H

#include <memory>
#include "../sat_checker.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"

class Jani2FNN;
class ActionOpPA;
class OpApplicability;

namespace Z3_IN_PLAJA { class SMTSolver; }

/** FNN-Sat checking for PPA (Z3). */
class FnnSatCheckerZ3 : public SatChecker {

protected:
    std::shared_ptr<ModelZ3PA> modelZ3;
    const Jani2FNN* jani2FNN;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;

    const ActionOpPA* set_action_op;

    void save_solution_if(bool rlt) override;
    bool check_() override; // internal auxiliary function to reduce code duplication

public:
    FnnSatCheckerZ3(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);
    ~FnnSatCheckerZ3() override;
    DELETE_CONSTRUCTOR(FnnSatCheckerZ3)


    void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) override;


    void increment() override;

    void push() override;
    void pop() override;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    void add_non_terminal_condition() override;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif

    void set_operator(ActionOpID_type action_op_id) override;

    /**
     * Add constraints res. predicates of the source state.
     * In particular also location values if in usage.
     * @param source
     */
    void add_source_state(const AbstractState& source) override;

    /**
     * @return true if the guard is not trivially true (and thus the query has been modified).
     */
    bool add_guard() override;

    /**
     * @return true if guard is not trivially true (and thus the query has *not* become trivially unsat).
     */
    bool add_guard_negation() override;

    void add_update() override;

    void add_target_state(const AbstractState& target) override;

    /**
     * @return false if the query has become unsat.
     */
    bool add_output_interface() override; // internally prior to each check

    /**
     * Check whether the current input query is sat (true) or unsat (false).
     * @param unknown, value to return in case input query could not be solved.
     * @return
     */
    bool check_relaxed() override;

    bool has_solution_box() override { return false; }

};

#endif // PLAJA_FNN_SAT_CHECKER_Z3_H
