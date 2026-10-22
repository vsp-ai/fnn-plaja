#include "fnn_sat_checker_milp.h"

#include <cmath>
#include <stdexcept>
#include <vector>

#include "milp/z3_query_to_gurobi.h"
#include "../../fd_adaptions/timer.h"
#include "../../information/model_information.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../../../stats/stats_base.h"
#include "../smt/model_z3_pa.h"

namespace {

    void collect_state_values(
        const ModelZ3PA& model,
        FNN_MILP::Z3QueryToGurobi& milp,
        StepIndex_type step,
        std::vector<FNN_MILP::Z3QueryToGurobi::StateValue>& values
    ) {
        const auto& state_vars = model.get_state_vars(step);
        const auto& model_info = model.get_model_info();
        for (auto it = model_info.stateIndexIterator(); !it.end(); ++it) {
            const auto state_index = it.state_index();
            if (not state_vars.exists_index(state_index)) { continue; }
            const auto expression = state_vars.to_z3_expr_index(state_index);
            if (milp.contains(expression)) { values.push_back(milp.state_value(expression)); }
        }
    }

    z3::expr encode_assignment(
        z3::context& context,
        const std::vector<FNN_MILP::Z3QueryToGurobi::StateValue>& values
    ) {
        auto assignment = context.bool_val(true);
        for (const auto& entry: values) {
            assignment = assignment && (
                entry.isBool
                    ? entry.expression == context.bool_val(entry.value != 0)
                    : entry.expression == context.int_val(static_cast<std::int64_t>(entry.value))
            );
        }
        return assignment;
    }

}

FnnSatCheckerMilp::FnnSatCheckerMilp(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config):
    FnnSatCheckerZ3(jani_2_fnn, config)
    , environment(true) {
    environment.set(GRB_IntParam_OutputFlag, 0);
    environment.set(GRB_IntParam_LogToConsole, 0);
    try {
        environment.start();
    } catch (const GRBException& error) {
        throw std::runtime_error(
            "Could not start Gurobi for FNN MILP verification (code "
            + std::to_string(error.getErrorCode()) + "): " + error.getMessage()
        );
    }
    PLAJA_LOG("Constructed FNN MILP checker (Gurobi with exact Z3 witness validation).")
}

bool FnnSatCheckerMilp::check_milp() {
    auto milp = FNN_MILP::Z3QueryToGurobi(environment, solver->collect_assertions());

    while (true) {
        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MILP_QUERIES); }
        const auto start = (*PLAJA_GLOBAL::timer)();
        const auto status = milp.solve();
        if (sharedStatistics) { sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_MILP, (*PLAJA_GLOBAL::timer)() - start); }

        if (status == FNN_MILP::Z3QueryToGurobi::Status::Infeasible) {
            if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MILP_QUERIES_UNSAT); }
            save_solution_if(false);
            solver->reset();
            return false;
        }

        std::vector<FNN_MILP::Z3QueryToGurobi::StateValue> assignment;
        collect_state_values(*modelZ3, milp, 0, assignment);
        if (set_target_state) { collect_state_values(*modelZ3, milp, 1, assignment); }
        if (assignment.empty()) { throw std::runtime_error("FNN MILP produced no finite-range state assignment to validate"); }

        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MILP_WITNESS_CHECKS); }
        solver->push();
        solver->add(encode_assignment(solver->get_context_z3(), assignment));
        const bool exact = solver->check();
        if (exact) { save_solution_if(true); }
        solver->pop();

        if (exact) {
            solver->reset();
            return true;
        }
        milp.block(assignment);
    }
}

bool FnnSatCheckerMilp::check_() { return check_milp(); }

bool FnnSatCheckerMilp::check_relaxed() { return check_milp(); }
