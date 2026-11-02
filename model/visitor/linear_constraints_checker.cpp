//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model/visitor/linear_constraints_checker.h"
#include "core/utils/floating_utils.h"
#include "core/utils/utils.h"
#include "model/ast/expression/non_standard/let_expression.h"
#include "model/ast/expression/non_standard/state_condition_expression.h"
#include "model/ast/expression/special_cases/nary_expression.h"
#include "model/ast/expression/special_cases/linear_expression.h"
#include "model/ast/expression/array_access_expression.h"
#include "model/ast/expression/unary_op_expression.h"
#include "model/ast/type/declaration_type.h"

#define LCC_CHECKER_NONE(STR) // PLAJA_LOG_DEBUG(STR) // for debugging

namespace LINEAR_CONSTRAINTS_CHECKER {

    [[nodiscard]] bool is_internally_possible(BinaryOpExpression::BinaryOp binary_op, LinearConstraintsChecker::LinearType requested_type) { // NOLINT(*-no-recursion)

        switch (requested_type) {

            case LinearConstraintsChecker::NONE: { PLAJA_ABORT; }

            case LinearConstraintsChecker::ADDEND: {
                static const std::unordered_set<BinaryOpExpression::BinaryOp> possible_ops { BinaryOpExpression::PLUS, BinaryOpExpression::MINUS, BinaryOpExpression::TIMES };
                return possible_ops.count(binary_op);
            }

            case LinearConstraintsChecker::CON_BOOL: { return binary_op == BinaryOpExpression::AND; }

            case LinearConstraintsChecker::DIS_BOOL: { return binary_op == BinaryOpExpression::OR; }

            case LinearConstraintsChecker::CONSTRAINT: {
                static const std::unordered_set<BinaryOpExpression::BinaryOp> possible_ops { BinaryOpExpression::EQ, BinaryOpExpression::NE, BinaryOpExpression::LT, BinaryOpExpression::LE, BinaryOpExpression::GT, BinaryOpExpression::GE };
                return possible_ops.count(binary_op) or is_internally_possible(binary_op, LinearConstraintsChecker::ADDEND);
            }

            case LinearConstraintsChecker::SUM: { return is_internally_possible(binary_op, LinearConstraintsChecker::ADDEND); }

            case LinearConstraintsChecker::ASSIGNMENT: { return is_internally_possible(binary_op, LinearConstraintsChecker::SUM); }

            case LinearConstraintsChecker::CON:
            case LinearConstraintsChecker::DIS: {
                return binary_op == BinaryOpExpression::AND or binary_op == BinaryOpExpression::OR or is_internally_possible(binary_op, LinearConstraintsChecker::CONSTRAINT);
            }

            case LinearConstraintsChecker::CONDITION: { return true; }

            default: { return false; }
        }

        PLAJA_ABORT

    }

}

/**********************************************************************************************************************/

LinearConstraintsChecker::LinearConstraintsChecker(LinearType requested_type, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification):
    type(NONE)
    , requestedType(requested_type)
    , specification(&specification)
    , isNf(true)
    , seenConsts(0) {}

LinearConstraintsChecker::~LinearConstraintsChecker() = default;

/**********************************************************************************************************************/

bool LinearConstraintsChecker::is(LinearConstraintsChecker::LinearType expr_type, const Expression& expr, LinearConstraintsChecker::LinearType super_type) const { // NOLINT(misc-no-recursion)
    switch (super_type) {
        case NONE: { PLAJA_ABORT; }
        case ADDEND: { return expr_type == ADDEND or (expr_type == VAR and not expr.get_type()->is_boolean_type()); }
        case LITERAL: { return expr_type == LITERAL or expr_type == TRUTH_VALUE or (expr_type == VAR and expr.get_type()->is_boolean_type()); }
        case CON_BOOL: { return expr_type == CON_BOOL or is(expr_type, expr, LITERAL); }
        case DIS_BOOL: { return expr_type == DIS_BOOL or is(expr_type, expr, LITERAL); }
        case CONSTRAINT: { return expr_type == CONSTRAINT; }
        case SUM: { return expr_type == SUM or is(expr_type, expr, ADDEND) or expr_type == SCALAR; }
        case ASSIGNMENT: { return expr_type == ASSIGNMENT or is(expr_type, expr, LITERAL) or is(expr_type, expr, SUM); }
        case CON: { return expr_type == CON or expr_type == DIS or is(expr_type, expr, CONSTRAINT) or is(expr_type, expr, CON_BOOL) or is(expr_type, expr, DIS_BOOL); }
        case DIS: { return expr_type == DIS or expr_type == CON or is(expr_type, expr, CONSTRAINT) or is(expr_type, expr, CON_BOOL) or is(expr_type, expr, DIS_BOOL); }
        case CONDITION: { return expr_type != NONE and not is(expr_type, expr, SUM); }
        default: { return expr_type == super_type; }
    }
    PLAJA_ABORT
}

bool LinearConstraintsChecker::is_internally_possible(LinearConstraintsChecker::LinearType expr_type, LinearConstraintsChecker::LinearType requested_type) { // NOLINT(*-no-recursion)
    switch (requested_type) {

        case NONE: { PLAJA_ABORT; }

        case ADDEND: {
            static const std::unordered_set<LinearType> possible_types { ADDEND, SCALAR, VAR };
            return possible_types.count(expr_type);
        }

        case LITERAL: {
            static const std::unordered_set<LinearType> possible_types { LITERAL, TRUTH_VALUE, VAR };
            return possible_types.count(expr_type);
        }

        case CON_BOOL: { return expr_type == CON_BOOL or is_internally_possible(expr_type, LITERAL); }

        case DIS_BOOL: { return expr_type == DIS_BOOL or is_internally_possible(expr_type, LITERAL); }

        case CONSTRAINT: { return expr_type == CONSTRAINT or is_internally_possible(expr_type, SUM); }

        case SUM: { return expr_type == SUM or is_internally_possible(expr_type, ADDEND); }

        case ASSIGNMENT: { return expr_type == ASSIGNMENT or is_internally_possible(expr_type, LITERAL) or is_internally_possible(expr_type, SUM); }

        case CON:
        case DIS: {
            static const std::unordered_set<LinearType> possible_types { CON, DIS, CON_BOOL, DIS_BOOL, CONSTRAINT, SUM };
            return possible_types.count(expr_type) or is_internally_possible(expr_type, ADDEND) or is_internally_possible(expr_type, LITERAL);
        }

        case CONDITION: { return true; }

        default: { return expr_type == requested_type; }
    }

    PLAJA_ABORT
}

/**********************************************************************************************************************/

void LinearConstraintsChecker::visit(const ArrayAccessExpression* exp) {

    if (not specification->allowArrayAccess
        or (not specification->allowBools and exp->is_boolean())) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
        return;
    }

    const auto* index = exp->get_index();

    if (not index->is_constant()) {
        type = NONE;
        return;
    }

    add_to_seen_vars(exp->get_variable_index());
    type = VAR;
}

void LinearConstraintsChecker::visit(const ArrayConstructorExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const ArrayValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const BoolValueExpression* /*exp*/) {
    if (not specification->allowBools) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
    } else { type = TRUTH_VALUE; }
    inc_seen_constants();
}

void LinearConstraintsChecker::visit(const DistributionSamplingExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const BinaryOpExpression* exp) {

    /* Fast check. */
    if (not LINEAR_CONSTRAINTS_CHECKER::is_internally_possible(exp->get_op(), requestedType)) {
        type = NONE;
        return;
    }

    type = NONE;
    exp->get_left()->accept(this);
    if (type == NONE or not is_internally_possible(type, requestedType)) { return; }
    const auto type_l = type;
    type = NONE;
    exp->get_right()->accept(this);
    if (type == NONE or not is_internally_possible(type, requestedType)) { return; }
    const auto type_r = type;

    const auto op = exp->get_op();

    switch (op) {

        case BinaryOpExpression::EQ:
        case BinaryOpExpression::NE:
        case BinaryOpExpression::LT:
        case BinaryOpExpression::LE:
        case BinaryOpExpression::GT:
        case BinaryOpExpression::GE: {
            // normal linear constraint of the form o {<,<=,=,>=,>}
            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("CONSTRAINT - NOT SPECIAL")
                type = NONE;
                return;
            }

            // op check
            if (not(op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
                    or (specification->allowEquality and (op == BinaryOpExpression::EQ or op == BinaryOpExpression::NE) and not specification->asPredicate) // allow equality (but default no for predicates)
                    or (specification->allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT)))) { // allow strict op
                LCC_CHECKER_NONE("STRICT OP")
                type = NONE;
                return;
            }

            // main check
            if (not(is(type_l, *exp->get_left(), SUM) and is(type_r, *exp->get_right(), SUM) and has_addend())) {
                LCC_CHECKER_NONE("SUM - MAIN")
                type = NONE;
                return;
            }

            type = CONSTRAINT;

            if (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) { isNf = false; }; // strict op is not standard however

            if (not is(type_r, *exp->get_right(), SCALAR)) { isNf = false; } // standard form has right-hand scalar

            if (specification->asPredicate and op != BinaryOpExpression::GE) { isNf = false; } // normal form predicate is of form sum >= scalar

            return;
        }

        case BinaryOpExpression::MINUS:
        case BinaryOpExpression::PLUS: {

            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("SUM - NOT SPECIAL")
                type = NONE;
                return;
            }

            if (is(type_l, *exp->get_left(), SUM) and is(type_r, *exp->get_right(), SUM)) { type = SUM; }
            else {
                LCC_CHECKER_NONE("SUM")
                type = NONE;
            }

            return;
        }

        case BinaryOpExpression::TIMES: {

            if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::TRUE) {
                LCC_CHECKER_NONE("MULTIPLICATION - NOT SPECIAL")
                type = NONE;
                return;
            }

            PLAJA_ASSERT(exp->get_left()->get_type()->is_numeric_type())
            PLAJA_ASSERT(exp->get_right()->get_type()->is_numeric_type())

            // ADDEND
            if ((is(type_l, *exp->get_left(), VAR) and is_factor(*exp->get_right()))
                or (is(type_r, *exp->get_right(), VAR) and is_factor(*exp->get_left()))) { type = ADDEND; }
                // SUM
            else if ((is(type_l, *exp->get_left(), SUM) and is_factor(*exp->get_right()))
                     or (is(type_r, *exp->get_right(), SUM) and is_factor(*exp->get_left()))) {
                isNf = false;
                type = SUM;
            } else {
                LCC_CHECKER_NONE("MULTIPLICATION")
                type = NONE;
            }

            return;
        }

        case BinaryOpExpression::OR: {
            if (is(type_l, *exp->get_left(), DIS_BOOL) and is(type_r, *exp->get_right(), DIS_BOOL)) { type = DIS_BOOL; }
            else if (is(type_l, *exp->get_left(), DIS) and is(type_r, *exp->get_right(), DIS)) {
                // additional check for linear disjunction (since may use same vars in both branches)
                if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE) {
                    auto spec_intern = *specification;
                    spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                    seenConsts = 0;
                    seenAddends.clear();
                    isNf = isNf and is_linear(*exp->get_left(), spec_intern) and is_linear(*exp->get_right(), spec_intern);
                }
                type = DIS;
            } else {
                LCC_CHECKER_NONE("OR")
                type = NONE;
            }
            return;
        }

        case BinaryOpExpression::AND: {

            if (is(type_l, *exp->get_left(), CON_BOOL) and is(type_r, *exp->get_right(), CON_BOOL)) { type = CON_BOOL; }
            else if (is(type_l, *exp->get_left(), CON) and is(type_r, *exp->get_right(), CON)) {
                // additional check for linear conjunction (since may use same vars in both branches)
                if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE) {
                    auto spec_intern = *specification;
                    spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                    seenConsts = 0;
                    seenAddends.clear();
                    isNf = isNf and is_linear(*exp->get_left(), spec_intern) and is_linear(*exp->get_right(), spec_intern);
                }
                type = CON;
            } else {
                LCC_CHECKER_NONE("AND")
                type = NONE;
            }

            return;
        }

        default: {
            LCC_CHECKER_NONE("BINARY - DEFAULT")
            type = NONE;
            return;
        }
    }

    PLAJA_ABORT
}

void LinearConstraintsChecker::visit(const IntegerValueExpression* /*exp*/) {
    inc_seen_constants();
    type = SCALAR;
}

void LinearConstraintsChecker::visit(const ITE_Expression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const RealValueExpression* /*exp*/) {
    inc_seen_constants();
    type = SCALAR;
}

void LinearConstraintsChecker::visit(const UnaryOpExpression* exp) {
    switch (exp->get_op()) {
        case UnaryOpExpression::NOT: {
            PLAJA_ASSERT(exp->get_type()->is_boolean_type())
            type = NONE;
            exp->get_operand()->accept(this);
            if (type != TRUTH_VALUE and type != VAR) {  // LITERAL: TRUE OR FALSE or variable
                LCC_CHECKER_NONE("UNARY")
                type = NONE;
            } else { type = LITERAL; }
            return;
        }
        default: {
            LCC_CHECKER_NONE("UNARY - DEFAULT")
            type = NONE;
            return;
        }
    }
    PLAJA_ABORT
}

void LinearConstraintsChecker::visit(const VariableExpression* exp) {

    if (not specification->allowBools and exp->is_boolean()) {
        LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
        type = NONE;
        return;
    }

    add_to_seen_vars(exp->get_index());
    type = VAR;

}

// non-standard

void LinearConstraintsChecker::visit(const ConstantArrayAccessExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE; // If index is constant, we can as well evaluate expression upfront.
}

void LinearConstraintsChecker::visit(const LocationValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const LetExpression* exp) {

    if (exp->get_number_of_free_variables() != 0) { // allow for let expression to occur due to legacy considerations, but no free variable support
        LCC_CHECKER_NONE("LET - FREE VARS")
        type = NONE;
        return;
    }

    type = NONE;
    AST_CONST_ACCEPT_IF(exp->get_expression(), Expression)
}

void LinearConstraintsChecker::visit(const PredicatesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const StateConditionExpression* exp) {
    const auto* constraint = exp->get_constraint();

    if (constraint) {
        type = NONE;

        constraint->accept(this);

        if (not is(type, *constraint, CONDITION)) {

            LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
            type = NONE;
            return;

        }

        if (exp->get_size_loc_values() == 0) { return; } // no locs, just keep type of state var constraint

    }

    type = CON;
}

void LinearConstraintsChecker::visit(const StateValuesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const StatesValuesExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

void LinearConstraintsChecker::visit(const VariableValueExpression*) {
    LCC_CHECKER_NONE(__PRETTY_FUNCTION__)
    type = NONE;
}

// special:

void LinearConstraintsChecker::visit(const LinearExpression* exp) {

    if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE) {
        LCC_CHECKER_NONE("SPECIAL")
        type = NONE;
        return;
    }

    if (exp->is_linear_sum()) { type = (specification->needsAddends and exp->get_number_addends() == 0) ? NONE : SUM; }
    else {
        PLAJA_ASSERT(exp->is_linear_constraint())
        PLAJA_ASSERT(exp->get_number_addends() > 0)

        const auto op = exp->get_op();

        // check op
        if (not(op == BinaryOpExpression::LE or op == BinaryOpExpression::GE
                or (specification->allowEquality and (op == BinaryOpExpression::EQ or op == BinaryOpExpression::NE) and not specification->asPredicate) // allow equality (but default no for predicates)
                or (specification->allowStrictOp and (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT)))) { // allow strict op
            LCC_CHECKER_NONE("SPECIAL - STRICT OP")
            type = NONE;
            return;
        }

        type = CONSTRAINT;

        if (op == BinaryOpExpression::LT or op == BinaryOpExpression::GT) { isNf = false; } // strict op is not standard however

        if (specification->asPredicate and op != BinaryOpExpression::GE) { isNf = false; }

    }
}

void LinearConstraintsChecker::visit(const NaryExpression* exp) {

    if (specification->specialRequest == LINEAR_CONSTRAINTS_CHECKER::SpecialAstRequest::FALSE) {
        LCC_CHECKER_NONE("JUNCTION - SPECIAL")
        type = NONE;
        return;
    }

    switch (exp->get_op()) {

        case BinaryOpExpression::OR: {

            bool dis_bool = true;

            for (auto it = exp->iterator(); !it.end(); ++it) {

                type = NONE;

                it->accept(this);

                if (not is(type, *it, DIS_BOOL)) {
                    dis_bool = false;
                }

                if (not is(type, *it, DIS)) {
                    LCC_CHECKER_NONE("JUNCTION - OR")
                    type = NONE;
                    return;
                }

            }

            type = dis_bool ? DIS_BOOL : DIS;

            // additional check for linear disjunction (since may use same vars in both branches)
            if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE and not dis_bool) {

                auto spec_intern = *specification;
                spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                seenConsts = 0;
                seenAddends.clear();

                for (auto it = exp->iterator(); !it.end(); ++it) {
                    if (not is_linear(*it, spec_intern)) {
                        isNf = false;
                        break;
                    }
                }

            }

            return;
        }

        case BinaryOpExpression::AND: {

            bool con_bool = true;

            for (auto it = exp->iterator(); !it.end(); ++it) {

                type = NONE;

                it->accept(this);

                if (not is(type, *it, CON_BOOL)) {
                    con_bool = false;
                }

                if (not is(type, *it, CON)) {
                    LCC_CHECKER_NONE("JUNCTION - OR")
                    type = NONE;
                    return;
                }

            }

            type = con_bool ? CON_BOOL : CON;

            // additional check for linear conjunction (since may use same vars in both branches)
            if (specification->nfRequest != LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE and not con_bool) {

                auto spec_intern = *specification;
                spec_intern.nfRequest = LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE;
                seenConsts = 0;
                seenAddends.clear();

                for (auto it = exp->iterator(); !it.end(); ++it) {
                    if (not is_linear(*it, spec_intern)) {
                        isNf = false;
                        break;
                    }
                }

            }

            return;

        }

        default: { PLAJA_ABORT }
    }

    PLAJA_ABORT
}

//

bool LinearConstraintsChecker::is_factor(const Expression& expr) { return is_scalar(expr) and not PLAJA_FLOATS::is_zero(expr.evaluate_floating_const()); }

bool LinearConstraintsChecker::is_linear(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification, LinearType type) {
    PLAJA_ASSERT(type != NONE)
    PLAJA_ASSERT(type != SUM or not specification.asPredicate)
    LinearConstraintsChecker checker(type, specification);
    expr.accept(&checker);

    if (not checker.is(checker.type, expr, type)) { return false; }

    if (specification.asPredicate and type == CONDITION) { // special handling for preds
        if (not checker.is(checker.type, expr, CONSTRAINT) and not checker.is(checker.type, expr, CON_BOOL) and not checker.is(checker.type, expr, DIS_BOOL)) { return false; }
    }

    if (specification.needsAddends and checker.type == SUM and not checker.has_addend()) { return false; } // special handling for sum

    // normal form request: is the linear constraint requested to be in normal form (or even requested to be non-normal)?
    switch (specification.nfRequest) {
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::NONE: { return true; }
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::TRUE: { return checker.is_nf(); }
        case LINEAR_CONSTRAINTS_CHECKER::NFRequest::FALSE: { return not checker.is_nf(); }
    }

    PLAJA_ABORT
}
