#ifndef PLAJA_FNN_Z3_QUERY_TO_GUROBI_H
#define PLAJA_FNN_Z3_QUERY_TO_GUROBI_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <z3++.h>
#include "gurobi_c++.h"

namespace FNN_MILP {

    class Z3QueryToGurobi final {
    public:
        enum class Status { Feasible, Infeasible };

        struct StateValue {
            z3::expr expression;
            GRBVar variable;
            bool isBool;
            std::int64_t value;

            StateValue(const z3::expr& expression_, GRBVar variable_, bool is_bool, std::int64_t value_):
                expression(expression_), variable(variable_), isBool(is_bool), value(value_) {}
        };

    private:
        GRBModel model;
        std::unordered_map<unsigned, GRBVar> variables;
        unsigned auxiliaryCounter;

        [[nodiscard]] std::string auxiliary_name(const std::string& prefix);
        [[nodiscard]] GRBVar variable(const z3::expr& expression);
        [[nodiscard]] GRBVar boolean(const z3::expr& expression);
        [[nodiscard]] GRBLinExpr affine(const z3::expr& expression);

        void add_boolean_equality(GRBVar result, GRBVar lhs, GRBVar rhs);
        void add_integral_disequality(const GRBLinExpr& active, const GRBLinExpr& difference);
        void add_numeric_atom(GRBVar result, const z3::expr& expression);
        void assert_formula(const z3::expr& expression);

    public:
        Z3QueryToGurobi(GRBEnv& environment, const z3::expr_vector& assertions);
        ~Z3QueryToGurobi() = default;

        Z3QueryToGurobi(const Z3QueryToGurobi&) = delete;
        Z3QueryToGurobi& operator=(const Z3QueryToGurobi&) = delete;

        [[nodiscard]] Status solve();
        [[nodiscard]] bool contains(const z3::expr& expression) const;
        [[nodiscard]] StateValue state_value(const z3::expr& expression) const;
        void block(const std::vector<StateValue>& assignment);
    };

}

#endif
