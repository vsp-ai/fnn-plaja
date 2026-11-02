//
// This file is part of the PlaJA code base.
//


#include "engines/verification/checkers/fnn/fnn_checker_z3.h"

#include "globals.h"
#include "core/option_parser/option_parser.h"
#include "core/option_parser/plaja_options.h"
#include "core/configuration.h"
#include "policies/fnn/jani_2_fnn.h"
#include "smt/solver/smt_solver_z3.h"
#include "model/ast/expression/non_standard/predicates_expression.h"
#include "core/state_values.h"
#include "engines/verification/pa_states/abstract_state.h"
#include "engines/verification/solution_checker_pa.h"
#include "engines/verification/solution_checker_instance_pa.h"
#include "engines/verification/smt/model_z3_pa.h"
#include "engines/verification/successor_generation/action_op/action_op_pa.h"
#include "engines/verification/successor_generation/update/update_pa.h"

FnnCheckerZ3::FnnCheckerZ3(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config)
    : PolicyChecker(jani_2_fnn, config)
    , modelZ3(nullptr)
    , jani2FNN(&jani_2_fnn)
    , solver(nullptr)
    , actionOp(nullptr) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3));
    PLAJA_ASSERT(modelZ3)
    modelZ3->increment(); // in case predicates have been added after construction

    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->share_context(), sharedStatistics);

    modelZ3->register_src_input_bounds(*solver, *jani2FNN);

    // add FNN structure
    PLAJA_ASSERT(modelZ3->has_fnn())
    modelZ3->add_fnn(*solver, 0);

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("FNN policy checker without stats."))
}

FnnCheckerZ3::~FnnCheckerZ3() = default;

void FnnCheckerZ3::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    PolicyChecker::set_op_applicability_cache(op_app);
    if (not modelZ3->get_op_applicability()) { modelZ3->share_op_applicability(op_app); }
}

void FnnCheckerZ3::increment() {
    modelZ3->increment();
    if (modelZ3->_predicates()) {
        for (auto pred_it = modelZ3->_predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) {
            modelZ3->register_src_bounds(*solver, *pred_it);
        }
    }
}

void FnnCheckerZ3::push() { solver->push(); }

void FnnCheckerZ3::pop() { solver->pop(); }

void FnnCheckerZ3::set_operator(ActionOpID_type action_op_id) {
    actionOpId = action_op_id;
    actionOp = &modelZ3->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(outputIndex == jani2FNN->get_output_index(actionOp->_action_label()))
}

void FnnCheckerZ3::add_source_state(const AbstractState& source) {
    sourceState = source.to_ptr();
    modelZ3->add_to(source, *solver, 0);
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void FnnCheckerZ3::add_non_terminal_condition() { modelZ3->exclude_terminal(*solver, 0); }

#endif

bool FnnCheckerZ3::add_guard() {
    PLAJA_ASSERT(actionOp)
    actionOp->add_to_solver(*solver);
    return not actionOp->_guards().is_true();
}

bool FnnCheckerZ3::add_guard_negation() {
    PLAJA_ASSERT(actionOp)
    const auto& guard = actionOp->_guards();
    if (guard.is_true()) { return false; }
    actionOp->add_src_bounds(*solver);
    solver->add(!guard);
    return true;
}

void FnnCheckerZ3::add_update() {
    PLAJA_ASSERT(actionOp)
    PLAJA_ASSERT(updateIndex != ACTION::noneUpdate)
    actionOp->get_update_pa(updateIndex).add_to_solver(*solver);
}

void FnnCheckerZ3::add_target_state(const AbstractState& target) {
    targetState = target.to_ptr();
    PLAJA_ASSERT(targetState->has_entailment_information() == target.has_entailment_information())

    PLAJA_ASSERT(actionOp)
    actionOp->get_update_pa(updateIndex).add_predicates_to_solver(*solver, target);
}

bool FnnCheckerZ3::add_output_interface() {
    PLAJA_ASSERT(actionLabel != ACTION::noneLabel)
    modelZ3->add_output_interface(*solver, actionLabel, 0);
    return true;
}

void FnnCheckerZ3::save_solution_if(bool rlt) {
    if (not rlt) {
        lastSolution = nullptr;
        return;
    }

    lastSolution = solver->extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, sourceState.get(), actionLabel, actionOpId, updateIndex, targetState.get()));
    PLAJA_ASSERT(not lastSolution or lastSolution->get_solution(0))
    PLAJA_ASSERT(not lastSolution or not targetState or lastSolution->get_solution(1))
}

bool FnnCheckerZ3::check_() {
    const bool rlt = solver->check();
    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        save_solution_if(rlt);

        STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
        PLAJA_ASSERT(not rlt or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), sourceState.get(), actionLabel, actionOpId, updateIndex, targetState.get(), targetState ? lastSolution->get_solution(1) : nullptr) or not solutionChecker->check_policy(*lastSolution->get_solution(0), actionLabel, nullptr))
        STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
    }

    solver->reset();

    return rlt;
}

bool FnnCheckerZ3::check_relaxed() {
    const bool rlt = solver->check();
    if constexpr (PLAJA_GLOBAL::reuseSolutions) { save_solution_if(rlt); }
    solver->reset();
    return rlt;
}
