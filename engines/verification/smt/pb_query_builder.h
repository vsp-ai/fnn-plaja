#ifndef PLAJA_FNN_PB_QUERY_BUILDER_H
#define PLAJA_FNN_PB_QUERY_BUILDER_H

#include "pseudo_boolean/pb_formula.h"
#include "core/forward_factories.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "engines/verification/using_predicate_abstraction.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class AbstractState;
class ActionOpPA;
class Jani2FNN;
class ModelZ3PA;

namespace FNN_PB {

enum class Granularity { Label, Operator };

struct QueryContext final {
    const Jani2FNN* jani2FNN = nullptr;
    const ModelZ3PA* modelZ3 = nullptr;
    const AbstractState* source = nullptr;
    const AbstractState* target = nullptr;
    const ActionOpPA* actionOp = nullptr;
    ActionLabel_type actionLabel = ACTION::noneLabel;
    ::OutputIndex_type outputIndex = 0;
    ActionOpID_type actionOpId = ACTION::noneOp;
    UpdateIndex_type updateIndex = ACTION::noneUpdate;
    bool requireNonTerminal = false;
    bool useOutputInterface = false;
    bool useGuard = false;
    bool useGuardNegation = false;
    bool useUpdate = false;
    bool useTargetState = false;
    bool useAppFiltering = false;
};

struct IncrementalFormula final {
    PB::Formula formula;
    std::vector<PB::Literal> currentPredicateLiterals;
    std::vector<PB::Literal> successorPredicateLiterals;
    std::unordered_map<ActionOpID_type, PB::Literal> activationLiterals;
    std::vector<ActionOpID_type> actionOperators;
};

[[nodiscard]] PB::Formula build_fresh_formula(const QueryContext& query);
[[nodiscard]] IncrementalFormula build_incremental_formula(const QueryContext& query, Granularity granularity);
[[nodiscard]] std::vector<PB::Literal> build_incremental_assumptions(
    const QueryContext& query, const IncrementalFormula& cached);
[[nodiscard]] std::string incremental_query_error(const QueryContext& query);
/** Integer state values (index order) encoded by the one-hot literals of a SAT model, for the current or successor copy. */
[[nodiscard]] std::vector<PLAJA::integer> decode_state(const QueryContext& query, const PB::Formula& formula,
                                                       const std::function<bool(PB::Literal)>& value, bool successor);

} // namespace FNN_PB

#endif
