//
// This file is part of the PlaJA code base (2019 - 2020).
//

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "Simplify"

#include <memory>
#include "sat_checker.h"
#include "../../include/compiler_config_const.h"
#include "../../globals.h"
#include "../factories/configuration.h"
#include "../information/jani_2_interface.h"
#include "../states/state_values.h"
#include "pa_states/abstract_state.h"
#include "solution_checker_pa.h"
#include "solution_checker_instance_pa.h"

/**********************************************************************************************************************/

SatChecker::SatChecker(const Jani2Interface& jani_2_interface, const PLAJA::Configuration& config)
    :
    jani2Interface(&jani_2_interface)
    , solutionChecker(nullptr)
    , sharedStatistics(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , set_action_label(ACTION::noneLabel)
    , set_output_index(NEURON_INDEX::none)
    , set_action_op_id(ACTION::noneOp)
    , set_update_index(ACTION::noneUpdate)
    , set_source_state(nullptr)
    , set_target_state(nullptr)
    , lastSolution(nullptr)
{
    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Sat-checker without stats."))
}

SatChecker::~SatChecker() = default;


void SatChecker::set_label(ActionLabel_type action_label) {
    PLAJA_ASSERT(is_action_selectable(action_label))
    set_action_label = action_label;
    set_output_index = jani2Interface->get_output_index(action_label);
}

void SatChecker::unset_label() {
    set_action_label = ACTION::noneLabel;
    set_output_index = NEURON_INDEX::none;
}

void SatChecker::unset_operator() {
    set_action_op_id = ACTION::noneOp;
}

void SatChecker::set_update(UpdateIndex_type update_index) {
    set_update_index = update_index;
}

void SatChecker::unset_update() {
    set_update_index = ACTION::noneUpdate;
}

void SatChecker::unset_source_state() { set_source_state = nullptr; }

void SatChecker::unset_target_state() { set_target_state = nullptr; }

bool SatChecker::is_action_selectable(ActionLabel_type action_label) const { return jani2Interface->is_learned(action_label); }

const StateValues& SatChecker::get_last_solution() const {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return *lastSolution->get_solution(0);
}

std::unique_ptr<StateValues> SatChecker::release_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(0))
    return lastSolution->retrieve_solution(0);
}

const StateValues* SatChecker::get_target_solution() const {
    PLAJA_ASSERT(has_solution())
    if (lastSolution->get_solution_size() > 1) {
        PLAJA_ASSERT(lastSolution->get_solution_size() == 2)
        return lastSolution->get_solution(1);
    } else {
        return nullptr;
    }
}

std::unique_ptr<StateValues> SatChecker::release_target_solution() {
    PLAJA_ASSERT(has_solution() and lastSolution->get_solution(1))
    return lastSolution->retrieve_solution(1);
}

/***check version*******************************************************************************************************/


bool SatChecker::check() {

    // unset exact solution if set:
    if constexpr (PLAJA_GLOBAL::reuseSolutions) { lastSolution = nullptr; }

    const bool rlt = check_();

    return rlt;
}

/***********************************************************************************************************************/

void SatChecker::print_statistics() {
}

#pragma GCC diagnostic pop
