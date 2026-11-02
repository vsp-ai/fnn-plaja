//
// This file is part of the PlaJA code base (2019 - 2020).
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "engines/verification/checkers/policy_checker.h"
#include "include/compiler_config_const.h"
#include "globals.h"
#include "core/configuration.h"
#include "policies/interface/jani_2_interface.h"
#include "core/state_values.h"
#include "engines/verification/pa_states/abstract_state.h"
#include "engines/verification/solution_checker_pa.h"
#include "engines/verification/solution_checker_instance_pa.h"
#include "core/exception/constructor_exception.h"
#include "model/ast/expression/non_standard/predicates_expression.h"
#include "model/visitor/extern/to_linear_expression.h"
#include "model/visitor/linear_constraints_checker.h"
#include "smt/successor_generation/op_applicability.h"
#include "engines/verification/pa_options.h"
#include "policies/nn/jani_2_nnet.h"
#include "policies/fnn/jani_2_fnn.h"
#include "engines/verification/checkers/nn/nn_checker.h"
#include "engines/verification/checkers/nn/nn_checker_factory.h"
#include "engines/verification/checkers/fnn/fnn_checker_factory.h"

/**********************************************************************************************************************/

PolicyChecker::PolicyChecker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config)
    :
    jani2Interface(&jani_2_interface)
    , solutionChecker(nullptr)
    , lazyNonTerminalPush(PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::doSelectionTest and config.get_bool_option(PLAJA_OPTION::lazyNonTerminalPush))
    , lazyInterfacePush(PLAJA_GLOBAL::enableApplicabilityFiltering and not PLAJA_GLOBAL::doSelectionTest and config.get_bool_option(PLAJA_OPTION::lazyPolicyInterfacePush))
    , sharedStatistics(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , opApp(nullptr)
    , actionLabel(ACTION::noneLabel)
    , outputIndex(NEURON_INDEX::none)
    , actionOpId(ACTION::noneOp)
    , updateIndex(ACTION::noneUpdate)
    , sourceState(nullptr)
    , targetState(nullptr)
    , lastSolution(nullptr)
{
    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Policy checker without stats."))
}

PolicyChecker::~PolicyChecker() = default;

void PolicyChecker::check_linear(const PredicatesExpression& predicates) {
    for (auto pred_it = predicates.predicatesIterator(); !pred_it.end(); ++pred_it) {
        if (not TO_LINEAR_EXP::is_linear_constraint(*pred_it) and not LinearConstraintsChecker::is_linear(*pred_it, LINEAR_CONSTRAINTS_CHECKER::Specification::as_predicate(true))) { throw ConstructorException(pred_it->to_string(), PLAJA_EXCEPTION::nonLinear); }
    }
}

void PolicyChecker::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) { opApp = op_app; }

void PolicyChecker::push_source(const AbstractState& source) {
    push();
    add_source_state(source);
}

void PolicyChecker::pop_source() {
    sourceState = nullptr;
    pop();
}

bool PolicyChecker::push_label(ActionLabel_type action_label) {
    if (not jani2Interface->is_learned(action_label)) { return true; }
    push();
    actionLabel = action_label;
    outputIndex = jani2Interface->get_output_index(action_label);
    return lazyInterfacePush or push_output_interface();
}

void PolicyChecker::pop_label() {
    if (not has_label()) { return; }
    actionLabel = ACTION::noneLabel;
    outputIndex = NEURON_INDEX::none;
    pop();
}

void PolicyChecker::push_operator(ActionOpID_type action_op_id) {
    if (not has_label()) { return; }
    push();
    set_operator(action_op_id);
    add_guard();
    if (lazyNonTerminalPush) { add_non_terminal_condition(); }
    if (lazyInterfacePush) { push_output_interface(); }
}

void PolicyChecker::pop_operator() {
    if (not has_label()) { return; }
    actionOpId = ACTION::noneOp;
    pop();
}

void PolicyChecker::push_update(UpdateIndex_type update_index) {
    if (not has_label()) { return; }
    push();
    updateIndex = update_index;
    add_update();
}

void PolicyChecker::pop_update() {
    if (not has_label()) { return; }
    updateIndex = ACTION::noneUpdate;
    pop();
}

void PolicyChecker::push_target(const AbstractState& target) {
    push();
    add_target_state(target);
}

void PolicyChecker::pop_target() {
    targetState = nullptr;
    pop();
}

bool PolicyChecker::push_output_interface() {
    // Skip the label's guard disjunction: each check also has an operator guard of the label (not so with selection tests).
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache and not PLAJA_GLOBAL::doSelectionTest) { if (opApp) { opApp->set_self_applicability(true); } }
    const bool rlt = add_output_interface();
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache and not PLAJA_GLOBAL::doSelectionTest) { if (opApp) { opApp->unset_self_applicability(); } }
    return rlt;
}

const StateValues& PolicyChecker::get_last_solution() const {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return *lastSolution->get_solution(0);
}

std::unique_ptr<StateValues> PolicyChecker::release_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return lastSolution->retrieve_solution(0);
}

const StateValues* PolicyChecker::get_target_solution() const {
    PLAJA_ASSERT(has_solution())
    if (lastSolution->get_solution_size() > 1) {
        PLAJA_ASSERT(lastSolution->get_solution_size() == 2)
        return lastSolution->get_solution(1);
    } else {
        return nullptr;
    }
}

std::unique_ptr<StateValues> PolicyChecker::release_target_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(1))
    return lastSolution->retrieve_solution(1);
}

/***check version*******************************************************************************************************/


bool PolicyChecker::check() {

    // unset exact solution if set:
    if constexpr (PLAJA_GLOBAL::reuseSolutions) { lastSolution = nullptr; }

    const bool rlt = check_();

    return rlt;
}

bool PolicyChecker::check_selection() { return PLAJA_GLOBAL::doRelaxedSelectionTestOnly ? check_relaxed() : check(); }

bool PolicyChecker::check_applicability() { return PLAJA_GLOBAL::doRelaxedNNApplicabilityTestOnly ? check_relaxed() : check(); }

/***********************************************************************************************************************/

void PolicyChecker::print_statistics() {
}

/**********************************************************************************************************************/

std::unique_ptr<PolicyChecker> POLICY_CHECKER::construct_checker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config) {
    if (const auto* jani_2_nnet = dynamic_cast<const Jani2NNet*>(&jani_2_interface)) { return NN_CHECKER::construct_checker(*jani_2_nnet, config); }
    const auto* jani_2_fnn = dynamic_cast<const Jani2FNN*>(&jani_2_interface);
    PLAJA_ASSERT(jani_2_fnn)
    return FNN_CHECKER::construct_checker(*jani_2_fnn, config);
}

#pragma GCC diagnostic pop
