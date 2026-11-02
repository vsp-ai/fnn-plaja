#include "policies/expert/expert_policy.h"

using namespace LINE_PARK;

/* Safety-first policy for TwoWayLine with Parking.  The nondeterministic
 * variants can drop packages while the truck moves and can fail at the line
 * boundaries when velocity is too high.  The policy stops after each one-cell
 * move, reverses from rest for missed packages, and parks only when there is
 * nothing left to recover.
 */

ActionLabel_type TwoWayLinePark::evaluate(const StateBase& state_values) const {
    // Derive road length from the truck index: optional ice indicators may
    // follow the usual state variables. State layout (automaton at index 0):
    //   index 0                       : automaton state
    //   index 1 .. number_of_locations: package count at each location
    //   index number_of_locations + 1 : truck position
    //   index number_of_locations + 2 : truck load
    //   index number_of_locations + 3: truck_velocity
    const int number_of_locations = line_location_count(*model);
    const int truck_position = state_values.get_int(number_of_locations + 1);
    const int truck_load = state_values.get_int(number_of_locations + 2);
    const int truck_velocity = state_values.get_int(number_of_locations + 3);
    const int goal_position = number_of_locations - 1;

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
        if (truck_velocity == 0 and truck_load > 0) {
            return ACTION_DROP;
        }
        if (truck_velocity == 0 and package_behind) {
            return ACTION_DECELERATE;
        }
        if (truck_velocity == 0 and truck_load == 0) {
            return park_or_stall(*model);
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
    if (truck_velocity > distance_to_right_edge or -truck_velocity > distance_to_left_edge) {
        return ACTION_DROP;
    }

    // Safety-first: brake down unexpected high velocity while there is still room
    // and stop after each one-cell move, so dropped packages are recoverable
    // before the truck drives past them.
    if (truck_velocity > 0) {
        return ACTION_DECELERATE;
    }
    if (truck_velocity < 0) {
        return ACTION_ACCELERATE;
    }

    // Velocity 0 from here on.
    if (packages_here > 0) {
        return ACTION_PICKUP;
    }

    // Go back for missing packages only from rest.
    if (package_behind) {
        return ACTION_DECELERATE;
    }

    // Otherwise make one forward step toward the goal.
    return ACTION_ACCELERATE;
}


std::vector<double> TwoWayLinePark::action_values(const StateBase& state_values) const {
    const int number_of_locations = line_location_count(*model);
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

    std::vector<double> scores(interface->get_num_output_features(), BAD_ACTION);
    const bool at_goal = truck_position == goal_position;
    const bool stopped = truck_velocity == 0;
    const bool motion_unsafe = truck_velocity > distance_to_right_edge or -truck_velocity > distance_to_left_edge;

    if (not at_goal and stopped and packages_here > 0) {
        set_score(*interface, scores, ACTION_PICKUP, BEST_ACTION);
    }

    if (stopped and truck_load > 0) {
        set_score(*interface, scores, ACTION_DROP, at_goal ? BEST_ACTION : NEUTRAL_ACTION);
    }

    // evaluate stalls with DROP when moving forward at the goal or too fast to stop before an edge.
    if ((at_goal and truck_velocity > 0) or (not at_goal and motion_unsafe)) {
        set_score(*interface, scores, ACTION_DROP, BEST_ACTION);
    }

    if (at_goal and stopped and truck_load == 0 and not package_behind) {
        set_score(*interface, scores, park_or_stall(*model), BEST_ACTION);
    }

    if (not motion_unsafe) {
        if (truck_velocity > 0) {
            set_score(*interface, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*interface, scores, ACTION_MOVE, NEUTRAL_ACTION);
            set_score(*interface, scores, ACTION_ACCELERATE, truck_velocity > 1 ? BAD_ACTION : NEUTRAL_ACTION);
        } else if (truck_velocity < 0) {
            set_score(*interface, scores, ACTION_ACCELERATE, BEST_ACTION);
            set_score(*interface, scores, ACTION_MOVE, NEUTRAL_ACTION);
            set_score(*interface, scores, ACTION_DECELERATE, truck_velocity < -1 ? BAD_ACTION : NEUTRAL_ACTION);
        } else if (at_goal and package_behind) {
            set_score(*interface, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*interface, scores, ACTION_MOVE, NEUTRAL_ACTION);
        } else if (not at_goal and packages_here <= 0) {
            if (package_behind) {
                set_score(*interface, scores, ACTION_DECELERATE, BEST_ACTION);
                set_score(*interface, scores, ACTION_ACCELERATE, NEUTRAL_ACTION);
            } else {
                set_score(*interface, scores, ACTION_ACCELERATE, BEST_ACTION);
                set_score(*interface, scores, ACTION_DECELERATE, truck_position > 0 ? NEUTRAL_ACTION : BAD_ACTION);
            }
            set_score(*interface, scores, ACTION_MOVE, NEUTRAL_ACTION);
        }
    }

    return scores;
}
