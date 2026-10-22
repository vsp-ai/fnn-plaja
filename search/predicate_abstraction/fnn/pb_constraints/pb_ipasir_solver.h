#ifndef PLAJA_FNN_PB_IPASIR_SOLVER_H
#define PLAJA_FNN_PB_IPASIR_SOLVER_H

#include "pb_formula.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace FNN_PB {

class IpasirSolver final {
private:
    struct Handle;
    std::unique_ptr<Handle> handle;
    const Formula* installedFormula;
    std::unordered_map<int, bool> modelValues; // variable -> value, from the last SAT solve
    bool hasModel = false;

public:
    explicit IpasirSolver(const Formula& formula);
    ~IpasirSolver();
    IpasirSolver(const IpasirSolver&) = delete;
    IpasirSolver& operator=(const IpasirSolver&) = delete;

    [[nodiscard]] bool solve(const std::vector<Literal>& assumptions = {});
    /** Value of a literal in the model of the last solve; only valid if that solve returned SAT. */
    [[nodiscard]] bool model_value(Literal literal) const;
};

[[nodiscard]] bool solve_fresh(const Formula& formula, const std::vector<Literal>& assumptions = {});

} // namespace FNN_PB

#endif
