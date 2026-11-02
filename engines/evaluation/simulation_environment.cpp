//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "engines/evaluation/simulation_environment.h"
#include "model/ast/iterators/model_iterator.h"
#include "model/ast/action.h"
#include "core/utils/rng.h"
#include "core/configuration.h"
#include "core/state.h"
#include "model/model_information.h"
#include "successor_generation/action_op.h"
#include "successor_generation/successor_generator_c.h"
#include "globals.h"

SimulationEnvironment::SimulationEnvironment(const PLAJA::Configuration& config, const Model& model_ref):
    model(&model_ref)
    , modelInfo(&model->get_model_information())
    , rng(PLAJA_GLOBAL::rng.get()) {

    if (config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
        successorGenerator = config.get_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);
        PLAJA_ASSERT(&successorGenerator->get_model() == model)
    } else {
        successorGenerator = config.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, model_ref));
    }

    stateRegistry = std::make_unique<StateRegistry>(modelInfo->get_ranges_int(), modelInfo->get_lower_bounds_int(), modelInfo->get_upper_bounds_int(), modelInfo->get_floating_state_size());
    initialState = stateRegistry->set_state(modelInfo->get_initial_values());
}

SimulationEnvironment::~SimulationEnvironment() = default;

//

State SimulationEnvironment::get_state(const StateID_type id) const { return stateRegistry->lookup_state(id); }

State SimulationEnvironment::get_state(const StateValues& state_values) const { return stateRegistry->add_state(state_values); }

// computations:

bool SimulationEnvironment::is_applicable(const StateBase& state, ActionLabel_type action_label) const {
    return successorGenerator->is_applicable(state, action_label);
}

//

std::vector<ActionLabel_type> SimulationEnvironment::extract_applicable_actions(const StateBase& state, bool including_silent_action) const {
    successorGenerator->explore(state);
    std::vector<ActionLabel_type> applicable_actions;

    // SILENT
    if (including_silent_action) {
        if (successorGenerator->is_applicable(ACTION::silentAction)) {
            applicable_actions.push_back(ACTION::silentAction);
        }
    }

    // NON-SILENT
    for (auto it = ModelIterator::actionIterator(*model); !it.end(); ++it) {
        const ActionID_type action_id = it->get_id();
        if (successorGenerator->is_applicable(action_id)) {
            applicable_actions.push_back(action_id);
        }
    }

    return applicable_actions;
}

//

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index) const {
    if (update_index == ACTION::noneUpdate) {
        // random update
        PLAJA::Prob_type prob = rng->prob();
        for (auto it = action_op.updateIterator(); !it.end(); ++it) {
            const PLAJA::Prob_type prob_update = it.prob(source);
            if (prob_update > prob) { 
                auto successor = successorGenerator->compute_successor(it.update(), source);
                return successor;
            }
            else { prob -= prob_update; }
        }
        PLAJA_ABORT
    } else {
        auto successor = successorGenerator->compute_successor(action_op.get_update(update_index), source);
        return successor;
    }
}

std::unique_ptr<State> SimulationEnvironment::compute_successor(const State& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT(is_applicable(source, action_label))
    return compute_successor_if_applicable(source, action_label);
}

/**********************************************************************************************************************/

std::unique_ptr<State> SimulationEnvironment::compute_successor_if_applicable(const State& source, ActionLabel_type action_label) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(source))
    const auto applicable_ops = successorGenerator->get_action(source, action_label);
    if (applicable_ops.empty()) { return nullptr; }
    else {
        const auto chosen_index = rng->index(applicable_ops.size()); // NOLINT(cppcoreguidelines-narrowing-conversions)
        auto current_index = 0;
        for (const auto* action_op: applicable_ops) {
            if (current_index == chosen_index) { return compute_successor(source, *action_op, ACTION::noneUpdate); }
            else { ++current_index; }
        }
        PLAJA_ABORT
    }
}
