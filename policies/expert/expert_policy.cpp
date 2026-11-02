#include "policies/expert/expert_policy.h"
#include <stdexcept>
#include "model/ast/variable_declaration.h"
#include "core/utils/utils.h"

std::vector<double> ExpertPolicy::action_values(const StateBase& state_values) const {
    std::vector<double> values(interface->get_num_output_features(), 0.0);
    const ActionLabel_type action = evaluate(state_values);
    if (interface->is_learned(action)) {
        values[interface->get_output_index(action)] = 1.0;
    }
    return values;
}

std::unique_ptr<ExpertPolicy> ExpertPolicy::create(const std::string& name, const Model* model, const Jani2Interface* interface) {
    PLAJA_LOG("Constructing manual policy for: " + name)
    if (name == "Transport") return std::make_unique<Transport>(model, interface);
    if (name == "OneWayLinePark") return std::make_unique<OneWayLinePark>(model, interface);
    if (name == "TwoWayLinePark") return std::make_unique<TwoWayLinePark>(model, interface);
    if (name == "Transport_Feat") return std::make_unique<TransportFeat>(model, interface);

    throw std::invalid_argument("Unknown domain-specific policy: " + name);
}

int LINE_PARK::line_location_count(const Model& model) {
    for (auto it = model.variableIterator(); !it.end(); ++it) {
        if (it.variable()->get_name() == "truck_0") {
            return static_cast<int>(it.variable_index()) - 1;
        }
    }
    throw std::invalid_argument("Line-world expert requires truck_0");
}

// The nop variants have no park action; the expert stalls with DROP instead.
ActionLabel_type LINE_PARK::park_or_stall(const Model& model) { return model.get_number_actions() > ACTION_PARK ? ACTION_PARK : ACTION_DROP; }

void LINE_PARK::set_score(const Jani2Interface& interface, std::vector<double>& scores, ActionLabel_type label, double score) {
    if (interface.is_learned(label)) { scores[interface.get_output_index(label)] = score; }
}
