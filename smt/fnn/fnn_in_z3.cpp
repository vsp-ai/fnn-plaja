#include "smt/fnn/fnn_in_z3.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "policies/fnn/formularized_network.h"
#include "core/utils/floating_utils.h"
#include "policies/fnn/jani_2_fnn.h"
#include "smt/context_z3.h"
#include "smt/solver/smt_solver_z3.h"
#include "smt/vars_in_z3.h"
#include "smt/using_smt.h"
#include "smt/visitor/extern/to_z3_visitor.h"

namespace {

z3::expr to_real(Z3_IN_PLAJA::Context& c, const z3::expr& e) {
    if (e.is_real()) { return e; }
    if (e.is_int()) { return z3::to_real(e); }
    if (e.is_bool()) { return z3::to_real(Z3_IN_PLAJA::bool_to_int(e)); }
    PLAJA_ASSERT(false)
    return c.real_val(0.0, Z3_IN_PLAJA::floatingPrecision);
}

z3::expr to_int(Z3_IN_PLAJA::Context& c, const z3::expr& e) {
    if (e.is_int()) { return e; }
    if (e.is_bool()) { return Z3_IN_PLAJA::bool_to_int(e); }
    PLAJA_ASSERT(false)
    return c.int_val(0);
}

} // namespace

FNNInZ3::FNNInZ3(const Jani2FNN& fnn_interface, Z3_IN_PLAJA::Context& c, const Z3_IN_PLAJA::StateVarsInZ3& state_vars)
    : fnnInterface(&fnn_interface)
    , context(&c) {

    const auto& fnn = fnnInterface->load_fnn();
    const auto& nodes = fnn.get_nodes();
    const auto init_logical_idx = fnn.get_init_logical_idx();
    const auto init_linear_idx = fnn.get_init_linear_idx();
    const auto linear_size = fnn.get_linear_size();
    const auto& linear_to_output = fnn.get_linear_to_output();

    PLAJA_ASSERT(init_logical_idx >= 0)
    PLAJA_ASSERT(init_linear_idx >= init_logical_idx)
    PLAJA_ASSERT(linear_to_output.size() == linear_size)

    nodeValues.reserve(nodes.size());
    nodeConstraints.reserve(nodes.size());
    outputValues.assign(linear_size, c.real_val(0.0, Z3_IN_PLAJA::floatingPrecision));

    const auto add_node = [&c, this](const z3::expr& expr, const char* prefix) -> z3::expr {
        Z3_IN_PLAJA::VarId_type var_id = 0;
        if (expr.is_bool()) { var_id = c.add_bool_var(prefix); }
        else if (expr.is_int()) { var_id = c.add_int_var(prefix); }
        else { var_id = c.add_real_var(prefix); }
        const auto& var_expr = c.get_var(var_id);
        nodeValues.push_back(var_expr);
        nodeConstraints.push_back(var_expr == expr);
        return var_expr;
    };

    {
        const auto& bins = fnn.get_bin_features();
        PLAJA_ASSERT(bins.size() == static_cast<std::size_t>(init_logical_idx))
        for (int i = 0; i < init_logical_idx; ++i) {
            const auto& bin = bins[i];
            bool use_real = false;
            if (!PLAJA_FLOATS::is_integer(bin.constant, PLAJA::floatingPrecision)) {
                use_real = true;
            }
            for (const auto& term : bin.terms) {
                const auto state_expr = state_vars.to_z3_expr_index(term.stateIndex);
                if (state_expr.is_real() || !PLAJA_FLOATS::is_integer(term.coeff, PLAJA::floatingPrecision)) {
                    use_real = true;
                    break;
                }
            }

            // Integer coefficients and constants (all benchmark predicates) are exact; others enter as decimals with 17 places.
            z3::expr sum = use_real ? c.real_val(0.0, Z3_IN_PLAJA::floatingPrecision) : c.int_val(0);
            for (const auto& term : bin.terms) {
                const auto state_expr = state_vars.to_z3_expr_index(term.stateIndex);
                if (use_real) {
                    const auto coeff = c.real_val(static_cast<PLAJA::floating>(term.coeff), Z3_IN_PLAJA::floatingPrecision);
                    sum = sum + coeff * to_real(c, state_expr);
                } else {
                    const auto coeff_int = c.int_val(static_cast<PLAJA::integer>(term.coeff));
                    sum = sum + coeff_int * to_int(c, state_expr);
                }
            }
            const auto constant = use_real
                                      ? c.real_val(static_cast<PLAJA::floating>(bin.constant), Z3_IN_PLAJA::floatingPrecision)
                                      : c.int_val(static_cast<PLAJA::integer>(bin.constant));
            z3::expr cond(c());
            switch (bin.op) {
                case PLAJA::PredicateOp::EQ:  cond = (sum == constant); break;
                case PLAJA::PredicateOp::NEQ: cond = (sum != constant); break;
                case PLAJA::PredicateOp::LT:  cond = (sum <  constant); break;
                case PLAJA::PredicateOp::LEQ: cond = (sum <= constant); break;
                case PLAJA::PredicateOp::GT:  cond = (sum >  constant); break;
                case PLAJA::PredicateOp::GEQ: cond = (sum >= constant); break;
                default: PLAJA_ASSERT(false)
            }
            add_node(cond, "fnn_bin");
        }
    }

    for (int i = init_logical_idx; i < static_cast<int>(nodes.size()); ++i) {
        const auto& node = nodes[i];
        switch (node.type) {
            case PLAJA::NodeType::AND: {
                std::vector<z3::expr> conjuncts;
                conjuncts.reserve(node.preds.size());
                for (const int p : node.preds) {
                    PLAJA_ASSERT(p >= 0 && static_cast<std::size_t>(p) < nodeValues.size())
                    conjuncts.push_back(nodeValues[p]);
                }
                const auto cond = conjuncts.empty() ? c.bool_val(true) : Z3_IN_PLAJA::to_conjunction(conjuncts);
                add_node(cond, "fnn_and");
                break;
            }
            case PLAJA::NodeType::OR: {
                std::vector<z3::expr> disjuncts;
                disjuncts.reserve(node.preds.size());
                for (const int p : node.preds) {
                    PLAJA_ASSERT(p >= 0 && static_cast<std::size_t>(p) < nodeValues.size())
                    disjuncts.push_back(nodeValues[p]);
                }
                const auto cond = disjuncts.empty() ? c.bool_val(false) : Z3_IN_PLAJA::to_disjunction(disjuncts);
                add_node(cond, "fnn_or");
                break;
            }
            case PLAJA::NodeType::NOT: {
                PLAJA_ASSERT(node.preds.size() == 1)
                const int p = node.preds[0];
                PLAJA_ASSERT(p >= 0 && static_cast<std::size_t>(p) < nodeValues.size())
                add_node(!nodeValues[p], "fnn_not");
                break;
            }
            case PLAJA::NodeType::LINEAR: {
                // Weights and bias enter as decimals with 17 places (error below 1e-17 each), which the argmax
                // tolerance of OutputConstraints::compute_eq covers.
                z3::expr acc = c.real_val(static_cast<PLAJA::floating>(node.bias), Z3_IN_PLAJA::floatingPrecision);
                for (std::size_t k = 0; k < node.preds.size(); ++k) {
                    const int p = node.preds[k];
                    PLAJA_ASSERT(p >= 0 && static_cast<std::size_t>(p) < nodeValues.size())
                    const auto weight = c.real_val(static_cast<PLAJA::floating>(node.weights[k]), Z3_IN_PLAJA::floatingPrecision);
                    acc = acc + weight * to_real(c, nodeValues[p]);
                }
                const auto node_var = add_node(acc, "fnn_linear");
                const int linear_idx = i - init_linear_idx;
                PLAJA_ASSERT(linear_idx >= 0 && static_cast<std::size_t>(linear_idx) < linear_to_output.size())
                const int out_idx = linear_to_output[linear_idx];
                PLAJA_ASSERT(out_idx >= 0 && static_cast<std::size_t>(out_idx) < outputValues.size())
                outputValues[static_cast<std::size_t>(out_idx)] = node_var;
                break;
            }
            default:
                PLAJA_ASSERT(false)
        }
    }
}

FNNInZ3::~FNNInZ3() = default;

void FNNInZ3::add(Z3_IN_PLAJA::SMTSolver& solver) const {
    for (const auto& constraint : nodeConstraints) { solver.add(constraint); }
}
