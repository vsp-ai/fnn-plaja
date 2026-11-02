//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <unordered_set>
#include "policies/interface/jani_2_interface.h"
#include "policies/interface/jani_2_interface_parser.h"
#include "model/ast/variable_declaration.h"

Jani2Interface::Jani2Interface(const Model& model_):
    model(&model_)
    , doApplicabilityFiltering(false) {
}

void Jani2Interface::parse(const Jani2InterfaceParser& parser) {
    doApplicabilityFiltering = parser.parse_applicability_filtering();
    inputFeatureMapping = parser.parse_input_interface();
    outputsToLabels = parser.parse_output_interface();
    compute_labels_to_outputs();
    compute_layer_sizes();
    compute_extended_inputs();
}

void Jani2Interface::compute_labels_to_outputs() {
    for (OutputIndex_type output_index = 0; output_index < outputsToLabels.size(); ++output_index) {
        labelsToOutputs[outputsToLabels[output_index]] = output_index;
    }
}

void Jani2Interface::compute_extended_inputs() {
    PLAJA_ASSERT(not layerSizes.empty()) // Must be computed first.

    if (not doApplicabilityFiltering) { return; }

    const auto& model_info = get_model_info();

    // Include locs?
    bool include_locs = false;
    for (auto loc_it = model_info.locationIndexIterator(); !loc_it.end(); ++loc_it) {
        if (loc_it.range() > 1) {
            include_locs = true;
            break;
        }
    }

    std::unordered_set<VariableIndex_type> actual_input_features;

    for (auto it = init_input_feature_iterator(false); !it.end(); ++it) { actual_input_features.insert(it.get_input_feature_index()); }

    const auto guard_vars = model->collect_guard_variables(include_locs);

    for (auto it = model_info.stateIndexIterator(include_locs); !it.end(); ++it) {
        const auto state_index = it.state_index();

        if (not actual_input_features.count(state_index) and guard_vars.count(state_index)) {

            if (it.is_location()) {
                inputFeatureMapping.push_back(std::make_unique<InputFeatureToLocation>(it.location_index()));
            } else {

                const auto id = model_info.get_id(state_index);

                if (model_info.is_array(id)) {
                    const auto& var_decl = *model->get_variable_by_id(id);
                    inputFeatureMapping.push_back(std::make_unique<InputFeatureToArray>(var_decl, state_index - var_decl.get_index()));
                } else { inputFeatureMapping.push_back(std::make_unique<InputFeatureToVariable>(*model->get_variable_by_id(id))); }

            }

        }

    }
}
