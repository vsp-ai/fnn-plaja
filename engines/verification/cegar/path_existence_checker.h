//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PATH_EXISTENCE_CHECKER_H
#define PLAJA_PATH_EXISTENCE_CHECKER_H

#include <list>
#include <memory>
#include <vector>
#include "include/ct_config_const.h"
#include "model/ast/expression/forward_expression.h"
#include "core/forward_factories.h"
#include "core/search_engine.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "smt/base/forward_solution_check.h"
#include "smt/forward_smt_z3.h"
#include "smt/using_smt.h"
#include "marabou/forward_marabou_in_plaja.h"
#include "core/forward_states.h"
#include "engines/verification/smt/forward_marabou_pa.h"
#include "engines/verification/smt/forward_smt_pa.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/cegar/forward_cegar.h"
#include "policies/interface/jani_2_interface.h"
#include "engines/verification/solution_checker_pa.h"
#include "engines/verification/solution_checker_instance_pa.h"

class SolutionCheckerInstancePaPath;

class PathExistenceChecker final {

private:
    const PredicatesStructure* predicates;
    std::unique_ptr<SolutionCheckerPa> solutionChecker;

    std::shared_ptr<ModelZ3PA> modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;

    std::shared_ptr<ModelMarabouPA> modelMarabou;
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solverMarabou;

/* Config. */
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    bool paStateIsTerminalExact; // Abstract state either contains terminal or non-terminal states.
    [[nodiscard]] inline bool check_terminal(bool pa_state_aware) const { return PLAJA_GLOBAL::enableTerminalStateSupport and not(pa_state_aware and paStateIsTerminalExact); }

#else

    [[nodiscard]] inline constexpr bool check_terminal(bool) const { return false; }

#endif

    /* Private aux. */
    [[nodiscard]] bool do_locs() const;
    [[nodiscard]] Z3_IN_PLAJA::Context& get_context() const;

    /** Sanity check in debug model. */
    [[nodiscard]] bool check_entailed_non_terminal(Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step) const;

    void encode_start(Z3_IN_PLAJA::SMTSolver& solver, const SolutionCheckerInstancePaPath& problem) const;
    void init_query(const SolutionCheckerInstancePaPath& problem);

    void add_predicates_aux(SpuriousnessResult& result, StepIndex_type step, const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver) const;
    bool add_predicates(SpuriousnessResult& result, StepIndex_type step, const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver, bool pa_state_aware);
    bool add_terminal(SpuriousnessResult& result, StepIndex_type step, Z3_IN_PLAJA::SMTSolver& solver);
    static void add_guards(SpuriousnessResult& result, StepIndex_type step, ToZ3ExprSplits& splits, Z3_IN_PLAJA::SMTSolver& solver);

    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_policy_incrementally_fnn(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem, const class SelectionRefinement& selection_refinement);
    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_policy_incrementally_nn(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem, const class SelectionRefinement& selection_refinement);

public:
    explicit PathExistenceChecker(const PLAJA::Configuration& configuration, const PredicatesStructure& predicates);
    ~PathExistenceChecker();
    DELETE_CONSTRUCTOR(PathExistenceChecker)

    [[nodiscard]] inline const SolutionCheckerPa& get_solution_checker() const { return *solutionChecker; }

    [[nodiscard]] const Expression* get_start() const;
    [[nodiscard]] const Expression* get_reach() const;
    [[nodiscard]] const class Jani2Interface* get_interface() const;
    [[nodiscard]] bool has_interface() const;
    [[nodiscard]] bool has_nn() const;
    [[nodiscard]] bool has_fnn() const;


    void set_interface(const class Jani2Interface* interface);
    void set_start(const Expression* start);
    void set_reach(const Expression* reach);
    [[nodiscard]] const Expression* get_non_terminal() const;

    void encode_path(Z3_IN_PLAJA::SMTSolver& solver, const SolutionCheckerInstancePaPath& problem) const;
    void encode_path(MARABOU_IN_PLAJA::SMTSolver& solver, const SolutionCheckerInstancePaPath& problem) const;

    [[nodiscard]] bool check_start(const AbstractState& abstract_state, bool include_model_init);
    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_incrementally(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem);

    /* Applicable if path is non-spuriousness under standard PA. */
    [[nodiscard]] std::unique_ptr<SpuriousnessResult> check_policy_incrementally(std::unique_ptr<SolutionCheckerInstancePaPath>&& problem, const class SelectionRefinement& selection_refinement);

    FCT_IF_DEBUG(void dump_solution(std::unique_ptr<SolutionCheckerInstance>&& solution_checker_instance) const;)

};

#endif //PLAJA_PATH_EXISTENCE_CHECKER_H
