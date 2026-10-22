#ifndef PLAJA_FNN_SAT_CHECKER_MILP_H
#define PLAJA_FNN_SAT_CHECKER_MILP_H

#include "fnn_sat_checker_z3.h"
#include "gurobi_c++.h"

/** FNN-SAT checking with a Gurobi MILP over-approximation and exact witness validation. */
class FnnSatCheckerMilp final: public FnnSatCheckerZ3 {
private:
    GRBEnv environment;

    [[nodiscard]] bool check_milp();
    bool check_() override;

public:
    FnnSatCheckerMilp(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);
    ~FnnSatCheckerMilp() override = default;
    DELETE_CONSTRUCTOR(FnnSatCheckerMilp)

    bool check_relaxed() override;
};

#endif
