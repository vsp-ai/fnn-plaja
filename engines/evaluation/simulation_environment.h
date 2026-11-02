//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SIMULATION_ENVIRONMENT_H
#define PLAJA_SIMULATION_ENVIRONMENT_H

#include <memory>
#include <vector>
#include "model/ast/forward_ast.h"
#include "model/using_parser.h"
#include "assertions.h"
#include "core/using_core.h"
#include "core/forward_factories.h"
#include "model/forward_information.h"
#include "core/forward_states.h"
#include "successor_generation/forward_successor_generation.h"

class SimulationEnvironment {
private:

    const Model* model;
    const ModelInformation* modelInfo;

    std::unique_ptr<State> initialState;
    std::unique_ptr<StateRegistry> stateRegistry; // Own structures for now, may think about sharing later.

    std::shared_ptr<SuccessorGeneratorC> successorGenerator;

    // simulation
    class RandomNumberGenerator* rng;

public:
    explicit SimulationEnvironment(const PLAJA::Configuration& config, const Model& model);
    ~SimulationEnvironment();
    DELETE_CONSTRUCTOR(SimulationEnvironment)

    /* Getter: */

    [[nodiscard]] inline const State& get_initial_state() const {
        PLAJA_ASSERT(initialState)
        return *initialState;
    }

    State get_state(StateID_type id) const;

    State get_state(const StateValues& state_values) const;

    /* Simulation: */

    [[nodiscard]] bool is_applicable(const StateBase& state, ActionLabel_type action_label) const;

    [[nodiscard]] std::vector<ActionLabel_type> extract_applicable_actions(const StateBase& state, bool including_silent_action) const;

    // Strictly speaking not const, as rng may be invoked, which is however secondary:

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, const ActionOp& action_op, UpdateIndex_type update_index = ACTION::noneUpdate) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor(const State& source, ActionLabel_type action_label) const;

    [[nodiscard]] std::unique_ptr<State> compute_successor_if_applicable(const State& source, ActionLabel_type action_label) const;

};

#endif //PLAJA_SIMULATION_ENVIRONMENT_H
