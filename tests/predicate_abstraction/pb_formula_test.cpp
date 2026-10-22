#include "../../search/predicate_abstraction/fnn/pb_constraints/pb_formula.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (not condition) { throw std::runtime_error(message); }
}

bool literal_value(FNN_PB::Literal literal, std::uint64_t assignment) {
    const auto variable = literal > 0 ? literal : -literal;
    const bool value = ((assignment >> (variable - 1)) & 1U) != 0;
    return literal > 0 ? value : not value;
}

bool raw_value(const FNN_PB::RawConstraint& constraint, std::uint64_t assignment) {
    long long lhs = 0;
    for (const auto& term : constraint.terms) {
        if (literal_value(term.literal, assignment)) { lhs += term.coefficient; }
    }
    if (constraint.relation == FNN_PB::Relation::GE) { return lhs >= constraint.rhs; }
    if (constraint.relation == FNN_PB::Relation::LE) { return lhs <= constraint.rhs; }
    return lhs == constraint.rhs;
}

bool normalized_value(const FNN_PB::NormalizationResult& normalized, std::uint64_t assignment) {
    if (normalized.kind == FNN_PB::NormalizedKind::Tautology) { return true; }
    if (normalized.kind == FNN_PB::NormalizedKind::Contradiction) { return false; }
    long long lhs = 0;
    for (const auto& term : normalized.constraint.terms) {
        if (literal_value(term.literal, assignment)) { lhs += term.coefficient; }
    }
    return lhs >= normalized.constraint.rhs;
}

bool formula_value(const FNN_PB::Formula& formula, std::uint64_t assignment) {
    if (formula.is_inconsistent()) { return false; }
    for (const auto& constraint : formula.normalized_constraints()) {
        long long lhs = 0;
        for (const auto& term : constraint.terms) {
            if (literal_value(term.literal, assignment)) { lhs += term.coefficient; }
        }
        if (lhs < constraint.rhs) { return false; }
    }
    return true;
}

void test_paper_normalization_example() {
    using namespace FNN_PB;
    RawConstraint raw { { {3, 1}, {-1, 2}, {-5, 3}, {4, 4} }, Relation::LE, -3 };
    const auto normalized = normalize_inequality(raw);
    require(normalized.kind == NormalizedKind::Proper, "paper example did not normalize to a proper constraint");
    require(normalized.constraint.rhs == 10, "paper example has wrong normalized rhs");
    const std::vector<Term> expected { {3, -1}, {1, 2}, {5, 3}, {4, -4} };
    require(normalized.constraint.terms.size() == expected.size(), "paper example has wrong term count");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(normalized.constraint.terms[i].coefficient == expected[i].coefficient,
                "paper example has wrong coefficient");
        require(normalized.constraint.terms[i].literal == expected[i].literal,
                "paper example has wrong literal");
    }
}

void test_normalization_truth_tables() {
    using namespace FNN_PB;
    const std::vector<RawConstraint> constraints {
        { { {2, 1}, {-3, 2}, {4, -1}, {-1, -2} }, Relation::GE, -1 },
        { { {-2, 1}, {5, -2}, {1, 3} }, Relation::LE, 2 },
        { { {1, 1}, {1, -1} }, Relation::GE, 1 },
        { { {1, 1}, {1, 2} }, Relation::GE, 3 },
    };
    for (const auto& raw : constraints) {
        const auto normalized = normalize_inequality(raw);
        for (std::uint64_t assignment = 0; assignment < 8; ++assignment) {
            require(raw_value(raw, assignment) == normalized_value(normalized, assignment),
                    "normalization changed the truth value");
        }
    }
}

void test_atomic_reification() {
    using namespace FNN_PB;
    Formula formula;
    const auto x = formula.variable("x");
    const auto y = formula.variable("y");
    const RawConstraint raw { { {2, x}, {3, y} }, Relation::GE, 3 };
    const auto r = formula.reify(raw, "r_D");
    require(r == 3, "unexpected atomic reification literal");
    for (std::uint64_t assignment = 0; assignment < 8; ++assignment) {
        require(formula_value(formula, assignment) == (literal_value(r, assignment) == raw_value(raw, assignment)),
                "atomic reification is not an equivalence");
    }
}

void test_equality_and_boolean_reification() {
    using namespace FNN_PB;
    Formula formula;
    const auto x = formula.variable("x");
    const auto y = formula.variable("y");
    const RawConstraint equality { { {1, x}, {1, y} }, Relation::EQ, 1 };
    const auto eq = formula.reify(equality, "r_eq");
    const auto either = formula.reify_or({x, y}, "r_or");
    const auto both = formula.reify_and({eq, either}, "r_root");

    for (std::uint64_t base = 0; base < 4; ++base) {
        std::size_t satisfying_extensions = 0;
        for (std::uint64_t extension = 0; extension < (1ULL << (formula.num_variables() - 2)); ++extension) {
            const auto assignment = base | (extension << 2);
            if (formula_value(formula, assignment)) {
                ++satisfying_extensions;
                const bool expected_eq = raw_value(equality, assignment);
                const bool expected_or = literal_value(x, assignment) or literal_value(y, assignment);
                require(literal_value(eq, assignment) == expected_eq, "equality literal has wrong value");
                require(literal_value(either, assignment) == expected_or, "OR literal has wrong value");
                require(literal_value(both, assignment) == (expected_eq and expected_or), "AND literal has wrong value");
            }
        }
        require(satisfying_extensions == 1, "reification variables do not have a unique extension");
    }
}

void test_guarded_contradiction_disables_only_activation() {
    using namespace FNN_PB;
    Formula formula;
    const auto activation = formula.variable("activate");
    formula.add_guarded({{}, Relation::GE, 1}, activation);
    require(not formula.is_inconsistent(), "guarded contradiction poisoned the root formula");
    require(formula_value(formula, 0), "disabled contradictory branch should be satisfiable");
    require(not formula_value(formula, 1), "enabled contradictory branch should be inconsistent");
}

} // namespace

int main() {
    test_paper_normalization_example();
    test_normalization_truth_tables();
    test_atomic_reification();
    test_equality_and_boolean_reification();
    test_guarded_contradiction_disables_only_activation();
    return 0;
}
