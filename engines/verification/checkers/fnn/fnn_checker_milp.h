#ifndef PLAJA_FNN_CHECKER_MILP_H
#define PLAJA_FNN_CHECKER_MILP_H

#include "engines/verification/checkers/fnn/fnn_checker_z3.h"
#include "gurobi_c++.h"

/** FNN policy checking with a Gurobi MILP over-approximation and exact witness validation. */
class FnnCheckerMilp final: public FnnCheckerZ3 {
private:
    GRBEnv environment;

    [[nodiscard]] bool check_milp();
    bool check_() override;

public:
    FnnCheckerMilp(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);
    ~FnnCheckerMilp() override = default;
    DELETE_CONSTRUCTOR(FnnCheckerMilp)

    bool check_relaxed() override;
};

#endif
