//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "policy_restriction.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/state.h"
#include "../../information/property_information.h"
#include "policy.h"


PolicyRestriction::PolicyRestriction(const PLAJA::Configuration& config, const Jani2Interface& policy_interface):
    policy(&policy_interface.load_policy(config))
    , cachedState(STATE::none) {
        std::cout << "Interface File: " << policy->get_interface().get_interface_file() << std::endl;
    }

PolicyRestriction::~PolicyRestriction() = default;

//

const Jani2Interface& PolicyRestriction::get_interface() const { return policy->get_interface(); }

//

//

bool PolicyRestriction::is_enabled(const State& state, ActionLabel_type action_label) const {
    const StateID_type queried_state_id = state.get_id_value();
    if (queried_state_id != cachedState) {
        cachedState = queried_state_id;
        policy->evaluate(state);
    }
    return policy->is_chosen(action_label, PLAJA_NN::argMaxPrecision);
}

bool PolicyRestriction::is_enabled(const StateBase& state, ActionLabel_type action_label) const { return policy->is_chosen(state, action_label, PLAJA_NN::argMaxPrecision); }

PLAJA::floating PolicyRestriction::get_selection_delta() const { return policy->get_selection_delta(); }

bool PolicyRestriction::is_chosen_with_tolerance() const { return policy->is_chosen_with_tolerance(); }

FCT_IF_DEBUG(void PolicyRestriction::dump_cache() const { policy->dump_cached_outputs(); })

/* */

void PolicyRestriction::evaluate(const StateBase& state) { policy->evaluate(state); }

bool PolicyRestriction::is_enabled(ActionLabel_type action_label) const { return policy->is_chosen(action_label); }

