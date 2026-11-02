//
// This file is part of PlaJA.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_ENTAILMENTS_H
#define PLAJA_PA_ENTAILMENTS_H

#include <list>
#include "model/ast/expression/forward_expression.h"
#include "core/utils/default_constructors.h"
#include "core/stats/forward_stats.h"
#include "core/forward_factories.h"
#include "engines/verification/optimization/forward_optimization_pa.h"
#include "engines/verification/smt/forward_smt_pa.h"
#include "engines/verification/using_predicate_abstraction.h"

class PaEntailments final {

private:
    const Expression* condition;
    const ModelZ3PA* modelPa;
    PLAJA::StatsBase* stats;

    PredicateIndex_type maxPredIndex; // For incremental usage.
    std::list<std::pair<PredicateIndex_type, bool>> entailments;

public:
    PaEntailments(const PLAJA::Configuration& config, const ModelZ3PA& model_pa, const Expression* condition);
    virtual ~PaEntailments();
    DELETE_CONSTRUCTOR(PaEntailments)

    [[nodiscard]] inline const ModelZ3PA& get_model_pa() const { return *modelPa; }

    [[nodiscard]] inline bool has_stats() const { return stats; }

    [[nodiscard]] inline const std::list<std::pair<PredicateIndex_type, bool>>& get_entailments() const { return entailments; }

    void increment();

};

#endif //PLAJA_PA_ENTAILMENTS_H
