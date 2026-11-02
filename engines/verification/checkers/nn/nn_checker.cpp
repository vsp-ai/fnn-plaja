//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "engines/verification/checkers/nn/nn_checker.h"
#include "include/compiler_config_const.h"
#include "globals.h"
#include "core/configuration.h"
#include "policies/nn/jani_2_nnet.h"
#include "marabou/model/model_marabou.h"
#include "marabou/solver/smt_solver_marabou.h"
#include "marabou/solver/solution_marabou.h"
#include "marabou/solver/solution_check_wrapper_marabou.h"
#include "core/state_values.h"
#include "engines/verification/pa_states/abstract_state.h"
#include "engines/verification/solution_checker_pa.h"
#include "engines/verification/solution_checker_instance_pa.h"
#include "engines/verification/smt/action_op_marabou_pa.h"
#include "engines/verification/smt/model_marabou_pa.h"

// extern
namespace PLAJA_OPTION::SMT_SOLVER_MARABOU { extern const std::unordered_map<std::string, MILPSolverBoundTighteningType> stringToBoundTightening; }
namespace PLAJA_OPTION {
    extern const std::string per_src_tightening;
    extern const std::string per_src_label_tightening;
}

/**********************************************************************************************************************/

NNChecker::NNChecker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config)
    : 
    PolicyChecker(jani_2_nnet, config), 
    modelMarabou(nullptr)
    , jani2NNet(&jani_2_nnet)
    , solverMarabou(nullptr)
    , perSrcBounds(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::per_src_tightening))
    , perSrcLabelBounds(config.get_option(PLAJA_OPTION::SMT_SOLVER_MARABOU::stringToBoundTightening, PLAJA_OPTION::per_src_label_tightening))
    , actionOp(nullptr)
{
    //

    const bool use_shared = config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU);
    if (not use_shared) { config.set_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU, std::make_shared<ModelMarabouPA>(config)); }
    modelMarabou = PLAJA_UTILS::cast_shared<ModelMarabouPA>(config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU));
    modelMarabou->increment(); // in case predicates have been added after construction
    solverMarabou = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, modelMarabou->make_query(true, true, 0));

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("NN policy checker without stats."))
}

NNChecker::~NNChecker() = default;

void NNChecker::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    PolicyChecker::set_op_applicability_cache(op_app);
    PLAJA_ASSERT(not modelMarabou->get_op_applicability())
    modelMarabou->share_op_applicability(op_app);
}

void NNChecker::increment() { modelMarabou->increment(); }

void NNChecker::push() { solverMarabou->push(); }

void NNChecker::pop() { solverMarabou->pop(); }


void NNChecker::set_operator(ActionOpID_type action_op_id) {
    actionOpId = action_op_id;
    actionOp = &modelMarabou->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(outputIndex == jani2NNet->get_output_index(actionOp->_action_label()))
}

void NNChecker::add_source_state(const AbstractState& source) {
    sourceState = source.to_ptr();
    modelMarabou->add_to(source, *solverMarabou);
    if (perSrcBounds != MILPSolverBoundTighteningType::NONE) { modelMarabou->lp_tightening(*solverMarabou, perSrcBounds); }
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void NNChecker::add_non_terminal_condition() { modelMarabou->exclude_terminal(*solverMarabou, 0); }

#endif

bool NNChecker::add_guard() {
    PLAJA_ASSERT(actionOp)
    return actionOp->add_guard(*solverMarabou, false, 0);
}

bool NNChecker::add_guard_negation() {
    PLAJA_ASSERT(actionOp)
    return actionOp->add_guard_negation(*solverMarabou, false, 0);
}

void NNChecker::add_update() {
    PLAJA_ASSERT(actionOp)
    PLAJA_ASSERT(updateIndex != ACTION::noneUpdate)
    actionOp->add_update(*solverMarabou, updateIndex, false, false);
}

void NNChecker::add_target_state(const AbstractState& target) {
    targetState = target.to_ptr();
    PLAJA_ASSERT(targetState->has_entailment_information() == target.has_entailment_information())

    PLAJA_ASSERT(actionOp)
    actionOp->add_target_predicate_state(target, *solverMarabou, updateIndex);
}


bool NNChecker::add_output_interface() {
    modelMarabou->add_output_interface(*solverMarabou, actionLabel);
    if (perSrcLabelBounds != MILPSolverBoundTighteningType::NONE) { return modelMarabou->lp_tightening_output(*solverMarabou, perSrcLabelBounds); }
    return true;
}

/***check version*******************************************************************************************************/

void NNChecker::save_solution_if(bool rlt) {

    /* Unset solution if unsat. */
    if (not rlt) {
        lastSolution = nullptr;
        return;
    }

    const bool has_instance = solverMarabou->get_solution_checker();

    if (not has_instance) { // Always extract solution via checker instance.
        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, sourceState.get(), actionLabel, actionOpId, updateIndex, targetState.get()));
    }

    lastSolution = solverMarabou->extract_solution_via_checker();
    PLAJA_ASSERT(not lastSolution or lastSolution->get_solution(0))
    PLAJA_ASSERT(not lastSolution or not targetState or lastSolution->get_solution(1))

    if (not has_instance) { solverMarabou->unset_solution_checker(); }

}

bool NNChecker::check_() {

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) {
        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, sourceState.get(), actionLabel, actionOpId, updateIndex, targetState.get()));
    }

    const bool rlt = solverMarabou->check();

    if constexpr (PLAJA_GLOBAL::reuseSolutions) {

        save_solution_if(rlt);

        // We expect floating errors in the NN but not the transition structure.
        STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
        PLAJA_ASSERT(not rlt or not solverMarabou->is_exact() or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), sourceState.get(), actionLabel, actionOpId, updateIndex, targetState.get(), lastSolution->get_solution(1)) or not solutionChecker->check_policy(*lastSolution->get_solution(0), actionLabel, nullptr))
        STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)
    }

    if constexpr (PLAJA_GLOBAL::roundRelaxedSolutions) { solverMarabou->unset_solution_checker(); }

    solverMarabou->reset();

    return rlt;

}

bool NNChecker::check_relaxed() {

    const bool rlt = solverMarabou->check_relaxed();

    if constexpr (PLAJA_GLOBAL::reuseSolutions) { save_solution_if(rlt); }

    solverMarabou->reset();

    return rlt;
}


/***********************************************************************************************************************/


#pragma GCC diagnostic pop
