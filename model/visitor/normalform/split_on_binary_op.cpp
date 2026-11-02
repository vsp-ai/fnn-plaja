//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "model/visitor/normalform/split_on_binary_op.h"
#include "core/utils/utils.h"
#include "model/ast/expression/non_standard/let_expression.h"
#include "model/ast/expression/non_standard/state_condition_expression.h"
#include "model/ast/expression/special_cases/nary_expression.h"
#include "model/visitor/ast_element_visitor_const.h"
#include "model/visitor/normalform/split_on_locs.h"

class IsSplitOnBinaryOp: private AstElementVisitorConst {

private:
    BinaryOpExpression::BinaryOp op;
    bool rlt; // NOLINT(*-use-default-member-init)

    void visit(const BinaryOpExpression* exp) override;
    void visit(const LetExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    explicit IsSplitOnBinaryOp(BinaryOpExpression::BinaryOp op);
public:
    ~IsSplitOnBinaryOp() override;
    DELETE_CONSTRUCTOR(IsSplitOnBinaryOp)

    [[nodiscard]] static bool is_split(const Expression& expr, BinaryOpExpression::BinaryOp op);

};

IsSplitOnBinaryOp::IsSplitOnBinaryOp(BinaryOpExpression::BinaryOp op):
    op(op)
    , rlt(false) {
}

IsSplitOnBinaryOp::~IsSplitOnBinaryOp() = default;

void IsSplitOnBinaryOp::visit(const BinaryOpExpression* exp) { rlt = exp->get_op() == op; }

void IsSplitOnBinaryOp::visit(const LetExpression* exp) {
    if (exp->get_number_of_free_variables() == 0) { exp->get_expression()->accept(this); }
}

void IsSplitOnBinaryOp::visit(const StateConditionExpression* exp) { rlt = op == BinaryOpExpression::AND and exp->get_constraint(); }

void IsSplitOnBinaryOp::visit(const NaryExpression* exp) { rlt = exp->get_op() == op; }

bool IsSplitOnBinaryOp::is_split(const Expression& expr, BinaryOpExpression::BinaryOp op) {
    IsSplitOnBinaryOp is_split_on_binary_op(op);
    expr.accept(&is_split_on_binary_op);
    return is_split_on_binary_op.rlt;
}

/**********************************************************************************************************************/

SplitOnBinaryOpBase::SplitOnBinaryOpBase(BinaryOpExpression::BinaryOp op):
    op(op) {
}

SplitOnBinaryOpBase::~SplitOnBinaryOpBase() = default;

bool SplitOnBinaryOpBase::is_split(const Expression& expr) const { return IsSplitOnBinaryOp::is_split(expr, op); }

bool SplitOnBinaryOpBase::is_split(const Expression& expr, BinaryOpExpression::BinaryOp op) { return IsSplitOnBinaryOp::is_split(expr, op); }

/**********************************************************************************************************************/

SplitOnBinaryOp::SplitOnBinaryOp(BinaryOpExpression::BinaryOp op):
    SplitOnBinaryOpBase(op) {
}

SplitOnBinaryOp::~SplitOnBinaryOp() = default;

/* */

void SplitOnBinaryOp::visit(BinaryOpExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    // left
    if (is_split(*exp->get_left())) { exp->get_left()->accept(this); }
    else { splitExpressions.push_back(exp->set_left()); }

    // right
    if (is_split(*exp->get_right())) { exp->get_right()->accept(this); }
    else { splitExpressions.push_back(exp->set_right()); }

}

void SplitOnBinaryOp::visit(LetExpression* exp) {
    PLAJA_ASSERT(exp->get_expression())
    PLAJA_ASSERT(is_split(*exp))
    PLAJA_ASSERT(is_split(*exp->get_expression()))
    exp->get_expression()->accept(this);
}

void SplitOnBinaryOp::visit(StateConditionExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    auto state_var_constraint = exp->set_constraint(nullptr);

    if (exp->get_size_loc_values() > 0) { splitExpressions.push_back(exp->deepCopy_Exp()); }

    if (state_var_constraint) {

        if (is_split(*state_var_constraint)) { state_var_constraint->accept(this); }
        else { splitExpressions.push_back(std::move(state_var_constraint)); }

    }

}

void SplitOnBinaryOp::visit(NaryExpression* exp) {
    PLAJA_ASSERT(is_split(*exp))

    for (auto it = exp->iterator(); !it.end(); ++it) {
        if (is_split(*it)) { it->accept(this); }
        else { splitExpressions.push_back(it.set()); }
    }

}

std::list<std::unique_ptr<Expression>> SplitOnBinaryOp::split(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op) {
    SplitOnBinaryOp split_on_binary_op(op);

    if (split_on_binary_op.is_split(*expr)) { expr->accept(&split_on_binary_op); }
    else { split_on_binary_op.splitExpressions.push_back(std::move(expr)); }

    return std::move(split_on_binary_op.splitExpressions);
}

std::list<std::unique_ptr<Expression>> SplitOnBinaryOp::split(const Expression& expr, BinaryOpExpression::BinaryOp op) { return split(expr.deepCopy_Exp(), op); }

std::unique_ptr<Expression> SplitOnBinaryOp::to_expr(std::list<std::unique_ptr<Expression>>&& splits, BinaryOpExpression::BinaryOp op) { // NOLINT(*-no-recursion)
    /* Special case. */
    if (splits.size() <= 2) { return ConstructBinaryOp::construct(op, std::move(splits)); }

    std::unique_ptr<StateConditionExpression> state_condition = op == BinaryOpExpression::AND ? ExtractOnLocs::extract(splits) : nullptr;
    PLAJA_ASSERT(not state_condition or not state_condition->get_constraint()) // Var constraint moved to splits.

    auto nary = std::make_unique<NaryExpression>(op);
    nary->reserve(splits.size());
    for (auto& split: splits) { nary->add_sub(std::move(split)); }
    nary->determine_type();

    if (state_condition) {
        PLAJA_ASSERT(not state_condition->get_constraint())
        PLAJA_ASSERT(op == BinaryOpExpression::AND)
        state_condition->set_constraint(std::move(nary));
        return state_condition;
    }

    return nary;
}

std::unique_ptr<Expression> SplitOnBinaryOp::to_expr(std::unique_ptr<Expression>&& expr, BinaryOpExpression::BinaryOp op) { return to_expr(split(std::move(expr), op), op); }

/**********************************************************************************************************************/

ToNary::ToNary(BinaryOpExpression::BinaryOp op, bool recursive):
    SplitOnBinaryOpBase(op)
    , recursive(recursive) {
}

ToNary::~ToNary() = default;

/* */

inline void ToNary::process_split(Expression* expr) {
    PLAJA_ASSERT(is_split(*expr))
    auto splits = SplitOnBinaryOp::split(expr->move_exp(), get_op());
    PLAJA_ASSERT(not std::any_of(splits.cbegin(), splits.cend(), [this](const auto& split) { return this->is_split(*split); }))
    if (recursive) { for (auto& split: splits) { to_nary(split, get_op(), recursive); } }
    set_to_replace(SplitOnBinaryOp::to_expr(std::move(splits), get_op()));
}

/* */

void ToNary::visit(BinaryOpExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(LetExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(StateConditionExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

void ToNary::visit(NaryExpression* exp) {
    if (is_split(*exp)) { process_split(exp); }
    else if (recursive) { AstVisitor::visit(exp); }
}

/* */

void ToNary::to_nary(std::unique_ptr<Expression>& expr, BinaryOpExpression::BinaryOp op, bool recursive) {
    ToNary to_nary(op, recursive);
    expr->accept(&to_nary);
    if (to_nary.replace_by_result) { expr = to_nary.move_result<Expression>(); }
}

/**********************************************************************************************************************/

ConstructBinaryOp::ConstructBinaryOp(BinaryOpExpression::BinaryOp op):
    op(op) {
}

ConstructBinaryOp::~ConstructBinaryOp() = default;

/* */

std::unique_ptr<Expression> ConstructBinaryOp::merge(std::unique_ptr<Expression>&& left, std::unique_ptr<Expression>&& right) {

    // else { // default:

    std::unique_ptr<BinaryOpExpression> rlt_expr(new BinaryOpExpression(op));

    rlt_expr->set_left(std::move(left));
    rlt_expr->set_right(std::move(right));

    rlt_expr->determine_type();

    return rlt_expr;

    // }

}

/* */

std::unique_ptr<Expression> ConstructBinaryOp::construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>& sub_expressions) { // NOLINT(misc-no-recursion)

    if (sub_expressions.size() == 2) {

        return ConstructBinaryOp(op).merge(std::move(sub_expressions.front()), std::move(sub_expressions.back()));

    } else {

        auto left = std::move(sub_expressions.front());
        sub_expressions.pop_front();
        auto right = construct(op, sub_expressions);

        return ConstructBinaryOp(op).merge(std::move(left), std::move(right));

    }

}

std::unique_ptr<Expression> ConstructBinaryOp::construct(BinaryOpExpression::BinaryOp op, std::list<std::unique_ptr<Expression>>&& sub_expressions) { // NOLINT(*-no-recursion)

    PLAJA_ASSERT(not sub_expressions.empty());

    if (sub_expressions.size() == 1) { return std::move(sub_expressions.front()); }

    std::unique_ptr<StateConditionExpression> state_condition = op == BinaryOpExpression::AND ? ExtractOnLocs::extract(sub_expressions) : nullptr;
    if (state_condition) {
        PLAJA_ASSERT(not state_condition->get_constraint()) // if (state_condition->get_constraint()) { sub_expressions.push_back(state_condition->set_constraint()); } // Var constraint moved to splits.
        state_condition->set_constraint(construct(BinaryOpExpression::AND, std::move(sub_expressions)));
        return state_condition;
    }

    return construct(op, sub_expressions);
}
