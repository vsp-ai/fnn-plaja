#include "fnn_sat_checker_pb.h"
#include "../../../globals.h"
#include "pb_constraints/pb_engine.h"
#include "pb_constraints/pb_query_builder.h"
#include "../../factories/configuration.h"
#include "../../information/jani2fnn/jani_2_fnn.h"
#include "../pa_states/abstract_state.h"
#include "../../smt/base/solution_checker_instance.h"
#include "../../states/state_values.h"
#include "../solution_checker_pa.h"
#include "../solution_checker_instance_pa.h"
#include "../smt/model_z3_pa.h"
#include "../successor_generation/action_op/action_op_pa.h"

FnnSatCheckerPB::FnnSatCheckerPB(const Jani2FNN& jani_2_fnn,
                                   const PLAJA::Configuration& config)
    : SatChecker(jani_2_fnn, config)
    , modelZ3(nullptr)
    , jani2FNN(&jani_2_fnn)
    , actionOp(nullptr)
    , engine(std::make_unique<FNN_PB::Engine>(config)) {
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    modelZ3 = std::static_pointer_cast<ModelZ3PA>(
        config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3));
    PLAJA_ASSERT(modelZ3)
    modelZ3->increment();
}

FnnSatCheckerPB::~FnnSatCheckerPB() = default;

void FnnSatCheckerPB::set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) {
    if (not modelZ3->get_op_applicability()) { modelZ3->share_op_applicability(op_app); }
}

void FnnSatCheckerPB::increment() {
    modelZ3->increment();
    engine->increment();
}

void FnnSatCheckerPB::push() { flagsStack.push_back(flags); }

void FnnSatCheckerPB::pop() {
    PLAJA_ASSERT(not flagsStack.empty())
    flags = flagsStack.back();
    flagsStack.pop_back();
}

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
void FnnSatCheckerPB::add_non_terminal_condition() { flags.requireNonTerminal = true; }
#endif

void FnnSatCheckerPB::set_operator(ActionOpID_type action_op_id) {
    set_action_op_id = action_op_id;
    actionOp = &modelZ3->get_action_op_pa(action_op_id);
    PLAJA_ASSERT(set_output_index == jani2FNN->get_output_index(actionOp->_action_label()))
}

void FnnSatCheckerPB::add_source_state(const AbstractState& source) {
    set_source_state = source.to_ptr();
}

bool FnnSatCheckerPB::add_guard() {
    PLAJA_ASSERT(actionOp)
    flags.useGuard = true;
    flags.useGuardNegation = false;
    return not actionOp->_guards().is_true();
}

bool FnnSatCheckerPB::add_guard_negation() {
    PLAJA_ASSERT(actionOp)
    flags.useGuard = false;
    flags.useGuardNegation = true;
    return not actionOp->_guards().is_true();
}

void FnnSatCheckerPB::add_update() {
    PLAJA_ASSERT(actionOp)
    PLAJA_ASSERT(set_update_index != ACTION::noneUpdate)
    flags.useUpdate = true;
}

void FnnSatCheckerPB::add_target_state(const AbstractState& target) {
    set_target_state = target.to_ptr();
    flags.useTargetState = true;
}

bool FnnSatCheckerPB::add_output_interface() {
    PLAJA_ASSERT(set_action_label != ACTION::noneLabel)
    flags.useOutputInterface = true;
    flags.useAppFilter = jani2FNN->_do_applicability_filtering();
    return true;
}

void FnnSatCheckerPB::save_solution_if(bool result) {
    lastSolution = nullptr;
    const auto& source_values = engine->witness_source();
    const auto& target_values = engine->witness_target();
    const auto* target = flags.useTargetState ? set_target_state.get() : nullptr;
    if (not result or not solutionChecker or not set_source_state or source_values.empty()
        or (target and target_values.empty())) { return; }

    auto instance = std::make_unique<SolutionCheckerInstancePa>(*solutionChecker, set_source_state.get(), set_action_label,
                                                                set_action_op_id, set_update_index, target);
    const auto to_state = [&instance](const std::vector<PLAJA::integer>& values) {
        auto state = std::make_unique<StateValues>(instance->get_solution_constructor());
        for (VariableIndex_type index = 0; index < values.size(); ++index) { state->assign_int<false>(index, values[index]); }
        return state;
    };
    instance->set_solution(to_state(source_values), 0);
    if (target) { instance->set_solution(to_state(target_values), 1); }
    lastSolution = std::move(instance);
}

bool FnnSatCheckerPB::check_() {
    const FNN_PB::QueryContext query {
        jani2FNN,
        modelZ3.get(),
        set_source_state.get(),
        set_target_state.get(),
        actionOp,
        set_action_label,
        set_output_index,
        set_action_op_id,
        set_update_index,
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
    PLAJA_ASSERT(not result or not lastSolution or solutionChecker->check(*lastSolution->get_solution(0), set_source_state.get(), set_action_label, set_action_op_id, set_update_index, lastSolution->get_solution_size() > 1 ? set_target_state.get() : nullptr, lastSolution->get_solution_size() > 1 ? lastSolution->get_solution(1) : nullptr) or not solutionChecker->check_policy(*lastSolution->get_solution(0), set_action_label, nullptr))
    STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame();)

    return result;
}

bool FnnSatCheckerPB::check_relaxed() { return check_(); }
