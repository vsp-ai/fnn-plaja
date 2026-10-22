#include "../../search/predicate_abstraction/fnn/pb_constraints/pb_formula.h"
#include "../../search/predicate_abstraction/fnn/pb_constraints/pb_ipasir_solver.h"

#include <stdexcept>

namespace {
void require(bool value, const char* message) { if (not value) { throw std::runtime_error(message); } }
}

int main() {
    using namespace FNN_PB;
    Formula formula;
    const auto x = formula.variable("x");
    const auto y = formula.variable("y");
    formula.add_eq({ {1, x}, {1, y} }, 1);
    const auto comparison = formula.reify({ { {2, x}, {3, y} }, Relation::GE, 3 }, "comparison");

    IpasirSolver retained(formula);
    require(retained.solve({x, negate(y), negate(comparison)}), "expected retained SAT assignment");
    require(retained.solve({negate(x), y, comparison}), "expected second retained SAT assignment");
    require(not retained.solve({x, y}), "one-hot contradiction should be inconsistent");
    require(retained.solve({negate(x), y}), "assumptions leaked between retained solves");
    require(solve_fresh(formula, {negate(x), y, comparison}), "fresh PB solve disagreed");

    Formula contradiction;
    contradiction.add_ge({}, 1);
    require(not solve_fresh(contradiction), "explicit PB contradiction was satisfiable");
    return 0;
}
