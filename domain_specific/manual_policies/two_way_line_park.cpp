#include "../domain_specific.h"
#include "../../parser/ast/variable_declaration.h"

/* Safety-first policy for TwoWayLine with Parking.  The nondeterministic
 * variants can drop packages while the truck moves and can fail at the line
 * boundaries when velocity is too high.  The policy stops after each one-cell
 * move, reverses from rest for missed packages, and parks only when there is
 * nothing left to recover.
 */

// Action ordering for TwoWayLine:
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


ActionLabel_type TwoWayLinePark::get_action(const StateValues& state_values) {
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

    bool package_behind = false;
    for (int loc = 0; loc < truck_position; ++loc) {
        if (state_values.get_int(loc + 1) > 0) {
            package_behind = true;
            break;
        }
    }

    // At the goal, drop carried packages.  If packages are still behind the
    // truck, reverse from rest; otherwise park as a safe terminal action.
    if (truck_position == goal_position) {
        if (truck_velocity == 0 && truck_load > 0) {
            return ACTION_DROP;
        }
        if (truck_velocity == 0 && package_behind) {
            return ACTION_DECELERATE;
        }
        if (truck_velocity == 0 && truck_load == 0) {
            return ACTION_PARK;
        }
        if (truck_velocity < 0) {
            return ACTION_ACCELERATE;
        }
        return ACTION_DROP;
    }

    // Avoid boundary failures: if the current velocity already cannot be
    // stopped before leaving the line, stall instead of forcing aux_vel = -1.
    const int distance_to_right_edge = goal_position - truck_position;
    const int distance_to_left_edge = truck_position;
    if (truck_velocity > distance_to_right_edge || -truck_velocity > distance_to_left_edge) {
        return ACTION_DROP;
    }

    // Brake down unexpected high velocity while there is still room.
    if (truck_velocity > 1) {
        return ACTION_DECELERATE;
    }
    if (truck_velocity < -1) {
        return ACTION_ACCELERATE;
    }

    // Safety-first: stop after each one-cell move so dropped packages are
    // recoverable before the truck drives past them.
    if (truck_velocity == 1) {
        return ACTION_DECELERATE;
    }
    if (truck_velocity == -1) {
        return ACTION_ACCELERATE;
    }

    // Velocity 0 from here on.
    if (packages_here > 0 && truck_position != goal_position) {
        return ACTION_PICKUP;
    }

    // Go back for missing packages only from rest.
    if (package_behind) {
        return ACTION_DECELERATE;
    }

    // Otherwise make one forward step toward the goal.
    return ACTION_ACCELERATE;
}


std::vector<double> TwoWayLinePark::action_values(const StateValues& state_values) {
    const int number_of_locations = line_location_count(*model_);
    const int truck_position = state_values.get_int(number_of_locations + 1);
    const int truck_load = state_values.get_int(number_of_locations + 2);
    const int truck_velocity = state_values.get_int(number_of_locations + 3);
    const int goal_position = number_of_locations - 1;
    const int packages_here = state_values.get_int(truck_position + 1);
    const int distance_to_right_edge = goal_position - truck_position;
    const int distance_to_left_edge = truck_position;

    bool package_behind = false;
    for (int loc = 0; loc < truck_position; ++loc) {
        if (state_values.get_int(loc + 1) > 0) { package_behind = true; break; }
    }

    std::vector<double> scores(jani_->get_num_output_features(), BAD_ACTION);
    const bool at_goal = truck_position == goal_position;
    const bool stopped = truck_velocity == 0;
    const bool motion_unsafe = truck_velocity > distance_to_right_edge || -truck_velocity > distance_to_left_edge;

    if (!at_goal && stopped && packages_here > 0) {
        set_score(*jani_, scores, ACTION_PICKUP, BEST_ACTION);
    }

    if (stopped && truck_load > 0) {
        set_score(*jani_, scores, ACTION_DROP, at_goal ? BEST_ACTION : NEUTRAL_ACTION);
    }

    if (at_goal && stopped && truck_load == 0 && !package_behind) {
        set_score(*jani_, scores, ACTION_PARK, BEST_ACTION);
    } else {
        set_score(*jani_, scores, ACTION_PARK, BAD_ACTION);
    }

    if (!motion_unsafe) {
        if (truck_velocity > 0) {
            set_score(*jani_, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*jani_, scores, ACTION_MOVE, NEUTRAL_ACTION);
            set_score(*jani_, scores, ACTION_ACCELERATE, truck_velocity > 1 ? BAD_ACTION : NEUTRAL_ACTION);
        } else if (truck_velocity < 0) {
            set_score(*jani_, scores, ACTION_ACCELERATE, BEST_ACTION);
            set_score(*jani_, scores, ACTION_MOVE, NEUTRAL_ACTION);
            set_score(*jani_, scores, ACTION_DECELERATE, truck_velocity < -1 ? BAD_ACTION : NEUTRAL_ACTION);
        } else if (at_goal && package_behind) {
            set_score(*jani_, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*jani_, scores, ACTION_MOVE, NEUTRAL_ACTION);
        } else if (!at_goal && packages_here <= 0) {
            if (package_behind) {
                set_score(*jani_, scores, ACTION_DECELERATE, truck_position > 0 ? BEST_ACTION : BAD_ACTION);
                set_score(*jani_, scores, ACTION_ACCELERATE, NEUTRAL_ACTION);
            } else {
                set_score(*jani_, scores, ACTION_ACCELERATE, truck_position < goal_position ? BEST_ACTION : BAD_ACTION);
                set_score(*jani_, scores, ACTION_DECELERATE, truck_position > 0 ? NEUTRAL_ACTION : BAD_ACTION);
            }
            set_score(*jani_, scores, ACTION_MOVE, NEUTRAL_ACTION);
        }
    }

    return scores;
}
