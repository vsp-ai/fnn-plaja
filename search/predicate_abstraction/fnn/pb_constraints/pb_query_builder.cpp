#include "pb_query_builder.h"

#include "../../../../parser/ast/assignment.h"
#include "../../../../parser/ast/expression/binary_op_expression.h"
#include "../../../../parser/ast/expression/bool_value_expression.h"
#include "../../../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../../../parser/ast/expression/unary_op_expression.h"
#include "../../../../parser/fnn_parser/formularized_network.h"
#include "../../../../utils/floating_utils.h"
#include "../../../../parser/visitor/extern/ast_standardization.h"
#include "../../../../parser/visitor/extern/to_linear_expression.h"
#include "../../../information/jani2fnn/jani_2_fnn.h"
#include "../../../information/model_information.h"
#include "../../pa_states/abstract_state.h"
#include "../../smt/model_z3_pa.h"
#include "../../successor_generation/action_op/action_op_pa.h"
#include "../../successor_generation/update/update_pa.h"
#include "../../../successor_generation/action_op.h"
#include "../../../successor_generation/base/action_op_base.h"
#include "../../../successor_generation/update.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

enum class StateCopy { Current, Successor };
constexpr long long FloatScale = 1000000;

long long checked_add(long long lhs, long long rhs, const char* context) {
    if ((rhs > 0 and lhs > std::numeric_limits<long long>::max() - rhs)
        or (rhs < 0 and lhs < std::numeric_limits<long long>::min() - rhs)) {
        throw std::overflow_error(std::string("PB overflow while ") + context);
    }
    return lhs + rhs;
}

long long checked_mul(long long lhs, long long rhs, const char* context) {
    long long result = 0;
    if (__builtin_mul_overflow(lhs, rhs, &result)) {
        throw std::overflow_error(std::string("PB overflow while ") + context);
    }
    return result;
}

long long scale(double value) {
    const auto scaled = std::trunc(value * static_cast<double>(FloatScale));
    if (not std::isfinite(scaled)
        or scaled < static_cast<double>(std::numeric_limits<long long>::min())
        or scaled > static_cast<double>(std::numeric_limits<long long>::max())) {
        throw std::overflow_error("PB FNN coefficient is outside signed 64-bit range");
    }
    return static_cast<long long>(scaled);
}

std::string one_hot_name(VariableIndex_type index, PLAJA::integer value, StateCopy copy) {
    return std::string(copy == StateCopy::Current ? "x_" : "xp_")
           + std::to_string(index + 1) + "_" + std::to_string(value);
}

std::unique_ptr<LinearExpression> to_linear(const Expression& expression) {
    if (TO_LINEAR_EXP::as_linear_exp(expression)) { return TO_LINEAR_EXP::to_linear_exp(expression); }
    auto standardized = AST_STANDARDIZATION::standardize_ast<Expression>(expression);
    if (standardized and TO_LINEAR_EXP::as_linear_exp(*standardized)) {
        return TO_LINEAR_EXP::to_linear_exp(*standardized);
    }
    return nullptr;
}

class Builder final {
private:
    const FNN_PB::QueryContext& query;
    const ModelInformation& modelInfo;
    const PLAJA::FormularizedNetwork& fnn;
    std::size_t freshIndex = 0;

    std::string fresh_name(const std::string& prefix) { return prefix + "__" + std::to_string(freshIndex++); }

    FNN_PB::Literal one_hot(FNN_PB::Formula& formula, VariableIndex_type index,
                             PLAJA::integer value, StateCopy copy) const {
        return formula.variable(one_hot_name(index, value, copy));
    }

    std::vector<FNN_PB::Term> state_value(FNN_PB::Formula& formula, VariableIndex_type index,
                                           StateCopy copy, long long factor = 1) const {
        std::vector<FNN_PB::Term> terms;
        const auto lower = modelInfo.get_lower_bound_int(index);
        const auto upper = modelInfo.get_upper_bound_int(index);
        for (PLAJA::integer value = lower; value <= upper; ++value) {
            terms.push_back({ checked_mul(factor, value, "encoding a state value"),
                              one_hot(formula, index, value, copy) });
        }
        return terms;
    }

    struct LinearSum final {
        std::vector<FNN_PB::Term> terms;
        long long constant = 0;
    };

    LinearSum linear_sum(FNN_PB::Formula& formula, const LinearExpression& expression, StateCopy copy) const {
        LinearSum result;
        for (auto addend = expression.addendIterator(); not addend.end(); ++addend) {
            const auto coefficient = static_cast<long long>(addend.factor()->evaluate_integer_const());
            auto terms = state_value(formula, addend.var_index(), copy, coefficient);
            result.terms.insert(result.terms.end(), terms.begin(), terms.end());
        }
        if (const auto* scalar = expression.get_scalar()) {
            result.constant = scalar->evaluate_integer_const();
        }
        return result;
    }

    FNN_PB::RawConstraint linear_comparison(FNN_PB::Formula& formula,
                                              const LinearExpression& expression,
                                              StateCopy copy) const {
        if (not expression.is_integer() or expression.is_linear_sum()) {
            throw std::runtime_error("PB expected an integer linear comparison");
        }
        auto sum = linear_sum(formula, expression, copy);
        FNN_PB::Relation relation;
        auto rhs = sum.constant;
        switch (expression.get_op()) {
            case BinaryOpExpression::GE: relation = FNN_PB::Relation::GE; break;
            case BinaryOpExpression::GT:
                relation = FNN_PB::Relation::GE;
                rhs = checked_add(rhs, 1, "converting > to >=");
                break;
            case BinaryOpExpression::LE: relation = FNN_PB::Relation::LE; break;
            case BinaryOpExpression::LT:
                relation = FNN_PB::Relation::LE;
                rhs = checked_add(rhs, -1, "converting < to <=");
                break;
            case BinaryOpExpression::EQ: relation = FNN_PB::Relation::EQ; break;
            case BinaryOpExpression::NE:
                throw std::logic_error("PB disequality must be handled through equality reification");
            default: throw std::runtime_error("PB unsupported linear comparison operator");
        }
        return { std::move(sum.terms), relation, rhs };
    }

    FNN_PB::Literal expression_literal(FNN_PB::Formula& formula, const Expression& expression,
                                        StateCopy copy, const std::string& name) {
        if (const auto* boolean = dynamic_cast<const BoolValueExpression*>(&expression)) {
            const auto literal = formula.variable(name);
            formula.add_fixed(literal, boolean->get_value());
            return literal;
        }
        if (const auto* unary = dynamic_cast<const UnaryOpExpression*>(&expression)) {
            if (unary->get_op() != UnaryOpExpression::NOT or not unary->get_operand()) {
                throw std::runtime_error("PB unsupported unary expression: " + expression.to_string(true));
            }
            return FNN_PB::negate(expression_literal(formula, *unary->get_operand(), copy, name + "__not"));
        }
        if (const auto* nary = dynamic_cast<const NaryExpression*>(&expression)) {
            std::vector<FNN_PB::Literal> children;
            std::size_t index = 0;
            for (auto it = nary->iterator(); not it.end(); ++it, ++index) {
                children.push_back(expression_literal(formula, *it, copy, name + "__" + std::to_string(index)));
            }
            if (nary->get_op() == BinaryOpExpression::AND) { return formula.reify_and(children, name); }
            if (nary->get_op() == BinaryOpExpression::OR) { return formula.reify_or(children, name); }
            throw std::runtime_error("PB unsupported n-ary expression: " + expression.to_string(true));
        }
        if (const auto* binary = dynamic_cast<const BinaryOpExpression*>(&expression)) {
            if (binary->get_op() == BinaryOpExpression::AND or binary->get_op() == BinaryOpExpression::OR
                or binary->get_op() == BinaryOpExpression::IMPLIES) {
                if (not binary->get_left() or not binary->get_right()) {
                    throw std::runtime_error("PB malformed binary Boolean expression");
                }
                const auto lhs = expression_literal(formula, *binary->get_left(), copy, name + "__lhs");
                const auto rhs = expression_literal(formula, *binary->get_right(), copy, name + "__rhs");
                if (binary->get_op() == BinaryOpExpression::AND) { return formula.reify_and({lhs, rhs}, name); }
                if (binary->get_op() == BinaryOpExpression::OR) { return formula.reify_or({lhs, rhs}, name); }
                return formula.reify_or({FNN_PB::negate(lhs), rhs}, name);
            }
        }
        auto linear = to_linear(expression);
        if (not linear or not linear->is_integer() or linear->is_linear_sum()) {
            throw std::runtime_error("PB unsupported expression: " + expression.to_string(true));
        }
        if (linear->get_op() == BinaryOpExpression::NE) {
            auto equality = linear_comparison_for_equality(formula, *linear, copy);
            return FNN_PB::negate(formula.reify(std::move(equality), name + "__eq"));
        }
        return formula.reify(linear_comparison(formula, *linear, copy), name);
    }

    FNN_PB::RawConstraint linear_comparison_for_equality(FNN_PB::Formula& formula,
                                                           const LinearExpression& expression,
                                                           StateCopy copy) const {
        auto sum = linear_sum(formula, expression, copy);
        return { std::move(sum.terms), FNN_PB::Relation::EQ, sum.constant };
    }

    void add_constraint(FNN_PB::Formula& formula, FNN_PB::RawConstraint constraint,
                        FNN_PB::Literal activation) const {
        if (activation == 0) { formula.add(std::move(constraint)); }
        else { formula.add_guarded(std::move(constraint), activation); }
    }

public:
    explicit Builder(const FNN_PB::QueryContext& query_)
        : query(query_), modelInfo(query_.jani2FNN->get_model_info()), fnn(query_.jani2FNN->load_fnn()) {}

    FNN_PB::Literal reify_expression(FNN_PB::Formula& formula, const Expression& expression,
                                      StateCopy copy, const std::string& name) {
        return expression_literal(formula, expression, copy, name);
    }

    void state_one_hot(FNN_PB::Formula& formula, StateCopy copy) const {
        for (VariableIndex_type index = 0; index < modelInfo.get_int_state_size(); ++index) {
            std::vector<FNN_PB::Term> terms;
            for (PLAJA::integer value = modelInfo.get_lower_bound_int(index);
                 value <= modelInfo.get_upper_bound_int(index); ++value) {
                terms.push_back({1, one_hot(formula, index, value, copy)});
            }
            formula.add_eq(std::move(terms), 1);
        }
    }

    void abstract_state(FNN_PB::Formula& formula, const AbstractState& state, StateCopy copy,
                        std::vector<FNN_PB::Literal>* predicate_literals = nullptr) {
        for (auto location = state.init_loc_state_it(); not location.end(); ++location) {
            formula.add_fixed(one_hot(formula, location.location_index(), location.location_value(), copy));
        }
        if (predicate_literals) { predicate_literals->assign(state.get_size_predicates(), 0); }
        for (auto predicate = state.init_pred_state_it(); not predicate.end(); ++predicate) {
            const auto index = predicate.predicate_index();
            const auto literal = expression_literal(
                formula, predicate.predicate(), copy,
                std::string(copy == StateCopy::Current ? "pred_curr_" : "pred_succ_") + std::to_string(index));
            if (predicate_literals) { predicate_literals->at(index) = literal; }
            if (not predicate_literals and predicate.is_set()) {
                formula.add_fixed(literal, predicate.predicate_value() == PA::True);
            }
        }
    }

    FNN_PB::Literal guard_literal(FNN_PB::Formula& formula, const ActionOpPA& action_op,
                                   const std::string& name) {
        std::vector<FNN_PB::Literal> children;
        const auto& concrete = action_op._concrete();
        for (auto location = concrete.locationIterator(); not location.end(); ++location) {
            children.push_back(one_hot(formula, location.loc(), location.src(), StateCopy::Current));
        }
        std::size_t index = 0;
        for (auto guard = concrete.guardIterator(); not guard.end(); ++guard, ++index) {
            children.push_back(expression_literal(formula, *guard, StateCopy::Current,
                                                  name + "__part_" + std::to_string(index)));
        }
        return formula.reify_and(children, name);
    }

    void update(FNN_PB::Formula& formula, const ActionOpPA& action_op,
                UpdateIndex_type update_index, FNN_PB::Literal activation = 0) {
        if (update_index >= action_op.size()) {
            if (activation == 0) { throw std::runtime_error("PB update index is outside the selected operator"); }
            formula.add_fixed(activation, false);
            return;
        }
        const auto& concrete = action_op.get_update_pa(update_index)._concrete();
        for (auto location = concrete.locationIterator(); not location.end(); ++location) {
            add_constraint(formula,
                           {{{1, one_hot(formula, location.loc(), location.dest(), StateCopy::Successor)}},
                            FNN_PB::Relation::GE, 1}, activation);
        }
        for (SequenceIndex_type sequence = 0; sequence < concrete.get_num_sequences(); ++sequence) {
            for (auto assignment = concrete.assignmentIterator(sequence); not assignment.end(); ++assignment) {
                if (assignment.array_index()) {
                    throw std::runtime_error("PB array assignment is unsupported: " + assignment->to_string(true));
                }
                auto lhs = state_value(formula, assignment.variable_index(), StateCopy::Successor);
                if (assignment.is_non_deterministic()) {
                    if (assignment.lower_bound()) {
                        auto lower = to_linear(*assignment.lower_bound());
                        if (not lower or not lower->is_integer() or not lower->is_linear_sum()) {
                            throw std::runtime_error("PB nonlinear nondeterministic lower bound: "
                                                     + assignment.lower_bound()->to_string(true));
                        }
                        auto rhs = linear_sum(formula, *lower, StateCopy::Current);
                        auto terms = lhs;
                        for (const auto& term : rhs.terms) { terms.push_back({-term.coefficient, term.literal}); }
                        add_constraint(formula, {std::move(terms), FNN_PB::Relation::GE, rhs.constant}, activation);
                    }
                    if (assignment.upper_bound()) {
                        auto upper = to_linear(*assignment.upper_bound());
                        if (not upper or not upper->is_integer() or not upper->is_linear_sum()) {
                            throw std::runtime_error("PB nonlinear nondeterministic upper bound: "
                                                     + assignment.upper_bound()->to_string(true));
                        }
                        auto rhs = linear_sum(formula, *upper, StateCopy::Current);
                        auto terms = lhs;
                        for (const auto& term : rhs.terms) { terms.push_back({-term.coefficient, term.literal}); }
                        add_constraint(formula, {std::move(terms), FNN_PB::Relation::LE, rhs.constant}, activation);
                    }
                    continue;
                }
                if (not assignment.value()) { throw std::runtime_error("PB deterministic assignment has no value"); }
                auto rhs_expression = to_linear(*assignment.value());
                if (not rhs_expression or not rhs_expression->is_integer() or not rhs_expression->is_linear_sum()) {
                    throw std::runtime_error("PB nonlinear assignment: " + assignment.value()->to_string(true));
                }
                auto rhs = linear_sum(formula, *rhs_expression, StateCopy::Current);
                for (const auto& term : rhs.terms) { lhs.push_back({-term.coefficient, term.literal}); }
                add_constraint(formula, {std::move(lhs), FNN_PB::Relation::EQ, rhs.constant}, activation);
            }
            for (const auto state_index : concrete.infer_non_updated_state_indexes(sequence, true)) {
                if (state_index >= modelInfo.get_int_state_size()) { continue; }
                auto terms = state_value(formula, state_index, StateCopy::Successor);
                auto current = state_value(formula, state_index, StateCopy::Current);
                for (const auto& term : current) { terms.push_back({-term.coefficient, term.literal}); }
                add_constraint(formula, {std::move(terms), FNN_PB::Relation::EQ, 0}, activation);
            }
        }
    }

    std::vector<FNN_PB::Literal> fnn_constraints(FNN_PB::Formula& formula) {
        const auto& nodes = fnn.get_nodes();
        std::vector<FNN_PB::Literal> node_literals(nodes.size(), 0);
        for (int index = 0; index < fnn.get_init_logical_idx(); ++index) {
            node_literals.at(index) = formula.variable("b_" + std::to_string(index + 1));
        }
        for (int index = fnn.get_init_logical_idx(); index < fnn.get_init_linear_idx(); ++index) {
            node_literals.at(index) = formula.variable("n_" + std::to_string(index + 1));
        }
        {
            {
                const auto& bins = fnn.get_bin_features();
                if (bins.size() != static_cast<std::size_t>(fnn.get_init_logical_idx())) {
                    throw std::runtime_error("PB bin-feature count does not match input-node count");
                }
                for (int index = 0; index < fnn.get_init_logical_idx(); ++index) {
                    const auto& bin = bins.at(index);
                    // Single-variable integer eq/neq bins (onehot atoms) map
                    // directly onto the state's one-hot literal instead of a
                    // reified linear constraint.
                    if ((bin.op == PLAJA::PredicateOp::EQ or bin.op == PLAJA::PredicateOp::NEQ)
                        and bin.terms.size() == 1
                        and bin.terms.front().coeff == static_cast<PLAJA::floating>(1)
                        and PLAJA_FLOATS::is_integer(bin.constant, PLAJA::floatingPrecision)) {
                        const auto state_index = bin.terms.front().stateIndex;
                        const auto value = static_cast<PLAJA::integer>(std::llround(static_cast<double>(bin.constant)));
                        if (state_index < modelInfo.get_int_state_size()
                            and value >= modelInfo.get_lower_bound_int(state_index)
                            and value <= modelInfo.get_upper_bound_int(state_index)) {
                            const auto literal = one_hot(formula, state_index, value, StateCopy::Current);
                            formula.add_eq({{1, node_literals.at(index)},
                                            {-1, bin.op == PLAJA::PredicateOp::NEQ ? FNN_PB::negate(literal) : literal}}, 0);
                            continue;
                        }
                    }
                    std::vector<FNN_PB::Term> terms;
                    for (const auto& term : bin.terms) {
                        auto state_terms = state_value(formula, term.stateIndex, StateCopy::Current,
                                                       scale(static_cast<double>(term.coeff)));
                        terms.insert(terms.end(), state_terms.begin(), state_terms.end());
                    }
                    FNN_PB::Relation relation;
                    auto rhs = scale(static_cast<double>(bin.constant));
                    bool negate_result = false;
                    switch (bin.op) {
                        case PLAJA::PredicateOp::GEQ: relation = FNN_PB::Relation::GE; break;
                        case PLAJA::PredicateOp::GT: relation = FNN_PB::Relation::GE; rhs = checked_add(rhs, 1, "encoding predicate bin >"); break;
                        case PLAJA::PredicateOp::LEQ: relation = FNN_PB::Relation::LE; break;
                        case PLAJA::PredicateOp::LT: relation = FNN_PB::Relation::LE; rhs = checked_add(rhs, -1, "encoding predicate bin <"); break;
                        case PLAJA::PredicateOp::EQ: relation = FNN_PB::Relation::EQ; break;
                        case PLAJA::PredicateOp::NEQ: relation = FNN_PB::Relation::EQ; negate_result = true; break;
                        default: throw std::runtime_error("PB unsupported predicate-bin operator");
                    }
                    const auto condition = formula.reify({std::move(terms), relation, rhs},
                                                         "bin_condition_" + std::to_string(index));
                    formula.add_eq({{1, node_literals.at(index)},
                                    {-1, negate_result ? FNN_PB::negate(condition) : condition}}, 0);
                }
            }
        }
        {
            for (int index = fnn.get_init_logical_idx(); index < fnn.get_init_linear_idx(); ++index) {
                std::vector<FNN_PB::Literal> predecessors;
                for (const auto predecessor : nodes.at(index).preds) {
                    if (predecessor < 0 or static_cast<std::size_t>(predecessor) >= node_literals.size()
                        or node_literals.at(predecessor) == 0) {
                        throw std::runtime_error("PB logical node has an invalid predecessor");
                    }
                    predecessors.push_back(node_literals.at(predecessor));
                }
                const auto name = "n_" + std::to_string(index + 1);
                if (nodes.at(index).type == PLAJA::NodeType::NOT) {
                    // Negation needs no constraint: the node's literal is the
                    // negated literal of its single predecessor.
                    if (predecessors.size() != 1) { throw std::runtime_error("PB NOT node must have exactly one predecessor"); }
                    node_literals.at(index) = FNN_PB::negate(predecessors.front());
                    continue;
                }
                FNN_PB::Literal encoded = 0;
                if (nodes.at(index).type == PLAJA::NodeType::AND) {
                    encoded = formula.reify_and(predecessors, name);
                } else if (nodes.at(index).type == PLAJA::NodeType::OR) {
                    encoded = formula.reify_or(predecessors, name);
                } else { throw std::runtime_error("PB non-Boolean node in logical FNN layer"); }
                if (encoded != node_literals.at(index)) { throw std::logic_error("PB logical-node literal mismatch"); }
            }
        }
        return node_literals;
    }

    FNN_PB::Literal label_applicability(FNN_PB::Formula& formula, ActionLabel_type label,
                                         const std::string& name) {
        if (not query.modelZ3) { throw std::runtime_error("PB applicability filtering has no PA model"); }
        std::vector<FNN_PB::Literal> guards;
        for (const auto* abstract_op : query.modelZ3->get_action_ops(label)) {
            const auto* action_op = dynamic_cast<const ActionOpPA*>(abstract_op);
            if (not action_op) { throw std::runtime_error("PB encountered a non-PA action operator"); }
            guards.push_back(guard_literal(formula, *action_op,
                                           name + "__op_" + std::to_string(action_op->_op_id())));
        }
        return formula.reify_or(guards, name);
    }

    void output_constraints(FNN_PB::Formula& formula, const std::vector<FNN_PB::Literal>& node_literals) {
        const auto& nodes = fnn.get_nodes();
        const auto& mapping = fnn.get_linear_to_output();
        std::vector<int> output_nodes(query.jani2FNN->get_num_output_features(), -1);
        for (int index = fnn.get_init_linear_idx(); index < static_cast<int>(nodes.size()); ++index) {
            const auto linear_index = static_cast<std::size_t>(index - fnn.get_init_linear_idx());
            if (linear_index >= mapping.size()) { throw std::runtime_error("PB linear-to-output mapping is too short"); }
            const auto output = static_cast<std::size_t>(mapping.at(linear_index));
            if (output < output_nodes.size()) { output_nodes.at(output) = index; }
        }
        const auto chosen = static_cast<std::size_t>(query.outputIndex);
        if (chosen >= output_nodes.size() or output_nodes.at(chosen) < 0) {
            throw std::runtime_error("PB chosen FNN output node is missing");
        }
        const auto& chosen_node = nodes.at(output_nodes.at(chosen));
        if (chosen_node.weights.size() != chosen_node.preds.size()) {
            throw std::runtime_error("PB chosen output has mismatched weights and predecessors");
        }
        for (std::size_t other = 0; other < output_nodes.size(); ++other) {
            if (other == chosen or output_nodes.at(other) < 0) { continue; }
            const auto& other_node = nodes.at(output_nodes.at(other));
            if (other_node.weights.size() != other_node.preds.size()) {
                throw std::runtime_error("PB competing output has mismatched weights and predecessors");
            }
            std::vector<FNN_PB::Term> terms;
            const auto append = [&](const PLAJA::FNNNode& node, long long sign) {
                for (std::size_t index = 0; index < node.preds.size(); ++index) {
                    const auto predecessor = node.preds.at(index);
                    if (predecessor < 0 or static_cast<std::size_t>(predecessor) >= node_literals.size()
                        or node_literals.at(predecessor) == 0) {
                        throw std::runtime_error("PB output references an invalid Boolean predecessor");
                    }
                    terms.push_back({checked_mul(sign, scale(static_cast<double>(node.weights.at(index))),
                                                 "encoding an output weight"), node_literals.at(predecessor)});
                }
            };
            append(chosen_node, 1);
            append(other_node, -1);
            // Scale each bias on its own, like the weights, so that all comparisons use one integer network.
            // Scaling the difference instead can differ by one per pair and need not match any bias vector.
            auto rhs = checked_add(scale(static_cast<double>(other_node.bias)), -scale(static_cast<double>(chosen_node.bias)),
                                   "encoding an output bias difference");
            if (other < chosen) { rhs = checked_add(rhs, 1, "encoding FNN tie breaking"); }
            FNN_PB::RawConstraint comparison{std::move(terms), FNN_PB::Relation::GE, rhs};
            if (query.useAppFiltering) {
                const auto applicability = label_applicability(
                    formula, query.jani2FNN->get_output_label(static_cast<OutputIndex_type>(other)),
                    "app_label_" + std::to_string(other));
                auto normalized = FNN_PB::normalize_inequality(comparison);
                if (normalized.kind == FNN_PB::NormalizedKind::Contradiction) {
                    formula.add_fixed(applicability, false);
                } else if (normalized.kind == FNN_PB::NormalizedKind::Proper) {
                    formula.add_guarded(normalized.constraint, applicability);
                }
            } else { formula.add(std::move(comparison)); }
        }
    }
};

void validate_base_query(const FNN_PB::QueryContext& query) {
    if (not query.jani2FNN or not query.modelZ3 or not query.source) {
        throw std::runtime_error("PB query is missing model or source state");
    }
    if (query.useTargetState and not query.target) { throw std::runtime_error("PB target state is missing"); }
    if ((query.useGuard or query.useGuardNegation or query.useUpdate) and not query.actionOp) {
        throw std::runtime_error("PB selected operator is missing");
    }
}

void add_state_assumptions(const AbstractState& state, const FNN_PB::IncrementalFormula& cached,
                           StateCopy copy, std::vector<FNN_PB::Literal>& assumptions) {
    for (auto location = state.init_loc_state_it(); not location.end(); ++location) {
        const auto symbol = cached.formula.symbols().find(
            one_hot_name(location.location_index(), location.location_value(), copy));
        if (symbol == cached.formula.symbols().end()) {
            throw std::runtime_error("PB incremental state literal is missing");
        }
        assumptions.push_back(symbol->second);
    }
    const auto& predicates = copy == StateCopy::Current
                             ? cached.currentPredicateLiterals : cached.successorPredicateLiterals;
    for (auto predicate = state.init_pred_state_it(); not predicate.end(); ++predicate) {
        if (not predicate.is_set()) { continue; }
        if (predicate.predicate_index() >= predicates.size() or predicates.at(predicate.predicate_index()) == 0) {
            throw std::runtime_error("PB incremental predicate literal is missing");
        }
        const auto literal = predicates.at(predicate.predicate_index());
        assumptions.push_back(predicate.predicate_value() == PA::True ? literal : FNN_PB::negate(literal));
    }
}

} // namespace

FNN_PB::Formula FNN_PB::build_fresh_formula(const QueryContext& query) {
    validate_base_query(query);
    Builder builder(query);
    Formula formula;
    builder.state_one_hot(formula, StateCopy::Current);
    if (query.useUpdate or query.useTargetState) { builder.state_one_hot(formula, StateCopy::Successor); }
    auto nodes = builder.fnn_constraints(formula);
    {
        builder.abstract_state(formula, *query.source, StateCopy::Current);
    }
    if (query.requireNonTerminal and query.modelZ3->get_non_terminal()) {
        // Like the Z3 checker, skip the condition if the model has none.
        const auto literal = builder.reify_expression(
            formula, *query.modelZ3->get_non_terminal(), StateCopy::Current, "non_terminal");
        formula.add_fixed(literal);
    }
    if (query.useGuard or query.useGuardNegation) {
        const auto guard = builder.guard_literal(formula, *query.actionOp,
                                                 "fresh_guard_op_" + std::to_string(query.actionOpId));
        formula.add_fixed(guard, query.useGuard and not query.useGuardNegation);
    }
    if (query.useUpdate) {
        builder.update(formula, *query.actionOp, query.updateIndex);
    }
    if (query.useTargetState) {
        builder.abstract_state(formula, *query.target, StateCopy::Successor);
    }
    if (query.useOutputInterface) { builder.output_constraints(formula, nodes); }
    return formula;
}

std::string FNN_PB::incremental_query_error(const QueryContext& query) {
    if (not query.jani2FNN or not query.modelZ3 or not query.source or not query.target or not query.actionOp) {
        return "missing model, source, target, or action-operator context";
    }
    if (query.actionLabel == ACTION::noneLabel or query.actionOpId == ACTION::noneOp
        or query.updateIndex == ACTION::noneUpdate) {
        return "missing action label, operator id, or update index";
    }
    if (not query.useOutputInterface or not query.useGuard or not query.useUpdate or not query.useTargetState) {
        return "expected output-interface, guard, update, and target constraints for a complete TSAT query";
    }
    if (query.useGuardNegation) { return "guard-negation queries are unsupported"; }
    if (query.updateIndex >= query.actionOp->size()) { return "selected update index is outside the selected operator"; }
    return {};
}

FNN_PB::IncrementalFormula FNN_PB::build_incremental_formula(const QueryContext& query,
                                                                Granularity granularity) {
    const auto error = incremental_query_error(query);
    if (not error.empty()) { throw std::runtime_error("unsupported PB incremental query: " + error); }
    Builder builder(query);
    IncrementalFormula cached;
    builder.state_one_hot(cached.formula, StateCopy::Current);
    builder.state_one_hot(cached.formula, StateCopy::Successor);
    auto nodes = builder.fnn_constraints(cached.formula);
    builder.output_constraints(cached.formula, nodes);
    {
        builder.abstract_state(cached.formula, *query.source, StateCopy::Current,
                               &cached.currentPredicateLiterals);
        builder.abstract_state(cached.formula, *query.source, StateCopy::Successor,
                               &cached.successorPredicateLiterals);
    }
    if (query.requireNonTerminal and query.modelZ3->get_non_terminal()) {
        const auto literal = builder.reify_expression(
            cached.formula, *query.modelZ3->get_non_terminal(), StateCopy::Current, "non_terminal");
        cached.formula.add_fixed(literal);
    }
    const auto add_operator = [&](const ActionOpPA& action_op) {
        const auto id = action_op._op_id();
        const auto activation = cached.formula.variable("activate_op_" + std::to_string(id));
        cached.activationLiterals.emplace(id, activation);
        cached.actionOperators.push_back(id);
        {
            const auto guard = builder.guard_literal(cached.formula, action_op,
                                                     "incremental_guard_op_" + std::to_string(id));
            cached.formula.add_ge({{1, negate(activation)}, {1, guard}}, 1);
        }
        {
            builder.update(cached.formula, action_op, query.updateIndex, activation);
        }
    };
    if (granularity == Granularity::Operator) {
        add_operator(*query.actionOp);
    } else {
        const auto& operators = query.modelZ3->get_action_ops(query.actionLabel);
        if (operators.empty()) { throw std::runtime_error("PB found no operator for the selected label"); }
        for (const auto* abstract_op : operators) {
            const auto* action_op = dynamic_cast<const ActionOpPA*>(abstract_op);
            if (not action_op) { throw std::runtime_error("PB encountered a non-PA action operator"); }
            add_operator(*action_op);
        }
    }
    return cached;
}

std::vector<FNN_PB::Literal> FNN_PB::build_incremental_assumptions(
    const QueryContext& query, const IncrementalFormula& cached) {
    const auto error = incremental_query_error(query);
    if (not error.empty()) { throw std::runtime_error("unsupported PB incremental query: " + error); }
    std::vector<Literal> assumptions;
    add_state_assumptions(*query.source, cached, StateCopy::Current, assumptions);
    add_state_assumptions(*query.target, cached, StateCopy::Successor, assumptions);
    for (const auto id : cached.actionOperators) {
        const auto literal = cached.activationLiterals.at(id);
        assumptions.push_back(id == query.actionOpId ? literal : negate(literal));
    }
    if (not cached.activationLiterals.count(query.actionOpId)) {
        throw std::runtime_error("PB selected operator is absent from the cached formula");
    }
    return assumptions;
}

std::vector<PLAJA::integer> FNN_PB::decode_state(const QueryContext& query, const Formula& formula,
                                                  const std::function<bool(Literal)>& value, bool successor) {
    const auto& model_info = query.jani2FNN->get_model_info();
    const auto copy = successor ? StateCopy::Successor : StateCopy::Current;
    std::vector<PLAJA::integer> state;
    state.reserve(model_info.get_int_state_size());
    for (VariableIndex_type index = 0; index < model_info.get_int_state_size(); ++index) {
        bool found = false;
        for (PLAJA::integer candidate = model_info.get_lower_bound_int(index);
             candidate <= model_info.get_upper_bound_int(index); ++candidate) {
            const auto symbol = formula.symbols().find(one_hot_name(index, candidate, copy));
            if (symbol != formula.symbols().end() and value(symbol->second)) {
                state.push_back(candidate);
                found = true;
                break;
            }
        }
        if (not found) { throw std::runtime_error("PB model has no one-hot value for state variable " + std::to_string(index)); }
    }
    return state;
}
