//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "engines/verification/smt/successor_generator_marabou_pa.h"
#include "core/utils/utils.h"
#include "engines/verification/smt/action_op_marabou_pa.h"
#include "engines/verification/smt/model_marabou_pa.h"

MARABOU_IN_PLAJA::SuccessorGeneratorPA::SuccessorGeneratorPA(const SuccessorGeneratorC& successor_generator, const ModelMarabouPA& model):
    SuccessorGenerator(successor_generator, model, false) {

    initialize_operators();
}

MARABOU_IN_PLAJA::SuccessorGeneratorPA::~SuccessorGeneratorPA() = default;

//

const ModelMarabouPA& MARABOU_IN_PLAJA::SuccessorGeneratorPA::get_model_pa() const { return PLAJA_UTILS::cast_ref<ModelMarabouPA>(get_model()); }

std::unique_ptr<ActionOpMarabou> MARABOU_IN_PLAJA::SuccessorGeneratorPA::initialize_op(const ActionOp& action_op) const { return std::make_unique<ActionOpMarabouPA>(action_op, get_model_pa()); }

//

const ActionOpMarabouPA& MARABOU_IN_PLAJA::SuccessorGeneratorPA::get_action_op_pa(ActionOpID_type op_id) const { return PLAJA_UTILS::cast_ref<ActionOpMarabouPA>(get_action_op(op_id)); }