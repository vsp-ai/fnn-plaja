#include "../domain_specific.h"
#include "../../parser/ast/variable_declaration.h"

/* Safety-first policy for OneWayLine with Parking.  The nondeterministic
 * variants can drop packages while the truck moves, so this policy stops after
 * each one-cell movement instead of coasting.  If the truck reaches the goal
 * empty after packages were left behind, parking is used as a safe terminal
 * fallback because one-way recovery is no longer possible.
 */


// Action ordering for OneWayLine with Parking:
namespace {
    int line_location_count(const Model& model) {
        for (auto it = model.variableIterator(); !it.end(); ++it) {
            if (it.variable()->get_name() == "truck_0") {
                return static_cast<int>(it.variable_index()) - 1;
            }
        }
        throw std::invalid_argument("Line-world expert requires truck_0");
    }

    constexpr int ACTION_PICKUP   = 0;
    constexpr int ACTION_DROP     = 1;
    constexpr int ACTION_ACCELERATE  = 2;
    constexpr int ACTION_DECELERATE = 3;
    constexpr int ACTION_MOVE = 4;
    constexpr int ACTION_PARK = 5;
    constexpr double BAD_ACTION = -1.0;
    constexpr double NEUTRAL_ACTION = 0.0;
    constexpr double BEST_ACTION = 1.0;

    void set_score(const Jani2Interface& jani, std::vector<double>& scores, ActionLabel_type label, double score) {
        if (jani.is_learned(label)) { scores[jani.get_output_index(label)] = score; }
    }
}


ActionLabel_type OneWayLinePark::get_action(const StateValues& state_values) {
    // Derive road length from the truck index: optional ice indicators may
    // follow the usual state variables. State layout (automaton at index 0):
    //   index 0                       : automaton state
    //   index 1 .. number_of_locations: package count at each location
    //   index number_of_locations + 1 : truck position
    //   index number_of_locations + 2 : truck load
    //   index number_of_locations + 3: truck_velocity
    const int number_of_locations   = line_location_count(*model_);
    const int truck_position        = state_values.get_int(number_of_locations + 1);
    const int truck_load            = state_values.get_int(number_of_locations + 2);
    const int truck_velocity        = state_values.get_int(number_of_locations + 3);
    const int goal_position         = number_of_locations - 1;

    // Package count at the truck's current location.
    // Locations are stored at indices 1 .. number_of_locations, so location k
    // lives at index k + 1.
    const int packages_here = state_values.get_int(truck_position + 1);

    // At the goal, dropping and then parking are the only useful safe actions.
    if (truck_position == goal_position) {
        if (truck_velocity == 0 && truck_load > 0) {
            return ACTION_DROP;
        }
        if (truck_velocity == 0 && truck_load == 0) {
            return ACTION_PARK;
        }
        return ACTION_DROP;
    }

    // If the state is already too fast to stop before the end, any motion action
    // is unsafe in the OWL models.  Stall rather than force aux_vel = -1.
    const int remaining_distance = goal_position - truck_position;
    if (truck_velocity > remaining_distance) {
        return ACTION_DROP;
    }

    // Brake down unexpected high velocity while there is still room.
    if (truck_velocity > 1) {
        return ACTION_DECELERATE;
    }

    // Safety-first: stop after each one-cell move.  This makes dropped packages
    // recoverable and keeps the truck from reaching the end with positive velocity.
    if (truck_velocity == 1) {
        return ACTION_DECELERATE;
    }

    // ---- Velocity 0 from here on. ----

    // Stopped on top of a package -> pick it up.
    if (packages_here > 0) {
        return ACTION_PICKUP;
    }

    // Move one cell at a time: accelerate from rest, then decelerate next step.
    return ACTION_ACCELERATE;
}


std::vector<double> OneWayLinePark::action_values(const StateValues& state_values) {
    const int number_of_locations = line_location_count(*model_);
    const int truck_position = state_values.get_int(number_of_locations + 1);
    const int truck_load = state_values.get_int(number_of_locations + 2);
    const int truck_velocity = state_values.get_int(number_of_locations + 3);
    const int goal_position = number_of_locations - 1;
    const int packages_here = state_values.get_int(truck_position + 1);
    const int remaining_distance = goal_position - truck_position;

    std::vector<double> scores(jani_->get_num_output_features(), BAD_ACTION);
    const bool at_goal = truck_position == goal_position;
    const bool stopped = truck_velocity == 0;
    const bool motion_unsafe = at_goal || truck_velocity > remaining_distance;

    if (!at_goal && stopped && packages_here > 0) {
        set_score(*jani_, scores, ACTION_PICKUP, BEST_ACTION);
    }

    if (stopped && truck_load > 0) {
        set_score(*jani_, scores, ACTION_DROP, at_goal ? BEST_ACTION : BAD_ACTION);
    }

    if (at_goal && stopped && truck_load == 0) {
        set_score(*jani_, scores, ACTION_PARK, BEST_ACTION);
    }

    if (!motion_unsafe) {
        if (truck_velocity > 0) {
            set_score(*jani_, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*jani_, scores, ACTION_MOVE, BAD_ACTION);
            set_score(*jani_, scores, ACTION_ACCELERATE, BAD_ACTION);
        } else if (packages_here <= 0) {
            set_score(*jani_, scores, ACTION_ACCELERATE, BEST_ACTION);
            set_score(*jani_, scores, ACTION_MOVE, BAD_ACTION);
            set_score(*jani_, scores, ACTION_DECELERATE, BAD_ACTION);
        }
    }

    return scores;
}
