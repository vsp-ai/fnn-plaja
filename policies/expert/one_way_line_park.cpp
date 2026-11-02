#include "policies/expert/expert_policy.h"

using namespace LINE_PARK;

/* Safety-first policy for OneWayLine with Parking.  The nondeterministic
 * variants can drop packages while the truck moves, so this policy stops after
 * each one-cell movement instead of coasting.  If the truck reaches the goal
 * empty after packages were left behind, parking is used as a safe terminal
 * fallback because one-way recovery is no longer possible.
 */

ActionLabel_type OneWayLinePark::evaluate(const StateBase& state_values) const {
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

    // At the goal, dropping and then parking are the only useful safe actions.
    if (truck_position == goal_position) {
        if (truck_velocity == 0 and truck_load == 0) {
            return park_or_stall(*model);
        }
        return ACTION_DROP;
    }

    // If the state is already too fast to stop before the end, any motion action
    // is unsafe in the OWL models.  Stall rather than force aux_vel = -1.
    const int remaining_distance = goal_position - truck_position;
    if (truck_velocity > remaining_distance) {
        return ACTION_DROP;
    }

    // Safety-first: brake down unexpected high velocity while there is still room
    // and stop after each one-cell move.  This makes dropped packages recoverable
    // and keeps the truck from reaching the end with positive velocity.
    if (truck_velocity > 0) {
        return ACTION_DECELERATE;
    }

    // Velocity 0 from here on.

    // Stopped on top of a package -> pick it up.
    if (packages_here > 0) {
        return ACTION_PICKUP;
    }

    // Move one cell at a time: accelerate from rest, then decelerate next step.
    return ACTION_ACCELERATE;
}


std::vector<double> OneWayLinePark::action_values(const StateBase& state_values) const {
    const int number_of_locations = line_location_count(*model);
    const int truck_position = state_values.get_int(number_of_locations + 1);
    const int truck_load = state_values.get_int(number_of_locations + 2);
    const int truck_velocity = state_values.get_int(number_of_locations + 3);
    const int goal_position = number_of_locations - 1;
    const int packages_here = state_values.get_int(truck_position + 1);
    const int remaining_distance = goal_position - truck_position;

    std::vector<double> scores(interface->get_num_output_features(), BAD_ACTION);
    const bool at_goal = truck_position == goal_position;
    const bool stopped = truck_velocity == 0;
    const bool motion_unsafe = at_goal or truck_velocity > remaining_distance;

    if (not at_goal and stopped and packages_here > 0) {
        set_score(*interface, scores, ACTION_PICKUP, BEST_ACTION);
    }

    if (at_goal and stopped and truck_load > 0) {
        set_score(*interface, scores, ACTION_DROP, BEST_ACTION);
    }

    // evaluate stalls with DROP when moving at the goal or too fast to stop.
    if ((at_goal and truck_velocity > 0) or (not at_goal and motion_unsafe)) {
        set_score(*interface, scores, ACTION_DROP, BEST_ACTION);
    }

    if (at_goal and stopped and truck_load == 0) {
        set_score(*interface, scores, park_or_stall(*model), BEST_ACTION);
    }

    if (not motion_unsafe) {
        if (truck_velocity > 0) {
            set_score(*interface, scores, ACTION_DECELERATE, BEST_ACTION);
            set_score(*interface, scores, ACTION_MOVE, BAD_ACTION);
            set_score(*interface, scores, ACTION_ACCELERATE, BAD_ACTION);
        } else if (packages_here <= 0) {
            set_score(*interface, scores, ACTION_ACCELERATE, BEST_ACTION);
            set_score(*interface, scores, ACTION_MOVE, BAD_ACTION);
            set_score(*interface, scores, ACTION_DECELERATE, BAD_ACTION);
        }
    }

    return scores;
}
