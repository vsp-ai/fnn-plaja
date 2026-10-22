//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "solution_enumerator_z3.h"
#include "../../states/state_values.h"
#include "../../../utils/utils.h"
#include "../model/model_z3.h"
#include "../solver/smt_solver_z3.h"
#include "../solver/solution_z3.h"
#include "../vars_in_z3.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace {

std::string format_elapsed(double elapsed) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << elapsed;
    return out.str();
}

}

SolutionEnumeratorZ3::SolutionEnumeratorZ3(Z3_IN_PLAJA::SMTSolver& solver_ref, const ModelZ3& model_z3):
    SolutionEnumeratorBase(model_z3.get_model_info())
    , solver(&solver_ref)
    , modelZ3(&model_z3)
    , stateIndexes(model_z3.get_src_vars().cache_state_indexes(false))
    , stateVars(nullptr) { // only for customized state indexes
}

SolutionEnumeratorZ3::~SolutionEnumeratorZ3() = default;

/* */

/* */

void SolutionEnumeratorZ3::initialize() { set_initialized(); }

bool SolutionEnumeratorZ3::retrieve_solution(StateValues& solution_state) {
    if (not solver->check()) { return false; } // Problem is unsat.
    auto solution = solver->get_solution();
    /* New solution found, extract and constraint for next solution ... */
    stateIndexes->extract_solution(*solution, solution_state);
    solver->add(!stateIndexes->encode_solution(*solution));
    return true;
}

std::unique_ptr<StateValues> SolutionEnumeratorZ3::retrieve_solution() {
    if (not solver->check()) { return nullptr; } // Problem is unsat.
    auto solution = solver->get_solution();
    /* New solution found, extract and constraint for next solution ... */
    auto solution_state = std::make_unique<StateValues>(get_constructor());
    stateIndexes->extract_solution(*solution, *solution_state);
    solver->add(!stateIndexes->encode_solution(*solution));
    return solution_state;
}

std::list<StateValues> SolutionEnumeratorZ3::enumerate_solutions() {
    PLAJA_LOG("Z3 initial-state enumeration progress: starting exhaustive start-state enumeration.")
    const auto progress_start = std::chrono::steady_clock::now();
    auto next_progress_log = progress_start + std::chrono::seconds(10);

    std::list<StateValues> solutions;
    solutions.push_back(get_constructor());
    while (retrieve_solution(solutions.back())) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= next_progress_log) {
            const auto elapsed = std::chrono::duration<double>(now - progress_start).count();
            PLAJA_LOG(
                "Z3 initial-state enumeration progress: found "
                + std::to_string(solutions.size())
                + " start states, elapsed "
                + format_elapsed(elapsed)
                + "s."
            )
            next_progress_log = now + std::chrono::seconds(10);
        }
        solutions.push_back(get_constructor());
    }
    solutions.pop_back(); // Pop last state which does not correspond to a new state.
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - progress_start).count();
    PLAJA_LOG(
        "Z3 initial-state enumeration progress: completed with "
        + std::to_string(solutions.size())
        + " start states, elapsed "
        + format_elapsed(elapsed)
        + "s."
    )
    return solutions;
}

/* */

std::list<StateValues> SolutionEnumeratorZ3::compute_solutions(Z3_IN_PLAJA::SMTSolver& solver, const ModelZ3& model_z3, StateValues&& constructor) {
    SolutionEnumeratorZ3 solution_enumerator(solver, model_z3);
    solution_enumerator.set_constructor(std::make_unique<StateValues>(std::move(constructor)));
    return solution_enumerator.enumerate_solutions();
}
