//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_POLICY_RESTRICTION_H
#define PLAJA_POLICY_RESTRICTION_H

#include <memory>
#include "assertions.h"
#include "core/forward_factories.h"
#include "policies/interface/jani_2_interface.h"
#include "core/forward_states.h"
#include "core/state_base.h"
#include "core/using_core.h"
#include "policies/forward_policy.h"
#include "policies/policy.h"
#include <iostream>
class PolicyRestriction {

private:
    const Policy* policy;

public:
    explicit PolicyRestriction(const PLAJA::Configuration& config, const Jani2Interface& policy_interface);
    ~PolicyRestriction();
    DELETE_CONSTRUCTOR(PolicyRestriction)

    [[nodiscard]] bool is_enabled(const StateBase& state, ActionLabel_type action_label) const;

    [[nodiscard]] PLAJA::floating get_selection_delta() const;

    [[nodiscard]] bool is_chosen_with_tolerance() const;

    FCT_IF_DEBUG(void dump_cache() const;)

};

#endif //PLAJA_POLICY_RESTRICTION_H
