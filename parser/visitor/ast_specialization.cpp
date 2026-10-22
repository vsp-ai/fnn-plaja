//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_specialization.h"
#include "../ast/expression/special_cases/linear_expression.h"
#include "../ast/expression/special_cases/nary_expression.h"
#include "../ast/expression/unary_op_expression.h"
#include "../ast/model.h"
#include "extern/visitor_base.h"
#include "extern/to_linear_expression.h"
#include "linear_constraints_checker.h"
#include "to_normalform.h"

AstSpecialization::AstSpecialization():
    doToNary(true) {
}

AstSpecialization::~AstSpecialization() = default;

/**********************************************************************************************************************/

void AstSpecialization::specialize_ast(std::unique_ptr<AstElement>& ast_elem) {
    AstSpecialization ast_opt;
    ast_elem->accept(&ast_opt);
    if (ast_opt.replace_by_result) { ast_elem = ast_opt.move_result<AstElement>(); }
}

void AstSpecialization::specialize_ast(Model& model) {
    AstSpecialization ast_opt;
    model.accept(&ast_opt);
    PLAJA_ASSERT(not ast_opt.replace_by_result)
}

/**********************************************************************************************************************/

void AstSpecialization::visit(BinaryOpExpression* expr) {
    if (TO_LINEAR_EXP::as_linear_exp(*expr)) { set_to_replace(TO_LINEAR_EXP::to_exp(*expr)); }
    else {

        /* If constraint/sum contains an array, then as_linear_expr fails, though substructures may be transformable -> we do not want that. */
        const LINEAR_CONSTRAINTS_CHECKER::Specification specification;
        if (LinearConstraintsChecker::is_linear_constraint(*expr, specification) or LinearConstraintsChecker::is_linear_sum(*expr, specification)) {
            return;
        }

        if (NaryExpression::is_op(expr->get_op()) and doToNary) {
            doToNary = false;

            AstVisitor::visit(expr); // First perform other specializations ...

            auto expr_to_nary = expr->move_exp();
            TO_NORMALFORM::to_nary(expr_to_nary);
            set_to_replace(std::move(expr_to_nary));

            doToNary = true;
        } else {
            AstVisitor::visit(expr);
        }

    }
}

void AstSpecialization::visit(UnaryOpExpression* expr) {
    if (TO_LINEAR_EXP::as_linear_exp(*expr)) { set_to_replace(TO_LINEAR_EXP::to_exp(*expr)); }
    else {
        const LINEAR_CONSTRAINTS_CHECKER::Specification specification;
        if (not(LinearConstraintsChecker::is_linear_constraint(*expr, specification) or LinearConstraintsChecker::is_linear_sum(*expr, specification))) {
            AstVisitor::visit(expr);
        }
    }
}

void AstSpecialization::visit(NaryExpression* expr) { // It may still be possible that subexpressions are nary-expressions themselves ...
    if (doToNary) {
        doToNary = false;

        AstVisitor::visit(expr); // First perform other specializations ...

        auto expr_to_nary = expr->move_exp();
        TO_NORMALFORM::to_nary(expr_to_nary);
        set_to_replace(std::move(expr_to_nary));

        doToNary = true;
    } else {
        AstVisitor::visit(expr);
    }
}

/**********************************************************************************************************************/

namespace AST_SPECIALIZATION {

    void specialize_ast(std::unique_ptr<AstElement>& ast_elem) { AstSpecialization::specialize_ast(ast_elem); }

    std::unique_ptr<AstElement> specialize_ast(const AstElement& ast_elem) {
        auto copy = ast_elem.deep_copy_ast();
        specialize_ast(copy);
        return copy;
    }

    void specialize_ast(Model& model) {
        AstSpecialization::specialize_ast(model);
    }

}