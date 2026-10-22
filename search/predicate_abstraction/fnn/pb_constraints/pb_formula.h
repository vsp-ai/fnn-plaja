//
// Pseudo-Boolean formula construction for FNN verification.
//

#ifndef PLAJA_FNN_PB_FORMULA_H
#define PLAJA_FNN_PB_FORMULA_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace FNN_PB {

using Literal = int;

struct Term final {
    long long coefficient = 0;
    Literal literal = 0;
};

enum class Relation { GE, LE, EQ };

struct RawConstraint final {
    std::vector<Term> terms;
    Relation relation = Relation::GE;
    long long rhs = 0;
};

/** The paper form: sum_i coefficient_i*literal_i >= rhs. */
struct NormalizedConstraint final {
    std::vector<Term> terms;
    long long rhs = 0;
};

enum class NormalizedKind { Proper, Tautology, Contradiction };

struct NormalizationResult final {
    NormalizedKind kind = NormalizedKind::Tautology;
    NormalizedConstraint constraint;
};

[[nodiscard]] Literal negate(Literal literal);
[[nodiscard]] NormalizationResult normalize_inequality(const RawConstraint& constraint);
void validate_normalized(const NormalizedConstraint& constraint);

class Formula final {
private:
    std::unordered_map<std::string, Literal> semanticToLiteral;
    std::vector<std::string> semanticNames;
    std::vector<NormalizedConstraint> constraints;
    bool inconsistent = false;

    void add_result(NormalizationResult result);

public:
    [[nodiscard]] Literal variable(const std::string& semantic_name);
    [[nodiscard]] const std::unordered_map<std::string, Literal>& symbols() const { return semanticToLiteral; }
    [[nodiscard]] const std::vector<NormalizedConstraint>& normalized_constraints() const { return constraints; }
    [[nodiscard]] std::size_t num_variables() const { return semanticNames.size(); }
    [[nodiscard]] bool is_inconsistent() const { return inconsistent; }

    void add(RawConstraint constraint);
    void add_ge(std::vector<Term> terms, long long rhs);
    void add_eq(std::vector<Term> terms, long long rhs);
    void add_fixed(Literal literal, bool value = true);
    void add_normalized(NormalizedConstraint constraint);
    void add_guarded(const NormalizedConstraint& constraint, Literal activation_literal);
    void add_guarded(RawConstraint constraint, Literal activation_literal);

    [[nodiscard]] Literal reify(RawConstraint constraint, const std::string& name);
    [[nodiscard]] Literal reify_normalized(const NormalizationResult& normalized, const std::string& name);
    [[nodiscard]] Literal reify_and(const std::vector<Literal>& children, const std::string& name);
    [[nodiscard]] Literal reify_or(const std::vector<Literal>& children, const std::string& name);
};

} // namespace FNN_PB

#endif
