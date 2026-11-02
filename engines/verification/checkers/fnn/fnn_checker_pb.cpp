#include "engines/verification/checkers/fnn/fnn_checker_pb.h"
#include "engines/verification/checkers/fnn/fnn_checker_factory.h"
#include "globals.h"
#include "engines/verification/smt/pb_engine.h"
#include "engines/verification/smt/pb_query_builder.h"
#include "core/configuration.h"
#include "policies/fnn/jani_2_fnn.h"
#include "engines/verification/pa_states/abstract_state.h"
#include "smt/base/solution_checker_instance.h"
#include "core/state_values.h"
#include "engines/verification/solution_checker_pa.h"
#include "engines/verification/solution_checker_instance_pa.h"
#include "engines/verification/smt/model_z3_pa.h"
#include "engines/verification/successor_generation/action_op/action_op_pa.h"

FnnCheckerPB::FnnCheckerPB(const Jani2FNN& jani_2_fnn,
                           const PLAJA::Configuration& config)
    : PolicyChecker(jani_2_fnn, config)
    , modelZ3(nullptr)
    , jani2FNN(&jani_2_fnn)
    , actionOp(nullptr)
    , engine(std::make_unique<FNN_PB::Engine>(config.get_bool_option(PLAJA_OPTION::use_incremental_pb),
                                              config.get_option(PLAJA_OPTION::incremental_pb_granularity) == "operator" ? FNN_PB::Granularity::Operator : FNN_PB::Granularity::Label,
                                              sharedStatistics)) {
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = std::static_pointer_cast<ModelZ3PA>(
        config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3));
    PLAJA_ASSERT(modelZ3)
    modelZ3->increment();
}

FnnCheckerPB::~FnnCheckerPB() = default;

void FnnCheckerPB::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    PolicyChecker::set_op_applicability_cache(op_app);
    if (not modelZ3->get_op_applicability()) { modelZ3->share_op_applicability(op_app); }
}

void FnnCheckerPB::increment() {
    modelZ3->increment();
    engine->increment();
}

void FnnCheckerPB::push() { flagsStack.push_back(flags); }

void FnnCheckerPB::pop() {
    PLAJA_ASSERT(not flagsStack.empty())
    flags = flagsStack.back();
    flagsStack.pop_back();
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
void FnnCheckerPB::add_non_terminal_condition() { flags.requireNonTerminal = true; }
#endif

void FnnCheckerPB::set_operator(ActionOpID_type action_op_id) {
    actionOpId = action_op_id;
    actionOp = &modelZ3->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(outputIndex == jani2FNN->get_output_index(actionOp->_action_label()))
}

void FnnCheckerPB::add_source_state(const AbstractState& source) {
    sourceState = source.to_ptr();
}

bool FnnCheckerPB::add_guard() {
    PLAJA_ASSERT(actionOp)
    flags.useGuard = true;
    flags.useGuardNegation = false;
    return not actionOp->_guards().is_true();
}

bool FnnCheckerPB::add_guard_negation() {
    PLAJA_ASSERT(actionOp)
    flags.useGuard = false;
    flags.useGuardNegation = true;
    return not actionOp->_guards().is_true();
}

void FnnCheckerPB::add_update() {
    PLAJA_ASSERT(actionOp)
    PLAJA_ASSERT(updateIndex != ACTION::noneUpdate)
    flags.useUpdate = true;
}

void FnnCheckerPB::add_target_state(const AbstractState& target) {
    targetState = target.to_ptr();
    flags.useTargetState = true;
}

bool FnnCheckerPB::add_output_interface() {
    PLAJA_ASSERT(actionLabel != ACTION::noneLabel)
    flags.useOutputInterface = true;
    flags.useAppFilter = jani2FNN->_do_applicability_filtering();
    return true;
}

void FnnCheckerPB::save_solution_if(bool result) {
    lastSolution = nullptr;
    const auto& source_values = engine->witness_source();
    const auto& target_values = engine->witness_target();
    const auto* target = flags.useTargetState ? targetState.get() : nullptr;
    if (not result or not solutionChecker or source_values.empty()
        or (target and target_values.empty())) { return; }

    auto instance = std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, sourceState.get(), actionLabel,
                                                                actionOpId, updateIndex, target);
    const auto to_state = [&instance](const std::vector<PLAJA::integer>& values) {
        auto state = std::make_unique<StateValues>(instance->get_solution_constructor());
        for (VariableIndex_type index = 0; index < values.size(); ++index) { state->assign_int<false>(index, values[index]); }
        return state;
    };
    instance->set_solution(to_state(source_values), 0);
    if (target) { instance->set_solution(to_state(target_values), 1); }
    lastSolution = std::move(instance);
}

bool FnnCheckerPB::check_() {
    const FNN_PB::QueryContext query {
        jani2FNN,
        modelZ3.get(),
        sourceState.get(),
        targetState.get(),
        actionOp,
        actionLabel,
        outputIndex,
        actionOpId,
        updateIndex,
        flags.requireNonTerminal,
        flags.useOutputInterface,
        flags.useGuard,
        flags.useGuardNegation,
        flags.useUpdate,
        flags.useTargetState,
        flags.useAppFilter
    };
    const bool result = engine->solve(query);
    save_solution_if(result);

    STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true);)
    PLAJA_ASSERT(not result or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), sourceState.get(), actionLabel, actionOpId, updateIndex, lastSolution->get_solution_size() > 1 ? targetState.get() : nullptr, lastSolution->get_solution_size() > 1 ? lastSolution->get_solution(1) : nullptr) or not solutionChecker->check_policy(*lastSolution->get_solution(0), actionLabel, nullptr))
    STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)

    return result;
}

bool FnnCheckerPB::check_relaxed() { return check_(); }
