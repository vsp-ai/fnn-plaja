#ifndef PLAJA_FNN_SAT_CHECKER_PB_H
#define PLAJA_FNN_SAT_CHECKER_PB_H

#include <memory>
#include <vector>
#include "../sat_checker.h"
#include "../pa_states/forward_pa_states.h"
#include "../smt/forward_smt_pa.h"

class ActionOpPA;
class Jani2FNN;
class OpApplicability;
namespace FNN_PB { class Engine; }

class FnnSatCheckerPB final : public SatChecker {
private:
    struct QueryFlags final {
        bool requireNonTerminal = false;
        bool useOutputInterface = false;
        bool useGuard = false;
        bool useGuardNegation = false;
        bool useUpdate = false;
        bool useTargetState = false;
        bool useAppFilter = false;
    };

    std::shared_ptr<ModelZ3PA> modelZ3;
    const Jani2FNN* jani2FNN;
    const ActionOpPA* actionOp;
    std::unique_ptr<FNN_PB::Engine> engine;
    QueryFlags flags;
    std::vector<QueryFlags> flagsStack;

    void save_solution_if(bool result) override;
    bool check_() override;

public:
    FnnSatCheckerPB(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);
    ~FnnSatCheckerPB() override;
    DELETE_CONSTRUCTOR(FnnSatCheckerPB)

    void set_op_applicability_cache(std::shared_ptr<OpApplicability>& op_app) override;
    void increment() override;
    void push() override;
    void pop() override;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    void add_non_terminal_condition() override;
#else
    inline void add_non_terminal_condition() { PLAJA_ABORT }
#endif

    void set_operator(ActionOpID_type action_op_id) override;
    void add_source_state(const AbstractState& source) override;
    bool add_guard() override;
    bool add_guard_negation() override;
    void add_update() override;
    void add_target_state(const AbstractState& target) override;
    bool add_output_interface() override;
    bool check_relaxed() override;
    bool has_solution_box() override { return false; }
};

#endif
