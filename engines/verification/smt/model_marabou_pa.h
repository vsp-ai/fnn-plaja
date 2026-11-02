//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_MARABOU_PA_H
#define PLAJA_MODEL_MARABOU_PA_H

#include <memory>
#include <vector>
#include "include/ct_config_const.h"
#include "model/ast/expression/forward_expression.h"
#include "marabou/model/model_marabou.h"
#include "marabou/forward_marabou_in_plaja.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "successor_generation/forward_successor_generation.h"
#include "engines/verification/using_predicate_abstraction.h"
#include "engines/verification/pa_states/forward_pa_states.h"
#include "engines/verification/smt/forward_marabou_pa.h"

class ModelMarabouPA final: public ModelMarabou {

private:
    std::vector<std::vector<std::unique_ptr<PredicateConstraint>>> predicatesInMarabouPerStep;

    [[nodiscard]] const std::vector<std::unique_ptr<PredicateConstraint>>& get_predicates_in_marabou(StepIndex_type step) const {
        PLAJA_ASSERT(step < predicatesInMarabouPerStep.size())
        return predicatesInMarabouPerStep[step];
    }

    void step_predicates(bool reset);

public:
    explicit ModelMarabouPA(const PLAJA::Configuration& config, const PredicatesExpression* predicates_ = nullptr);
    ~ModelMarabouPA() override;
    DELETE_CONSTRUCTOR(ModelMarabouPA)

    void generate_steps(StepIndex_type max_step) override;

    void increment();

    [[nodiscard]] inline std::size_t get_num_predicates_marabou() const { return get_predicates_in_marabou(0).size(); }

    [[nodiscard]] inline const PredicateConstraint* get_predicate_in_marabou(PredicateIndex_type pred_index, StepIndex_type step = 0) const {
        PLAJA_ASSERT(pred_index < get_predicates_in_marabou(step).size())
        return get_predicates_in_marabou(step)[pred_index].get();
    }

    [[nodiscard]] const ActionOpMarabouPA& get_action_op_pa(ActionOpID_type op_id) const;

    void add_to(const PaStateBase& pa_state, MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

};

namespace MODEL_MARABOU_PA { extern void make_sharable(const PLAJA::Configuration& config); } // to hide internals of ModelZ3PA in main CEGAR


#endif //PLAJA_MODEL_MARABOU_PA_H
