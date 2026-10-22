#ifndef PLAJA_FNN_IN_Z3_H
#define PLAJA_FNN_IN_Z3_H

#include <vector>
#include <z3++.h>
#include "../../../parser/nn_parser/using_nn.h"
#include "../../information/jani2fnn/jani_2_fnn.h"
#include "../forward_smt_z3.h"

namespace Z3_IN_PLAJA { class Context; class SMTSolver; class StateVarsInZ3; }

class FNNInZ3 {

private:
    const Jani2FNN* fnnInterface;
    Z3_IN_PLAJA::Context* context;

    std::vector<z3::expr> nodeValues;
    std::vector<z3::expr> nodeConstraints;
    std::vector<z3::expr> outputValues;

public:
    FNNInZ3(const Jani2FNN& fnn_interface, Z3_IN_PLAJA::Context& c, const Z3_IN_PLAJA::StateVarsInZ3& state_vars);
    ~FNNInZ3();
    DELETE_CONSTRUCTOR(FNNInZ3)

    [[nodiscard]] inline std::size_t get_output_layer_size() const { return outputValues.size(); }

    [[nodiscard]] inline const z3::expr& get_output_var_z3(OutputIndex_type output_index) const {
        PLAJA_ASSERT(output_index < outputValues.size())
        return outputValues[output_index];
    }

    void add(Z3_IN_PLAJA::SMTSolver& solver) const;

};

#endif // PLAJA_FNN_IN_Z3_H
