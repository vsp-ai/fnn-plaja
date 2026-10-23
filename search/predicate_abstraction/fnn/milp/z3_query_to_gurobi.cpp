#include "z3_query_to_gurobi.h"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include "../../../fd_adaptions/search_engine.h"

namespace {

    [[nodiscard]] std::runtime_error unsupported(const z3::expr& expression) {
        std::ostringstream message;
        message << "Unsupported expression in FNN MILP query: " << expression;
        return std::runtime_error(message.str());
    }

    [[nodiscard]] bool is_uninterpreted_constant(const z3::expr& expression) {
        return expression.num_args() == 0 and expression.decl().decl_kind() == Z3_OP_UNINTERPRETED;
    }

}

std::string FNN_MILP::Z3QueryToGurobi::auxiliary_name(const std::string& prefix) {
    return prefix + "_" + std::to_string(auxiliaryCounter++);
}

GRBVar FNN_MILP::Z3QueryToGurobi::variable(const z3::expr& expression) {
    if (not is_uninterpreted_constant(expression) or expression.is_array()) { throw unsupported(expression); }

    const auto id = expression.id();
    const auto existing = variables.find(id);
    if (existing != variables.end()) { return existing->second; }

    char type;
    if (expression.is_bool()) { type = GRB_BINARY; }
    else if (expression.is_int()) { type = GRB_INTEGER; }
    else if (expression.is_real()) { type = GRB_CONTINUOUS; }
    else { throw unsupported(expression); }

    const auto lower = expression.is_bool() ? 0.0 : -GRB_INFINITY;
    const auto upper = expression.is_bool() ? 1.0 : GRB_INFINITY;
    auto result = model.addVar(lower, upper, 0.0, type, expression.decl().name().str());
    variables.emplace(id, result);
    return result;
}

void FNN_MILP::Z3QueryToGurobi::add_boolean_equality(GRBVar result, GRBVar lhs, GRBVar rhs) {
    model.addConstr(result <= 1 - lhs + rhs);
    model.addConstr(result <= 1 + lhs - rhs);
    model.addConstr(result >= 1 - lhs - rhs);
    model.addConstr(result >= lhs + rhs - 1);
}

void FNN_MILP::Z3QueryToGurobi::add_integral_disequality(const GRBLinExpr& active, const GRBLinExpr& difference) {
    auto below = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, auxiliary_name("int_neq_below"));
    auto above = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, auxiliary_name("int_neq_above"));
    model.addConstr(below + above == active);
    model.addGenConstrIndicator(below, 1, difference, GRB_LESS_EQUAL, -1.0);
    model.addGenConstrIndicator(above, 1, difference, GRB_GREATER_EQUAL, 1.0);
}

void FNN_MILP::Z3QueryToGurobi::add_numeric_atom(GRBVar result, const z3::expr& expression) {
    const auto kind = expression.decl().decl_kind();
    if (expression.num_args() != 2) { throw unsupported(expression); }

    const auto difference = affine(expression.arg(0)) - affine(expression.arg(1));
    const bool integral = expression.arg(0).is_int() and expression.arg(1).is_int();
    switch (kind) {
        case Z3_OP_LE:
            model.addGenConstrIndicator(result, 1, difference, GRB_LESS_EQUAL, 0.0);
            model.addGenConstrIndicator(result, 0, difference, GRB_GREATER_EQUAL, integral ? 1.0 : 0.0);
            return;
        case Z3_OP_LT:
            model.addGenConstrIndicator(result, 1, difference, GRB_LESS_EQUAL, integral ? -1.0 : 0.0);
            model.addGenConstrIndicator(result, 0, difference, GRB_GREATER_EQUAL, 0.0);
            return;
        case Z3_OP_GE:
            model.addGenConstrIndicator(result, 1, difference, GRB_GREATER_EQUAL, 0.0);
            model.addGenConstrIndicator(result, 0, difference, GRB_LESS_EQUAL, integral ? -1.0 : 0.0);
            return;
        case Z3_OP_GT:
            model.addGenConstrIndicator(result, 1, difference, GRB_GREATER_EQUAL, integral ? 1.0 : 0.0);
            model.addGenConstrIndicator(result, 0, difference, GRB_LESS_EQUAL, 0.0);
            return;
        case Z3_OP_EQ:
            model.addGenConstrIndicator(result, 1, difference, GRB_EQUAL, 0.0);
            if (integral) { add_integral_disequality(1 - result, difference); }
            return;
        case Z3_OP_DISTINCT:
            model.addGenConstrIndicator(result, 0, difference, GRB_EQUAL, 0.0);
            if (integral) { add_integral_disequality(result, difference); }
            return;
        default: throw unsupported(expression);
    }
}

GRBVar FNN_MILP::Z3QueryToGurobi::boolean(const z3::expr& expression) {
    if (not expression.is_bool()) { throw unsupported(expression); }
    if (expression.is_true() or expression.is_false()) {
        const auto id = expression.id();
        const auto existing = variables.find(id);
        if (existing != variables.end()) { return existing->second; }
        const auto value = expression.is_true() ? 1.0 : 0.0;
        auto result = model.addVar(value, value, 0.0, GRB_BINARY, auxiliary_name("bool_const"));
        variables.emplace(id, result);
        return result;
    }
    if (is_uninterpreted_constant(expression)) { return variable(expression); }

    const auto id = expression.id();
    const auto existing = variables.find(id);
    if (existing != variables.end()) { return existing->second; }

    auto result = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, auxiliary_name("bool"));
    variables.emplace(id, result);

    const auto kind = expression.decl().decl_kind();
    switch (kind) {
        case Z3_OP_NOT:
            model.addConstr(result + boolean(expression.arg(0)) == 1);
            break;
        case Z3_OP_AND: {
            GRBLinExpr sum = 0;
            for (unsigned i = 0; i < expression.num_args(); ++i) {
                const auto child = boolean(expression.arg(i));
                model.addConstr(result <= child);
                sum += child;
            }
            model.addConstr(result >= sum - static_cast<double>(expression.num_args()) + 1.0);
            break;
        }
        case Z3_OP_OR: {
            GRBLinExpr sum = 0;
            for (unsigned i = 0; i < expression.num_args(); ++i) {
                const auto child = boolean(expression.arg(i));
                model.addConstr(result >= child);
                sum += child;
            }
            model.addConstr(result <= sum);
            break;
        }
        case Z3_OP_IMPLIES: {
            const auto lhs = boolean(expression.arg(0));
            const auto rhs = boolean(expression.arg(1));
            model.addConstr(result >= 1 - lhs);
            model.addConstr(result >= rhs);
            model.addConstr(result <= 1 - lhs + rhs);
            break;
        }
        case Z3_OP_IFF:
            add_boolean_equality(result, boolean(expression.arg(0)), boolean(expression.arg(1)));
            break;
        case Z3_OP_EQ:
            if (expression.arg(0).is_bool() and expression.arg(1).is_bool()) {
                add_boolean_equality(result, boolean(expression.arg(0)), boolean(expression.arg(1)));
            } else { add_numeric_atom(result, expression); }
            break;
        case Z3_OP_DISTINCT:
        case Z3_OP_LE:
        case Z3_OP_LT:
        case Z3_OP_GE:
        case Z3_OP_GT:
            add_numeric_atom(result, expression);
            break;
        default: throw unsupported(expression);
    }
    return result;
}

GRBLinExpr FNN_MILP::Z3QueryToGurobi::affine(const z3::expr& expression) {
    if (expression.is_numeral()) { return GRBLinExpr(expression.as_double()); }
    if (is_uninterpreted_constant(expression)) {
        if (expression.is_bool() or expression.is_array()) { throw unsupported(expression); }
        return GRBLinExpr(variable(expression));
    }

    const auto kind = expression.decl().decl_kind();
    switch (kind) {
        case Z3_OP_TO_REAL: return affine(expression.arg(0));
        case Z3_OP_ADD: {
            GRBLinExpr result = 0;
            for (unsigned i = 0; i < expression.num_args(); ++i) { result += affine(expression.arg(i)); }
            return result;
        }
        case Z3_OP_SUB: {
            if (expression.num_args() == 0) { throw unsupported(expression); }
            auto result = affine(expression.arg(0));
            for (unsigned i = 1; i < expression.num_args(); ++i) { result -= affine(expression.arg(i)); }
            return result;
        }
        case Z3_OP_UMINUS: return -affine(expression.arg(0));
        case Z3_OP_MUL:
            if (expression.num_args() != 2) { throw unsupported(expression); }
            if (expression.arg(0).is_numeral()) { return expression.arg(0).as_double() * affine(expression.arg(1)); }
            if (expression.arg(1).is_numeral()) { return expression.arg(1).as_double() * affine(expression.arg(0)); }
            throw unsupported(expression);
        case Z3_OP_DIV: {
            if (expression.num_args() != 2 or not expression.arg(1).is_numeral()) { throw unsupported(expression); }
            const auto denominator = expression.arg(1).as_double();
            if (denominator == 0) { throw unsupported(expression); }
            return affine(expression.arg(0)) * (1.0 / denominator);
        }
        case Z3_OP_ITE: {
            if (expression.num_args() != 3 or not expression.arg(1).is_numeral() or not expression.arg(2).is_numeral()) { throw unsupported(expression); }
            const auto then_value = expression.arg(1).as_double();
            const auto else_value = expression.arg(2).as_double();
            return else_value + (then_value - else_value) * boolean(expression.arg(0));
        }
        default: throw unsupported(expression);
    }
}

void FNN_MILP::Z3QueryToGurobi::assert_formula(const z3::expr& expression) { model.addConstr(boolean(expression) == 1); }

FNN_MILP::Z3QueryToGurobi::Z3QueryToGurobi(GRBEnv& environment, const z3::expr_vector& assertions):
    model(environment)
    , variables()
    , auxiliaryCounter(0) {
    model.set(GRB_IntParam_OutputFlag, 0);
    model.set(GRB_IntParam_LogToConsole, 0);
    for (const auto& assertion: assertions) { assert_formula(assertion); }
    model.update();
}

FNN_MILP::Z3QueryToGurobi::Status FNN_MILP::Z3QueryToGurobi::solve() {
    try {
        const auto left = SearchEngine::time_left();
        if (left < GRB_INFINITY) { model.set(GRB_DoubleParam_TimeLimit, std::max(0., left)); }
        model.optimize();
        const auto status = model.get(GRB_IntAttr_Status);
        if (status == GRB_OPTIMAL) { return Status::Feasible; }
        if (status == GRB_INFEASIBLE) { return Status::Infeasible; }
        if (status == GRB_TIME_LIMIT) { throw SearchEngine::TIMEOUT; }
        throw std::runtime_error("Gurobi returned non-conclusive status " + std::to_string(status));
    } catch (const GRBException& error) {
        throw std::runtime_error(
            "Gurobi failed while solving the FNN MILP query (code "
            + std::to_string(error.getErrorCode()) + "): " + error.getMessage()
        );
    }
}

bool FNN_MILP::Z3QueryToGurobi::contains(const z3::expr& expression) const { return variables.count(expression.id()) > 0; }

FNN_MILP::Z3QueryToGurobi::StateValue FNN_MILP::Z3QueryToGurobi::state_value(const z3::expr& expression) const {
    const auto it = variables.find(expression.id());
    if (it == variables.end() or (not expression.is_bool() and not expression.is_int())) { throw unsupported(expression); }
    const auto raw_value = it->second.get(GRB_DoubleAttr_X);
    const auto rounded = static_cast<std::int64_t>(std::llround(raw_value));
    if (std::abs(raw_value - static_cast<double>(rounded)) > 1.0e-5) { throw std::runtime_error("Gurobi returned a non-integral state value"); }
    return { expression, it->second, expression.is_bool(), rounded };
}

void FNN_MILP::Z3QueryToGurobi::block(const std::vector<StateValue>& assignment) {
    if (assignment.empty()) { throw std::runtime_error("Cannot block an empty MILP state assignment"); }

    GRBLinExpr differs = 0;
    for (const auto& entry: assignment) {
        if (entry.isBool) {
            differs += entry.value == 0 ? entry.variable : 1 - entry.variable;
            continue;
        }
        auto below = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, auxiliary_name("state_below"));
        auto above = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, auxiliary_name("state_above"));
        model.addGenConstrIndicator(below, 1, entry.variable, GRB_LESS_EQUAL, static_cast<double>(entry.value - 1));
        model.addGenConstrIndicator(above, 1, entry.variable, GRB_GREATER_EQUAL, static_cast<double>(entry.value + 1));
        differs += below + above;
    }
    model.addConstr(differs >= 1);
    model.update();
}
