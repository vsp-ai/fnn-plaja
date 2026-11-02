#include "engines/verification/smt/pb_engine.h"
#include "pseudo_boolean/pb_ipasir_solver.h"
#include "core/stats/stats_base.h"
#include "engines/verification/pa_states/abstract_state.h"

#include <tuple>

struct FNN_PB::Engine::CacheKey final {
    ActionLabel_type label;
    ActionOpID_type operatorId;
    OutputIndex_type output;
    UpdateIndex_type update;
    std::size_t generation;
    std::size_t predicates;
    bool nonTerminal;
    bool applicabilityFiltering;

    bool operator<(const CacheKey& other) const {
        return std::tie(label, operatorId, output, update, generation, predicates,
                        nonTerminal, applicabilityFiltering)
               < std::tie(other.label, other.operatorId, other.output, other.update,
                          other.generation, other.predicates, other.nonTerminal,
                          other.applicabilityFiltering);
    }
};

struct FNN_PB::Engine::Entry final {
    IncrementalFormula query;
    std::unique_ptr<PB::IpasirSolver> solver;

    explicit Entry(IncrementalFormula query_)
        : query(std::move(query_)), solver(std::make_unique<PB::IpasirSolver>(query.formula)) {}
};

FNN_PB::Engine::Engine(bool use_incremental, Granularity granularity, PLAJA::StatsBase* statistics)
    : useIncremental(use_incremental)
    , granularity(granularity)
    , statistics(statistics) {}

FNN_PB::Engine::~Engine() = default;

bool FNN_PB::Engine::solve(const QueryContext& query) {
    PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_QUERIES);
    witnessSource.clear();
    witnessTarget.clear();
    if (not useIncremental) {
        PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_FRESH_QUERIES);
        PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_FORMULA_BUILDS);
        auto formula = build_fresh_formula(query);
        PB::IpasirSolver solver(formula);
        const bool result = solver.solve();
        if (result) { save_witness(query, formula, solver, query.useUpdate or query.useTargetState); }
        return result;
    }
    PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_INCREMENTAL_QUERIES);
    const CacheKey key {
        query.actionLabel,
        granularity == Granularity::Operator ? query.actionOpId : ACTION::noneOp,
        query.outputIndex,
        query.updateIndex,
        predicateGeneration,
        query.source ? query.source->get_size_predicates() : 0,
        query.requireNonTerminal,
        query.useAppFiltering
    };
    auto iterator = cache.find(key);
    if (iterator == cache.end()) {
        PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_FORMULA_BUILDS);
        auto entry = std::make_unique<Entry>(build_incremental_formula(query, granularity));
        iterator = cache.emplace(key, std::move(entry)).first;
    } else {
        PLAJA::StatsBase::inc_attr_unsigned(statistics, PLAJA::StatsUnsigned::PB_CACHE_HITS);
    }
    const auto assumptions = build_incremental_assumptions(query, iterator->second->query);
    const bool result = iterator->second->solver->solve(assumptions);
    if (result) { save_witness(query, iterator->second->query.formula, *iterator->second->solver, true); }
    return result;
}

void FNN_PB::Engine::increment() {
    cache.clear();
    ++predicateGeneration;
}

void FNN_PB::Engine::save_witness(const QueryContext& query, const PB::Formula& formula,
                                  const PB::IpasirSolver& solver, bool has_successor) {
    const auto value = [&solver](PB::Literal literal) { return solver.model_value(literal); };
    witnessSource = decode_state(query, formula, value, false);
    if (has_successor) { witnessTarget = decode_state(query, formula, value, true); }
}
