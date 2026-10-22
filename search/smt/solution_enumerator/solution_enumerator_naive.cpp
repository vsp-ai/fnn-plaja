//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "solution_enumerator_naive.h"
#include "../../../exception/not_supported_exception.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../states/state_values.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace {

std::string format_count(long double value) {
    if (value < 1000000.0L) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(0) << value;
        return out.str();
    }

    std::ostringstream out;
    out << std::scientific << std::setprecision(3) << value;
    return out.str();
}

}

SolutionEnumeratorNaive::SolutionEnumeratorNaive(const ModelInformation& model_info, const Expression& condition_ref):
    SolutionEnumeratorBase(model_info)
    , condition(&condition_ref)
    , candidateStatesTotal(0)
    , candidateStatesChecked(0)
    , candidatesSinceProgressCheck(0)
    , progressStartTime()
    , progressNextLogTime() {

    static bool warning_printed(false);
    if (not warning_printed) {
        PLAJA_LOG(PLAJA_UTILS::to_red_string("Warning: Naive solution enumeration."))
        PLAJA_LOG(PLAJA_UTILS::to_red_string("Consider building SMT-Module (dependency: Z3) to enable scalable enumeration methods."))
        warning_printed = true;
    }

    if (model_info.get_floating_state_size() > 0) { throw NotSupportedException("Naive solution enumeration cannot handle continuous state spaces."); }

}

SolutionEnumeratorNaive::~SolutionEnumeratorNaive() = default;

/* */

void SolutionEnumeratorNaive::reset_progress() {
    candidateStatesTotal = 1;
    candidateStatesChecked = 0;
    candidatesSinceProgressCheck = 0;

    for (VariableIndex_type state_index = 0; state_index < get_model_info().get_int_state_size(); ++state_index) {
        const auto lower_bound = get_model_info().get_lower_bound_int(state_index);
        const auto upper_bound = get_model_info().get_upper_bound_int(state_index);
        candidateStatesTotal *= static_cast<long double>(upper_bound - lower_bound + 1);
    }

    progressStartTime = std::chrono::steady_clock::now();
    progressNextLogTime = progressStartTime + std::chrono::seconds(10);

    PLAJA_LOG("Naive initial-state enumeration progress: scanning up to " + format_count(candidateStatesTotal) + " candidate assignments.")
}

void SolutionEnumeratorNaive::report_progress(bool force) {
    const auto now = std::chrono::steady_clock::now();
    if (not force and now < progressNextLogTime) { return; }

    const auto elapsed = std::chrono::duration<double>(now - progressStartTime).count();
    const auto rate = elapsed > 0 ? candidateStatesChecked / elapsed : 0.0L;
    const auto percent = candidateStatesTotal > 0 ? 100.0L * candidateStatesChecked / candidateStatesTotal : 0.0L;

    std::ostringstream message;
    message << "Naive initial-state enumeration progress: checked "
            << format_count(candidateStatesChecked) << " / " << format_count(candidateStatesTotal)
            << " candidates (" << std::fixed << std::setprecision(6) << static_cast<double>(percent) << "%), found "
            << solutions.size() << " start states, elapsed " << std::setprecision(1) << elapsed << "s, rate "
            << format_count(rate) << " candidates/s.";
    PLAJA_LOG(message.str())

    progressNextLogTime = now + std::chrono::seconds(10);
}

/* */

void SolutionEnumeratorNaive::compute_solutions_rec(VariableIndex_type state_index, StateValues& state_values) { // NOLINT(misc-no-recursion)

    if (state_index == get_model_info().get_int_state_size()) {

        /* Sanity: floating not supported. */
        PLAJA_ASSERT(get_model_info().get_int_state_size() == get_model_info().get_state_size())
        PLAJA_ASSERT(get_model_info().get_floating_state_size() == 0)

        ++candidateStatesChecked;
        ++candidatesSinceProgressCheck;
        if (condition->evaluate_integer(state_values)) { solutions.push_back(state_values); }
        if (candidatesSinceProgressCheck >= 10000) {
            candidatesSinceProgressCheck = 0;
            report_progress(false);
        }

    } else {

        const auto lower_bound = get_model_info().get_lower_bound_int(state_index);
        const auto upper_bound = get_model_info().get_upper_bound_int(state_index);
        auto next_state_index = state_index + 1;

        for (auto value = lower_bound; value <= upper_bound; ++value) {
            state_values.assign_int<false>(state_index, value);
            compute_solutions_rec(next_state_index, state_values);
        }

    }
}

/* */

void SolutionEnumeratorNaive::initialize() {
    solutions.clear();
    reset_progress();
    StateValues state_values(get_model_info().get_int_state_size(), 0);
    compute_solutions_rec(0, state_values);
    report_progress(true);
    set_initialized();
}

/* */

std::list<StateValues> SolutionEnumeratorNaive::compute_solutions(const Expression& condition, const ModelInformation& model_info) {
    SolutionEnumeratorNaive enumerator(model_info, condition);
    return enumerator.enumerate_solutions();
}
