//
// This file is part of the PlaJA code base.
//


#include "fnn_sat_checker_z3.h"

#include "../../../globals.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/plaja_options.h"
#include "../../factories/configuration.h"
#include "../../information/jani2fnn/jani_2_fnn.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../states/state_values.h"
#include "../pa_states/abstract_state.h"
#include "../solution_checker_pa.h"
#include "../solution_checker_instance_pa.h"
#include "../smt/model_z3_pa.h"
#include "../successor_generation/action_op/action_op_pa.h"
#include "../successor_generation/update/update_pa.h"

FnnSatCheckerZ3::FnnSatCheckerZ3(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config)
    : SatChecker(jani_2_fnn, config)
    , modelZ3(nullptr)
    , jani2FNN(&jani_2_fnn)
    , solver(nullptr)
    , set_action_op(nullptr) {

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = std::static_pointer_cast<ModelZ3PA>(config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3));
    PLAJA_ASSERT(modelZ3)
    std::cout << "[FnnSatChecker] Constructing shared Z3 query ..." << std::endl;
    modelZ3->increment(); // in case predicates have been added after construction

    solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->share_context(), sharedStatistics);

    // register bounds for inputs and predicates
    for (auto it = jani2FNN->init_input_feature_iterator(true); !it.end(); ++it) {
        if (not it.get_input_feature()->is_location()) { modelZ3->register_src_bound(*solver, it.get_input_feature()->get_variable_id()); }
    }

    if (modelZ3->_predicates()) {
        for (auto pred_it = modelZ3->_predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) {
            modelZ3->register_src_bounds(*solver, *pred_it);
        }
    }

    // add FNN structure
    if (modelZ3->has_fnn()) { modelZ3->add_fnn(*solver, 0); }

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("FNN-sat-checker without stats."))
}

FnnSatCheckerZ3::~FnnSatCheckerZ3() = default;



void FnnSatCheckerZ3::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    if (not modelZ3->get_op_applicability()) { modelZ3->share_op_applicability(op_app); }
}

void FnnSatCheckerZ3::increment() {
    modelZ3->increment();
    if (modelZ3->_predicates()) {
        for (auto pred_it = modelZ3->_predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) {
            modelZ3->register_src_bounds(*solver, *pred_it);
        }
    }
}

void FnnSatCheckerZ3::push() { solver->push(); }

void FnnSatCheckerZ3::pop() { solver->pop(); }

void FnnSatCheckerZ3::set_operator(ActionOpID_type action_op_id) {
    set_action_op_id = action_op_id;
    set_action_op = &modelZ3->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(set_output_index == jani2FNN->get_output_index(set_action_op->_action_label()))
}

void FnnSatCheckerZ3::add_source_state(const AbstractState& source) {
    set_source_state = source.to_ptr();
    modelZ3->add_to(source, *solver, 0);
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void FnnSatCheckerZ3::add_non_terminal_condition() { modelZ3->exclude_terminal(*solver, 0); }

#endif

bool FnnSatCheckerZ3::add_guard() {
    PLAJA_ASSERT(set_action_op)
    set_action_op->add_to_solver(*solver);
    return not set_action_op->_guards().is_true();
}

bool FnnSatCheckerZ3::add_guard_negation() {
    PLAJA_ASSERT(set_action_op)
    const auto& guard = set_action_op->_guards();
    if (guard.is_true()) { return false; }
    set_action_op->add_src_bounds(*solver);
    solver->add(!guard);
    return true;
}

void FnnSatCheckerZ3::add_update() {
    PLAJA_ASSERT(set_action_op)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    set_action_op->get_update_pa(set_update_index).add_to_solver(*solver);
}

void FnnSatCheckerZ3::add_target_state(const AbstractState& target) {
    set_target_state = target.to_ptr();
    PLAJA_ASSERT(set_target_state->has_entailment_information() == target.has_entailment_information())

    PLAJA_ASSERT(set_action_op)
    set_action_op->get_update_pa(set_update_index).add_predicates_to_solver(*solver, target);
}

bool FnnSatCheckerZ3::add_output_interface() {
    PLAJA_ASSERT(set_action_label != ACTION::noneLabel)
    modelZ3->add_output_interface(*solver, set_action_label, 0);
    return true;
}

void FnnSatCheckerZ3::save_solution_if(bool rlt) {
    if (not rlt) {
        lastSolution = nullptr;
        return;
    }

    lastSolution = solver->extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get()));
    PLAJA_ASSERT(not lastSolution or lastSolution->get_solution(0))
    PLAJA_ASSERT(not lastSolution or not set_target_state or lastSolution->get_solution(1))
}

bool FnnSatCheckerZ3::check_() {
    const bool rlt = solver->check();
    if constexpr (PLAJA_GLOBAL::reuseSolutions) {
        save_solution_if(rlt);

        STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
        PLAJA_ASSERT(not rlt or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, set_target_state.get(), set_target_state ? lastSolution->get_solution(1) : nullptr) or not solutionChecker->check_policy(*lastSolution->get_solution(0), set_action_label, nullptr))
        STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
    }

    solver->reset();

    return rlt;
}

bool FnnSatCheckerZ3::check_relaxed() {
    const bool rlt = solver->check();
    if constexpr (PLAJA_GLOBAL::reuseSolutions) { save_solution_if(rlt); }
    solver->reset();
    return rlt;
}
