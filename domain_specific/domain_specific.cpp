#include "domain_specific.h"

std::vector<double> DomainSpecific::action_values(const StateValues& state_values) {
    std::vector<double> values(jani_->get_num_output_features(), 0.0);
    const ActionLabel_type action = get_action(state_values);
    if (jani_->is_learned(action)) {
        values[jani_->get_output_index(action)] = 1.0;
    }
    return values;
}

std::unique_ptr<DomainSpecific> DomainSpecific::create(const std::string& name, const Model* model, const Jani2Interface* jani) {
    std::cout << "Constructing manual policy for: " << name << std::endl;
    if (name == "Transport")         return std::make_unique<Transport>(model, jani);
    if (name == "OneWayLinePark")    return std::make_unique<OneWayLinePark>(model, jani);
    if (name == "TwoWayLinePark")    return std::make_unique<TwoWayLinePark>(model, jani);
    if (name == "Transport_Feat")    return std::make_unique<Transport_Feat>(model, jani);

    throw std::invalid_argument("Unknown domain-specific policy: " + name);
}