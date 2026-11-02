#ifndef PLAJA_FNN_PB_ENGINE_H
#define PLAJA_FNN_PB_ENGINE_H

#include "engines/verification/smt/pb_query_builder.h"

#include <cstddef>
#include <map>
#include <memory>

namespace PLAJA { class StatsBase; }

namespace PB { class IpasirSolver; }

namespace FNN_PB {

class Engine final {
private:
    struct CacheKey;
    struct Entry;

    bool useIncremental;
    Granularity granularity;
    std::size_t predicateGeneration = 0;
    PLAJA::StatsBase* statistics;
    std::map<CacheKey, std::unique_ptr<Entry>> cache;
    // Witness of the last SAT query (empty if the last query was UNSAT or had no successor copy).
    std::vector<PLAJA::integer> witnessSource;
    std::vector<PLAJA::integer> witnessTarget;

    void save_witness(const QueryContext& query, const PB::Formula& formula, const PB::IpasirSolver& solver, bool has_successor);

public:
    Engine(bool use_incremental, Granularity granularity, PLAJA::StatsBase* statistics);
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    [[nodiscard]] bool solve(const QueryContext& query);
    void increment();

    [[nodiscard]] const std::vector<PLAJA::integer>& witness_source() const { return witnessSource; }
    [[nodiscard]] const std::vector<PLAJA::integer>& witness_target() const { return witnessTarget; }
};

} // namespace FNN_PB

#endif
