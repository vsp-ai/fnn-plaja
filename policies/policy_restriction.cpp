//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "policies/policy_restriction.h"
#include "core/configuration.h"
#include "model/property_information.h"
#include "policies/policy.h"


PolicyRestriction::PolicyRestriction(const PLAJA::Configuration& config, const Jani2Interface& policy_interface):
    policy(&policy_interface.load_policy(config)) {}

PolicyRestriction::~PolicyRestriction() = default;

bool PolicyRestriction::is_enabled(const StateBase& state, ActionLabel_type action_label) const { return policy->is_chosen(state, action_label, PLAJA_NN::argMaxPrecision); }

PLAJA::floating PolicyRestriction::get_selection_delta() const { return policy->get_selection_delta(); }

bool PolicyRestriction::is_chosen_with_tolerance() const { return policy->is_chosen_with_tolerance(); }

FCT_IF_DEBUG(void PolicyRestriction::dump_cache() const { policy->dump_cached_outputs(); })

