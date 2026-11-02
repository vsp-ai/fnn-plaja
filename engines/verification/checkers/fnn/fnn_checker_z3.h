//
// This file is part of the PlaJA code base.
//

#ifndef PLAJA_FNN_CHECKER_Z3_H
#define PLAJA_FNN_CHECKER_Z3_H

#include <memory>
#include "engines/verification/checkers/policy_checker.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/smt/forward_smt_pa.h"

class Jani2FNN;
class ActionOpPA;
class OpApplicability;

namespace Z3_IN_PLAJA { class SMTSolver; }

/** FNN policy checking for PPA (Z3). */
class FnnCheckerZ3 : public PolicyChecker {

protected:
    std::shared_ptr<ModelZ3PA> modelZ3;
    const Jani2FNN* jani2FNN;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;

    const ActionOpPA* actionOp;

    void save_solution_if(bool rlt) override;
    bool check_() override; // internal auxiliary function to reduce code duplication

public:
    FnnCheckerZ3(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);
    ~FnnCheckerZ3() override;
    DELETE_CONSTRUCTOR(FnnCheckerZ3)


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

    void add_source_state(const AbstractState& source) override;

    bool add_guard() override;

    bool add_guard_negation() override;

    void add_update() override;

    void add_target_state(const AbstractState& target) override;

    bool add_output_interface() override; // internally prior to each check

    bool check_relaxed() override;

};

#endif // PLAJA_FNN_CHECKER_Z3_H
