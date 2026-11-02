//
// This file is part of the PlaJA code base.
//

#include "policies/fnn/fnn_policy.h"

#include <algorithm>
#include <utility>
#include <limits>

#include "core/utils/floating_utils.h"
#include "policies/fnn/jani_2_fnn.h"
#include "core/state_base.h"
#include "successor_generation/successor_generator_c.h"
#include "policies/fnn/formularized_network.h"

#include "core/configuration.h"

FNNPolicy::FNNPolicy(const PLAJA::Configuration& config, const Jani2FNN& fnn_interface)
    : fnn(&fnn_interface.load_fnn())
    , fnnInterface(&fnn_interface)
    , successorGenerator(nullptr)
    , cachedLabel(ACTION::nullAction)
    , selectionDelta(0)
    , chosenWithTolerance(false) {
    if (fnnInterface->_do_applicability_filtering()) {
        if (not config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
            config.set_sharable<SuccessorGeneratorC>(
                PLAJA::SharableKey::SUCC_GEN,
                std::make_shared<SuccessorGeneratorC>(config, fnnInterface->get_model()));
        }

        successorGenerator = config.get_sharable_as_const<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);

        PLAJA_ASSERT(&successorGenerator->get_model() == &fnnInterface->get_model())
    }
}

FNNPolicy::~FNNPolicy() = default;

ActionLabel_type FNNPolicy::evaluate_argmax(const std::vector<Floating_type>& outputs) const {
    PLAJA_ASSERT(not outputs.empty())
    Floating_type max_score = -std::numeric_limits<Floating_type>::infinity();
    std::size_t max_idx = -1;
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        if (outputs[i] > max_score) {
            max_score = outputs[i];
            max_idx = i;
        }
    }
    PLAJA_ASSERT(max_idx < fnnInterface->get_num_output_features())
    return fnnInterface->get_output_label(max_idx);
}

ActionLabel_type FNNPolicy::evaluate_app_filtering(const StateBase& state,
                                                  const std::vector<Floating_type>& outputs,
                                                  std::vector<bool>& enabled_labels) const {
    PLAJA_ASSERT(successorGenerator)

    const auto num_outputs = outputs.size();

    std::vector<std::pair<Floating_type, OutputIndex_type>> sorted;
    sorted.reserve(num_outputs);
    enabled_labels.reserve(num_outputs);

    for (OutputIndex_type oi = 0; oi < num_outputs; ++oi) {
        sorted.emplace_back(outputs[oi], oi);
        enabled_labels.push_back(successorGenerator->is_applicable(state, fnnInterface->get_output_label(oi)));
    }

    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return (a.first > b.first) or (a.first == b.first and a.second < b.second);
              });

    for (const auto& vi : sorted) {
        if (enabled_labels[vi.second]) {
            return fnnInterface->get_output_label(vi.second);
        }
    }

    // no applicable op: fall back to highest-scoring regardless
    return fnnInterface->get_output_label(sorted.front().second);
}

bool FNNPolicy::is_chosen_argmax(const std::vector<Floating_type>& outputs,
                                 ActionLabel_type action_label,
                                 double tolerance) const {
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    const auto label_index = fnnInterface->get_output_index(action_label);
    const auto cached_label_index = fnnInterface->get_output_index(cachedLabel);
    const auto label_value = outputs[label_index];
    const auto value_chosen_label = outputs[cached_label_index];
    PLAJA_ASSERT(value_chosen_label >= label_value)
    selectionDelta = value_chosen_label - label_value;
    PLAJA_ASSERT(selectionDelta >= 0)
    return PLAJA_FLOATS::is_zero(selectionDelta, tolerance);
}

bool FNNPolicy::is_chosen_app_filtering(const std::vector<Floating_type>& outputs,
                                        const std::vector<bool>& enabled_labels,
                                        ActionLabel_type action_label,
                                        double tolerance) const {
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    const auto label_index = fnnInterface->get_output_index(action_label);
    const auto cached_label_index = fnnInterface->get_output_index(cachedLabel);
    const auto label_value = outputs[label_index];
    const auto value_chosen_label = outputs[cached_label_index];
    PLAJA_ASSERT(enabled_labels[cached_label_index] or not enabled_labels[label_index])

    if (enabled_labels[cached_label_index] and not enabled_labels[label_index]) { return false; }

    PLAJA_ASSERT(value_chosen_label >= label_value)
    selectionDelta = value_chosen_label - label_value;
    PLAJA_ASSERT(selectionDelta >= 0)
    return PLAJA_FLOATS::is_zero(selectionDelta, tolerance);
}

ActionLabel_type FNNPolicy::evaluate(const StateBase& state) const {
    cachedOutputs.clear();
    cachedEnabledLabels.clear();
    selectionDelta = 0;
    chosenWithTolerance = false;
    cachedOutputs = fnn->evaluate(state);

    cachedLabel = ( fnnInterface->_do_applicability_filtering()
                        ? evaluate_app_filtering(state, cachedOutputs, cachedEnabledLabels)
                        : evaluate_argmax(cachedOutputs) );

    return cachedLabel;
}

std::vector<double> FNNPolicy::action_values(const StateBase& state) const {
    selectionDelta = 0;
    chosenWithTolerance = false;
    const auto out = fnn->evaluate(state);
    return std::vector<double>(out.begin(), out.end());
}

bool FNNPolicy::is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance) const {
    evaluate(state);
    return is_chosen(action_label, tolerance);
}

bool FNNPolicy::is_chosen(ActionLabel_type action_label, double tolerance) const {
    selectionDelta = 0;
    chosenWithTolerance = false;

    // if action is not learned or the action is the decided by the policy, return true.
    if (not fnnInterface->is_learned(action_label) or action_label == cachedLabel) {
        return true;
    }

    // if no tolerance, return false (policy decided differently)
    if (tolerance <= 0) { return false; }

    // otherwise, check with tolerance
    const bool is_selected = (
        fnnInterface->_do_applicability_filtering()
            ? is_chosen_app_filtering(cachedOutputs, cachedEnabledLabels, action_label, tolerance)
            : is_chosen_argmax(cachedOutputs, action_label, tolerance));
    chosenWithTolerance = is_selected and selectionDelta > 0;
    return is_selected;
}

#ifndef NDEBUG

#include <sstream>
#include "core/utils/utils.h"

void FNNPolicy::dump_cached_outputs() const {
    std::stringstream ss;
    ss << "Policy output: ";
    for (auto val: cachedOutputs) { ss << val << PLAJA_UTILS::commaString; }
    ss << " chosen label: " << cachedLabel;
    ss << ", last selection delta: " << selectionDelta << PLAJA_UTILS::dotString;
    PLAJA_LOG(ss.str())
}

#endif
