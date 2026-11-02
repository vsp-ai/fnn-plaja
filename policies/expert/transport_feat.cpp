#include "policies/expert/expert_policy.h"

ActionLabel_type TransportFeat::evaluate(const StateBase& state_values) const {
    // As Transport, plus index number_of_locations + 3: num_packages_behind_truck
    const int number_of_locations = state_values.get_int_state_size() - 5;
    return transport_action(state_values, number_of_locations, state_values.get_int(number_of_locations + 3) > 0);
}
