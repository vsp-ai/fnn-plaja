//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_CHECKER_H
#define PLAJA_NN_CHECKER_H

#include <list>
#include <vector>
#include <memory>
#include "include/marabou_include/milp_solver_bound_tightening_type.h"
#include "include/ct_config_const.h"
#include "core/stats/forward_stats.h"
#include "assertions.h"
#include "core/forward_factories.h"
#include "policies/nn/forward_jani_2_nnet.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "smt/base/forward_solution_check.h"
#include "marabou/forward_marabou_in_plaja.h"
#include "core/forward_states.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/smt/forward_marabou_pa.h"
#include "engines/verification/checkers/policy_checker.h"

/** NN policy checking for PPA. */
class NNChecker : public PolicyChecker {

protected:
    // construction data
    std::shared_ptr<ModelMarabouPA> modelMarabou;
    const Jani2NNet* jani2NNet;

    // checking modes
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solverMarabou;
    MILPSolverBoundTighteningType perSrcBounds;
    MILPSolverBoundTighteningType perSrcLabelBounds;

    const ActionOpMarabouPA* actionOp;

    void save_solution_if(bool rlt) override;

    virtual bool check_() override; // internal auxiliary function to reduce code duplication

public:
    NNChecker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);
    virtual ~NNChecker() override;
    DELETE_CONSTRUCTOR(NNChecker)

    void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) override;

    [[nodiscard]] const Jani2NNet& _jani_2_nnet() const { return *jani2NNet; }

    void increment() override;

    void push() override;
    void pop() override;

    void set_operator(ActionOpID_type action_op_id) override;

    /**
     * Add constraints res. predicates of the source state.
     * In particular also location values if in usage.
     * @param source
     */
    void add_source_state(const AbstractState& source) override;
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    void add_non_terminal_condition() override;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif
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


    void print_statistics();

};

#endif //PLAJA_NN_CHECKER_H
