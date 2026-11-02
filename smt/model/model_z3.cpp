//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt/model/model_z3.h"
#include "core/exception/not_implemented_exception.h"
#include "model/ast/assignment.h"
#include "model/ast/automaton.h"
#include "model/ast/destination.h"
#include "model/ast/edge.h"
#include "model/ast/expression/lvalue_expression.h"
#include "model/ast/expression/non_standard/state_values_expression.h"
#include "model/ast/expression/non_standard/states_values_expression.h"
#include "model/ast/model.h"
#include "model/visitor/to_normalform.h"
#include "model/visitor/variables_of.h"
#include "core/configuration.h"
#include "smt/smt_options.h"
#include "policies/fnn/jani_2_fnn.h"
#include "model/model_information.h"
#include "model/ast/expression/non_standard/predicates_expression.h"
#include "engines/verification/successor_generation/action_op/action_op_abstract.h"
#include "smt/successor_generation/successor_generator_to_z3.h"
#include "core/state_values.h"
#include "smt/constraint_z3.h"
#include "smt/context_z3.h"
#include "smt/policy/output_constraints_in_z3.h"
#include "smt/solver/smt_solver_z3.h"
#include "smt/utils/to_z3_expr.h"
#include "smt/vars_in_z3.h"
#include "smt/visitor/extern/to_z3_visitor.h"
#include "smt/visitor/smt_vars_of.h"
#include <z3++.h>

// external:
namespace SUCCESSOR_GENERATOR_ABSTRACT { [[nodiscard]] extern std::list<const ActionOpAbstract*> extract_ops_per_label(const SuccessorGeneratorAbstract& successor_generator, ActionLabel_type action_label); }

/**********************************************************************************************************************/


ModelZ3::ModelZ3(const PLAJA::Configuration& config):
    ModelSmt(std::shared_ptr<PLAJA::SmtContext>(new Z3_IN_PLAJA::Context()), config)
    , context(PLAJA_UTILS::cast_ptr<Z3_IN_PLAJA::Context>(get_context_ptr().get()))
    , fnnSupport(has_fnn())
    , successorGenerationToZ3(new SuccessorGeneratorToZ3(*this)) {
    set_start(std::move(start));
    set_reach(std::move(reach));

    /* Array constants? */
    PLAJA_ASSERT(context->get_number_of_variables() == 0)
    for (const ConstantIdType id: modelInfo->get_constants()) {
        const auto z3_var = Z3_IN_PLAJA::StateVarsInZ3::add(*model->get_constant(id), get_context());
        constants.emplace(id, z3_var);
    };

    generate_steps(maxStep);
    if (fnnSupport) { outputConstraints = Z3_IN_PLAJA::OutputConstraints::construct(*this); }

}

ModelZ3::~ModelZ3() = default;

/**********************************************************************************************************************/

std::shared_ptr<Z3_IN_PLAJA::Context> ModelZ3::share_context() const { return PLAJA_UTILS::cast_shared<Z3_IN_PLAJA::Context>(get_context_ptr()); }

z3::context& ModelZ3::get_context_z3() const { return get_context()(); }

/**********************************************************************************************************************/

void ModelZ3::set_start(std::unique_ptr<Expression>&& start_ptr) {
    ModelSmt::set_start(std::move(start_ptr));
    startZ3 = nullptr;
    startZ3WithInit = nullptr;
}

void ModelZ3::set_reach(std::unique_ptr<Expression>&& reach_ptr) {
    ModelSmt::set_reach(std::move(reach_ptr));
    reachZ3.clear();
}

bool ModelZ3::initial_state_is_subsumed_by_start() const { return not startZ3WithInit; }

/**********************************************************************************************************************/

[[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> ModelZ3::generate_state_variables(StepIndex_type step_index) const {

    std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> state_vars(new Z3_IN_PLAJA::StateVarsInZ3(*this));

    const std::string postfix = PLAJA_UTILS::underscoreString + std::to_string(step_index);

    if (not ignore_locs()) {

        for (auto it_loc = modelInfo->locationIndexIterator(); !it_loc.end(); ++it_loc) { state_vars->add_loc(it_loc.state_index(), postfix); }

    } else if (fnnSupport) {

        for (auto it_loc = modelInfo->locationIndexIterator(); !it_loc.end(); ++it_loc) {
            if (std::any_of(inputLocations.cbegin(), inputLocations.cend(), [&it_loc](VariableIndex_type loc_input) { return it_loc.state_index() == loc_input; })) {
                state_vars->add_loc(it_loc.state_index(), postfix);
            }
        }

    }

    for (auto it_var = model->variableIterator(); !it_var.end(); ++it_var) {
        state_vars->add(it_var.variable_id(), postfix);
    }

    return state_vars;
}

/**********************************************************************************************************************/

z3::expr ModelZ3::compute_assignment(const Assignment& assignment, const Z3_IN_PLAJA::StateVarsInZ3& src_vars, const Z3_IN_PLAJA::StateVarsInZ3& target_vars, std::unordered_map<VariableID_type, std::vector<z3::expr>>* updated_array_indexes) const {
    PLAJA_ASSERT(assignment.get_index() == 0) // we do not support sequences

    // Cache assigned variable value.
    const VariableID_type var_id = assignment.get_updated_var_id();
    const auto* index = assignment.get_array_index();

    ToZ3Expr var_ref(get_context()); // variable reference expression

    // check for array
    if (index) { // array assignment
        auto index_z3 = Z3_IN_PLAJA::to_z3(*index, src_vars);

        // Cache array index.
        if (updated_array_indexes) {
            auto it = updated_array_indexes->find(var_id);
            if (it == updated_array_indexes->end()) { updated_array_indexes->emplace(var_id, std::vector<z3::expr> { index_z3.primary }); }
            else { (*updated_array_indexes)[var_id].push_back(index_z3.primary); }
        }

        // bounds
        if (index->is_constant()) { // constant indexes are statistically checked to be within-bounds
            PLAJA_ASSERT(0 <= index->evaluate_integer_const() && index->evaluate_integer_const() <= get_array_size(var_id))
        } else { // add bounds on index without removing "inner bounds"
            index_z3.conjunct_auxiliary(0 <= index_z3.primary && index_z3.primary < context->int_val(PLAJA_UTILS::cast_numeric<PLAJA::integer>(get_array_size(var_id)))); // add bounds on index
        }

        // finally set var_ref.first to actual access expression:
        var_ref.set(z3::select(target_vars.to_z3_expr(var_id), index_z3.primary));
        if (index_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(index_z3.release_auxiliary()); }

    } else { // non-array assignment

        var_ref.set(target_vars.to_z3_expr(var_id));

    }

    // assigned value expression
    if (assignment.is_non_deterministic()) {

        PLAJA_ASSERT(assignment.get_lower_bound())
        PLAJA_ASSERT(assignment.get_upper_bound())

        // Encode lower <= x' <= upper (same semantics as the PB encoder and the Marabou encoding).
        auto var = var_ref.primary;

        ToZ3Expr lb_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_lower_bound(), src_vars);
        ToZ3Expr ub_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_upper_bound(), src_vars);
        var_ref.set(var >= lb_z3.release() && var <= ub_z3.release());
        if (lb_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(lb_z3.release_auxiliary()); }
        if (ub_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(ub_z3.release_auxiliary()); }

    } else {

        PLAJA_ASSERT(assignment.get_value())
        ToZ3Expr val_z3 = Z3_IN_PLAJA::to_z3(*assignment.get_value(), src_vars);

        var_ref.set(var_ref.primary == val_z3.primary);
        if (val_z3.has_auxiliary()) { var_ref.conjunct_auxiliary(val_z3.release_auxiliary()); }

    }

    return var_ref.to_conjunction();
}

ToZ3Expr ModelZ3::to_z3(const Expression& expr, StepIndex_type step_index) const { return Z3_IN_PLAJA::to_z3(expr, get_state_vars(step_index)); }

z3::expr ModelZ3::to_z3_condition(const Expression& expr, StepIndex_type step_index) const { return Z3_IN_PLAJA::to_z3_condition(expr, get_state_vars(step_index)); }

void ModelZ3::add_expression(Z3_IN_PLAJA::SMTSolver& solver, const Expression& condition, StepIndex_type step_index) const { solver.add(to_z3_condition(condition, step_index)); }

/**********************************************************************************************************************/

// TODO might utilize z3::expr::substitute instead of recomputing structure for each step.
//  However, at least in terms of time consumption no issue, maybe beneficial to z3 heuristics or memory?
void ModelZ3::generate_steps(StepIndex_type max_step) {
    PLAJA_ASSERT(variablesPerStep.empty() or variablesPerStep.size() == get_max_num_step())
    PLAJA_ASSERT(fnnInZ3PerStep.empty() or fnnInZ3PerStep.size() == get_max_path_len())

    maxStep = std::max(max_step, maxStep);

    /* Vars (one additional "layer" to account for "target"). */

    variablesPerStep.reserve(get_max_num_step());

    for (auto it = iterate_steps(variablesPerStep.size()); !it.end(); ++it) {
        PLAJA_ASSERT(it.step() == variablesPerStep.size())
        variablesPerStep.push_back(generate_state_variables(it.step()));
    }

    PLAJA_ASSERT(variablesPerStep.size() == get_max_num_step())

    // policy (FNN):
    if (fnnSupport) {
        PLAJA_ASSERT(interface)
        auto fnnInterface = dynamic_cast<const Jani2FNN*>(interface);
        PLAJA_ASSERT(fnnInterface)
        fnnInZ3PerStep.reserve(get_max_path_len());

        for (auto it = iterate_path_from_to(fnnInZ3PerStep.size(), get_max_path_len()); !it.end(); ++it) {
            fnnInZ3PerStep.push_back(std::make_unique<FNNInZ3>(*fnnInterface, *context, get_state_vars(it.step())));
        }

        PLAJA_ASSERT(fnnInZ3PerStep.size() == get_max_path_len())
    }

    // start and reach:
    if (not startZ3) {
        if (start) {
            PLAJA_ASSERT(not startZ3WithInit)
            const auto* states_values = dynamic_cast<const StatesValuesExpression*>(start.get());
            if (states_values) { // Disjunction over the start states.
                std::vector<z3::expr> states;
                for (auto it = states_values->init_state_values_iterator(); !it.end(); ++it) {
                    states.push_back(get_state_vars(0).encode_state(it->construct_state_values(modelInfo->get_initial_values()), not ignore_locs()));
                }
                startZ3 = std::make_unique<z3::expr>(Z3_IN_PLAJA::to_disjunction(states));
            } else {
                startZ3 = std::make_unique<z3::expr>(to_z3_condition(*start, 0));
            }
            if (not ModelSmt::initial_state_is_subsumed_by_start()) { // Only set if model's initial state is not subsumed.
                startZ3WithInit = std::make_unique<z3::expr>(get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()) || *startZ3);
            }
        } else if (not startZ3WithInit) {
            startZ3WithInit = std::make_unique<z3::expr>(get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()));
        }
    }

    if (reach) {
        reachZ3.reserve(get_max_num_step());
        for (auto it = iterate_steps(reachZ3.size()); !it.end(); ++it) { reachZ3.push_back(std::make_unique<z3::expr>(to_z3_condition(*reach, it.step()))); }
    }
    PLAJA_ASSERT((not reach and reachZ3.empty()) or reachZ3.size() == get_max_num_step())

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    const auto* non_terminal = get_non_terminal();
    if (non_terminal) {
        nonTerminalSmt.reserve(get_max_num_step());
        for (auto it = iterate_steps(nonTerminalSmt.size()); !it.end(); ++it) { nonTerminalSmt.push_back(std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(to_z3_condition(*non_terminal, it.step()))); }
    }
    PLAJA_ASSERT((not non_terminal and nonTerminalSmt.empty()) or nonTerminalSmt.size() == get_max_num_step())
#endif

    if (outputConstraints) { outputConstraints->generate_steps(); }

}

const z3::expr& ModelZ3::get_var(VariableID_type var, StepIndex_type step) const { return get_state_vars(step).to_z3_expr(var); }

void ModelZ3::register_constant(Z3_IN_PLAJA::SMTSolver& solver, ConstantIdType constant) const {
    const auto constant_z3 = get_constant_var(constant);
    solver.register_bound_if(constant_z3, context->get_bound(constant_z3));
}

void ModelZ3::register_bound(Z3_IN_PLAJA::SMTSolver& solver, VariableID_type var, StepIndex_type step) const { // NOLINT(bugprone-easily-swappable-parameters)
    const auto& state_vars = get_state_vars(step);
    const auto var_z3 = state_vars.to_z3(var);
    const auto* bound = context->get_bound_if(var_z3);
    if (not bound) { return; }
    solver.register_bound_if(var_z3, *bound);
}

void ModelZ3::register_bounds(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step, bool ignore_locs) const {

    /* Always register constants */
    for (const auto constant: constants) {
        const auto* bound = context->get_bound_if(constant.second);
        if (bound) { solver.register_bound_if(constant.second, *bound); }
    }

    const auto state_vars = get_state_vars(step);

    if (not ignore_locs and not ignoreLocations) {
        for (auto it_loc = state_vars.locIterator(); !it_loc.end(); ++it_loc) {
            solver.register_bound_if(it_loc.loc_z3(), context->get_bound(it_loc.loc_z3()));
        }
    }

    for (auto it_var = state_vars.varIterator(); !it_var.end(); ++it_var) {
        const auto var_z3 = it_var.var_z3();
        const auto* bound = context->get_bound_if(var_z3);
        if (bound) { solver.register_bound_if(var_z3, *bound); }
    }

}

void ModelZ3::register_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Expression& expr, StepIndex_type step) const {
    for (const auto var: Z3_IN_PLAJA::collect_smt_vars(expr, get_state_vars(step))) {
        const auto* bound = context->get_bound_if(var);
        if (bound) { solver.register_bound_if(var, *bound); }
    }
}

void ModelZ3::register_src_input_bounds(Z3_IN_PLAJA::SMTSolver& solver, const Jani2Interface& policy_interface) const {
    for (auto it = policy_interface.init_input_feature_iterator(true); !it.end(); ++it) {
        if (not it.get_input_feature()->is_location()) { register_src_bound(solver, it.get_input_feature()->get_variable_id()); }
    }
    if (_predicates()) {
        for (auto pred_it = _predicates()->predicatesIterator(); !pred_it.end(); ++pred_it) { register_src_bounds(solver, *pred_it); }
    }
}

void ModelZ3::add_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init) const {
    PLAJA_ASSERT(startZ3WithInit or ModelSmt::initial_state_is_subsumed_by_start())
    if (include_init) { solver.add(initial_state_is_subsumed_by_start() ? *startZ3 : *startZ3WithInit); }
    else if (start) {
        PLAJA_ASSERT(startZ3)
        solver.add(*startZ3);
    }
}

void ModelZ3::add_init(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const { solver.add(get_state_vars(step).encode_state(modelInfo->get_initial_values(), not ignore_locs())); }

void ModelZ3::exclude_start(Z3_IN_PLAJA::SMTSolver& solver, bool include_init, StepIndex_type step) const {

    if (step == 0) {
        if (include_init) { solver.add(!(initial_state_is_subsumed_by_start() ? *startZ3 : *startZ3WithInit)); }
        else {
            if (start) { solver.add(!*startZ3); }
            else { solver.add(context->bool_val(false)); }
        }
    }

    if (include_init) {
        solver.add(!get_state_vars(0).encode_state(modelInfo->get_initial_values(), not ignore_locs()));
    }

    if (start) { solver.add(!to_z3_condition(*start, step)); }
    else if (not include_init) { solver.add(context->bool_val(false)); }

}

void ModelZ3::add_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    if (not reach) { solver.add(context->bool_val(false)); return; }
    PLAJA_ASSERT(step < reachZ3.size())
    solver.add(*reachZ3[step]);
}

void ModelZ3::exclude_reach(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    if (reach) {
        PLAJA_ASSERT(step < reachZ3.size())
        solver.add(!*reachZ3[step]);
    }
}

void ModelZ3::add_fnn(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const {
    PLAJA_ASSERT(has_fnn())
    PLAJA_ASSERT(step < fnnInZ3PerStep.size())
    fnnInZ3PerStep[step]->add(solver);
}

std::size_t ModelZ3::get_output_layer_size(StepIndex_type step) const {
    if (has_fnn()) { return get_fnn_in_smt(step).get_output_layer_size(); }
    PLAJA_ABORT
}

const z3::expr& ModelZ3::get_output_var_z3(OutputIndex_type output_index, StepIndex_type step) const {
    if (has_fnn()) { return get_fnn_in_smt(step).get_output_var_z3(output_index); }
    PLAJA_ABORT
}

void ModelZ3::add_output_interface(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { // NOLINT(bugprone-easily-swappable-parameters)
    PLAJA_ASSERT(interface)
    outputConstraints->add(solver, action_label, step);
}

void ModelZ3::add_output_interface_for_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type action_op, StepIndex_type step) const {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { set_self_applicability(true); }
    add_output_interface(solver, successorGenerationToZ3->get_action_op_structure(action_op)._action_label(), step);
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { unset_self_applicability(); }
}

#ifdef BUILD_PA

void ModelZ3::share_action_ops(const SuccessorGeneratorAbstract& successor_generator) {
    if (not interface) { return; } // so far only needed for policy checking

    // TODO cache action ops in z3 (preliminary quick fix)
    for (auto output_it = interface->init_output_feature_iterator(); !output_it.end(); ++output_it) {

        auto& action_ops_for_label = actionOpsPerLabel.emplace(output_it.get_output_label(), SUCCESSOR_GENERATOR_ABSTRACT::extract_ops_per_label(successor_generator, output_it.get_output_label())).first->second;
        for (const auto* action_op_z3: action_ops_for_label) {
            actionOps.emplace(action_op_z3->_op_id(), action_op_z3);
        }

    }
}

#endif

/* */

const ActionOpToZ3& ModelZ3::get_action_op_structure(ActionOpID_type op) const { return successorGenerationToZ3->get_action_op_structure(op); }

void ModelZ3::add_guard(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, bool do_locs, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.guard_to_z3(get_state_vars(step), do_locs)->add_to_solver(solver);
}

void ModelZ3::add_update(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.update_to_z3(update_index, get_state_vars(step), get_state_vars(step + 1), do_locs, do_copy)->add_to_solver(solver);
}

void ModelZ3::add_action_op(Z3_IN_PLAJA::SMTSolver& solver, ActionOpID_type op, UpdateIndex_type update_index, bool do_locs, bool do_copy, StepIndex_type step) const {
    const auto& action_op = successorGenerationToZ3->get_action_op_structure(op);
    action_op.to_z3(update_index, get_state_vars(step), get_state_vars(step + 1), do_locs, do_copy)->add_to_solver(solver);
}

std::unique_ptr<Z3_IN_PLAJA::Constraint> ModelZ3::guards_to_smt(ActionLabel_type action_label, StepIndex_type step) const { return successorGenerationToZ3->generate_guards(action_label, step); }

void ModelZ3::add_guard_disjunction(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const { guards_to_smt(action_label, step)->add_to_solver(solver); }

void ModelZ3::add_loop_exclusion(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step_index) const {
    if (step_index < 1) { return; }

    for (StepIndex_type step_index_1 = 0; step_index_1 <= step_index; ++step_index_1) {

        const auto& variables_per_step_1 = get_state_vars(step_index_1);

        for (StepIndex_type step_index_2 = 0; step_index_2 < step_index_1; ++step_index_2) {

            const auto& variables_per_step_2 = get_state_vars(step_index_2);

            std::vector<z3::expr> state_disjunction;
            state_disjunction.reserve(variables_per_step_1.size());

            // locations
            for (auto it_loc = variables_per_step_1.locIterator(); !it_loc.end(); ++it_loc) {
                state_disjunction.push_back(it_loc.loc_z3_expr() != variables_per_step_2.loc_to_z3_expr(it_loc.loc()));
            }

            for (auto it_var = variables_per_step_1.varIterator(); !it_var.end(); ++it_var) {
                state_disjunction.push_back(it_var.var_z3_expr() != variables_per_step_2.to_z3_expr(it_var.var()));
            }

            solver.add(Z3_IN_PLAJA::to_disjunction(state_disjunction));
        }

    }
}

/**********************************************************************************************************************/

std::unique_ptr<PLAJA::SmtSolver> ModelZ3::init_solver(const PLAJA::Configuration& config, StepIndex_type step) const {
    auto solver = std::make_unique<Z3_IN_PLAJA::SMTSolver>(share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS));
    if (step == SEARCH::noneStep) { return solver; }
    for (auto it = iterate_steps(step); !it.end(); ++it) { add_bounds(*solver, it.step(), ignore_locs()); }
    return solver;
}

std::unique_ptr<PLAJA::SmtConstraint> ModelZ3::to_smt(const Expression& expr, StepIndex_type step) const { return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(to_z3_condition(expr, step)); }

void ModelZ3::add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const { add_expression(PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver), expr, step); }
