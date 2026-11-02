#ifndef PLAJA_PB_IPASIR_SOLVER_H
#define PLAJA_PB_IPASIR_SOLVER_H

#include "pseudo_boolean/pb_formula.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace PB {

class IpasirSolver final {
private:
    struct Handle;
    std::unique_ptr<Handle> handle;
    const Formula* installedFormula;
    std::unordered_map<int, bool> modelValues; // variable -> value, from the last SAT solve

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

} // namespace PB

#endif
