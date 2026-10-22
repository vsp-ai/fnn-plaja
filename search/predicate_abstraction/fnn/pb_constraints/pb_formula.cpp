//
// Pseudo-Boolean normalization and reification for FNN verification.
//

#include "pb_formula.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace {

long long checked_add(long long lhs, long long rhs, const char* operation) {
    if ((rhs > 0 and lhs > std::numeric_limits<long long>::max() - rhs)
        or (rhs < 0 and lhs < std::numeric_limits<long long>::min() - rhs)) {
        throw std::overflow_error(std::string("PB signed 64-bit overflow while ") + operation);
    }
    return lhs + rhs;
}

long long checked_negate(long long value, const char* operation) {
    if (value == std::numeric_limits<long long>::min()) {
        throw std::overflow_error(std::string("PB signed 64-bit overflow while ") + operation);
    }
    return -value;
}

struct Coefficients final { long long positive = 0; long long negative = 0; };

} // namespace

FNN_PB::Literal FNN_PB::negate(Literal literal) {
    if (literal == 0 or literal == std::numeric_limits<Literal>::min()) {
        throw std::runtime_error("PB cannot negate invalid literal");
    }
    return -literal;
}

FNN_PB::NormalizationResult FNN_PB::normalize_inequality(const RawConstraint& input) {
    if (input.relation == Relation::EQ) {
        throw std::invalid_argument("PB normalize_inequality does not accept equality; split it first");
    }

    long long rhs = input.relation == Relation::GE
                    ? input.rhs
                    : checked_negate(input.rhs, "flipping a <= right-hand side");
    std::map<int, Coefficients> by_variable;

    for (const auto& raw_term : input.terms) {
        if (raw_term.literal == 0) { throw std::invalid_argument("PB constraint contains literal 0"); }
        if (raw_term.literal == std::numeric_limits<Literal>::min()) {
            throw std::overflow_error("PB literal id cannot be represented as an absolute value");
        }
        long long coefficient = input.relation == Relation::GE
                                ? raw_term.coefficient
                                : checked_negate(raw_term.coefficient, "flipping a <= coefficient");
        if (coefficient == 0) { continue; }
        Literal literal = raw_term.literal;
        // -c*l = c*~l-c, hence the normalized rhs grows by c.
        if (coefficient < 0) {
            coefficient = checked_negate(coefficient, "complementing a negative coefficient");
            literal = negate(literal);
            rhs = checked_add(rhs, coefficient, "shifting rhs for a complemented coefficient");
        }
        const auto variable = literal > 0 ? literal : -literal;
        auto& coefficients = by_variable[variable];
        if (literal > 0) {
            coefficients.positive = checked_add(coefficients.positive, coefficient, "merging positive coefficients");
        } else {
            coefficients.negative = checked_add(coefficients.negative, coefficient, "merging negative coefficients");
        }
    }

    NormalizationResult result;
    long long coefficient_sum = 0;
    for (const auto& [variable, coefficients] : by_variable) {
        // a*x+b*~x = min(a,b)+|a-b|*the remaining polarity.
        const auto common = std::min(coefficients.positive, coefficients.negative);
        rhs = checked_add(rhs, -common, "cancelling complementary literals");
        const auto difference = coefficients.positive >= coefficients.negative
                                ? coefficients.positive - coefficients.negative
                                : -(coefficients.negative - coefficients.positive);
        if (difference > 0) {
            result.constraint.terms.push_back({ difference, variable });
            coefficient_sum = checked_add(coefficient_sum, difference, "summing normalized coefficients");
        } else if (difference < 0) {
            const auto magnitude = checked_negate(difference, "normalizing a coefficient");
            result.constraint.terms.push_back({ magnitude, -variable });
            coefficient_sum = checked_add(coefficient_sum, magnitude, "summing normalized coefficients");
        }
    }

    if (rhs <= 0) { result.kind = NormalizedKind::Tautology; return result; }
    if (rhs > coefficient_sum) { result.kind = NormalizedKind::Contradiction; return result; }
    result.kind = NormalizedKind::Proper;
    result.constraint.rhs = rhs;
    validate_normalized(result.constraint);
    return result;
}

void FNN_PB::validate_normalized(const NormalizedConstraint& constraint) {
    if (constraint.rhs <= 0) { throw std::logic_error("PB normalized rhs is not positive"); }
    std::set<int> variables;
    long long sum = 0;
    for (const auto& term : constraint.terms) {
        if (term.literal == 0 or term.literal == std::numeric_limits<Literal>::min()) {
            throw std::logic_error("PB normalized constraint contains an invalid literal");
        }
        if (term.coefficient <= 0) { throw std::logic_error("PB normalized coefficient is not positive"); }
        const auto variable = term.literal > 0 ? term.literal : -term.literal;
        if (not variables.insert(variable).second) {
            throw std::logic_error("PB normalized constraint contains both/repeated polarities of a variable");
        }
        sum = checked_add(sum, term.coefficient, "validating normalized coefficient sum");
    }
    if (constraint.rhs > sum) { throw std::logic_error("PB normalized constraint is contradictory"); }
}

FNN_PB::Literal FNN_PB::Formula::variable(const std::string& semantic_name) {
    const auto existing = semanticToLiteral.find(semantic_name);
    if (existing != semanticToLiteral.end()) { return existing->second; }
    if (semanticNames.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("PB exhausted literal identifiers");
    }
    const auto literal = static_cast<int>(semanticNames.size()) + 1;
    semanticNames.push_back(semantic_name);
    semanticToLiteral.emplace(semantic_name, literal);
    return literal;
}

void FNN_PB::Formula::add_result(NormalizationResult result) {
    switch (result.kind) {
        case NormalizedKind::Tautology: return;
        case NormalizedKind::Contradiction: inconsistent = true; return;
        case NormalizedKind::Proper: add_normalized(std::move(result.constraint)); return;
    }
    throw std::logic_error("PB unknown normalization result");
}

void FNN_PB::Formula::add(RawConstraint constraint) {
    if (constraint.relation != Relation::EQ) { add_result(normalize_inequality(constraint)); return; }
    auto lower = constraint;
    lower.relation = Relation::GE;
    add_result(normalize_inequality(lower));
    auto upper = std::move(constraint);
    upper.relation = Relation::LE;
    add_result(normalize_inequality(upper));
}

void FNN_PB::Formula::add_ge(std::vector<Term> terms, long long rhs) { add({ std::move(terms), Relation::GE, rhs }); }
void FNN_PB::Formula::add_eq(std::vector<Term> terms, long long rhs) { add({ std::move(terms), Relation::EQ, rhs }); }
void FNN_PB::Formula::add_fixed(Literal literal, bool value) { add_ge({ {1, value ? literal : negate(literal)} }, 1); }

void FNN_PB::Formula::add_normalized(NormalizedConstraint constraint) {
    validate_normalized(constraint);
    constraints.push_back(std::move(constraint));
}

void FNN_PB::Formula::add_guarded(const NormalizedConstraint& constraint, Literal activation_literal) {
    validate_normalized(constraint);
    auto guarded = constraint;
    guarded.terms.push_back({ guarded.rhs, negate(activation_literal) });
    add_normalized(std::move(guarded));
}

void FNN_PB::Formula::add_guarded(RawConstraint constraint, Literal activation_literal) {
    const auto add_one = [&](RawConstraint inequality) {
        auto normalized = normalize_inequality(inequality);
        switch (normalized.kind) {
            case NormalizedKind::Tautology: return;
            case NormalizedKind::Contradiction: add_fixed(activation_literal, false); return;
            case NormalizedKind::Proper: add_guarded(normalized.constraint, activation_literal); return;
        }
        throw std::logic_error("PB unknown guarded normalization result");
    };
    if (constraint.relation != Relation::EQ) { add_one(std::move(constraint)); return; }
    auto lower = constraint;
    lower.relation = Relation::GE;
    add_one(std::move(lower));
    constraint.relation = Relation::LE;
    add_one(std::move(constraint));
}

FNN_PB::Literal FNN_PB::Formula::reify_normalized(const NormalizationResult& normalized, const std::string& name) {
    const auto reification_literal = variable(name);
    if (normalized.kind == NormalizedKind::Tautology) { add_fixed(reification_literal, true); return reification_literal; }
    if (normalized.kind == NormalizedKind::Contradiction) { add_fixed(reification_literal, false); return reification_literal; }

    const auto& source = normalized.constraint;
    validate_normalized(source);
    long long sum = 0;
    for (const auto& term : source.terms) {
        sum = checked_add(sum, term.coefficient, "summing normalized coefficients for reification");
    }
    const auto complement_degree = checked_add(sum - source.rhs, 1,
                                               "computing the reification complement degree");

    NormalizedConstraint forward = source;
    forward.terms.push_back({ source.rhs, negate(reification_literal) });
    add_normalized(std::move(forward));

    NormalizedConstraint backward;
    backward.rhs = complement_degree;
    backward.terms.reserve(source.terms.size() + 1);
    backward.terms.push_back({ complement_degree, reification_literal });
    for (const auto& term : source.terms) { backward.terms.push_back({ term.coefficient, negate(term.literal) }); }
    add_normalized(std::move(backward));
    return reification_literal;
}

FNN_PB::Literal FNN_PB::Formula::reify(RawConstraint constraint, const std::string& name) {
    if (constraint.relation != Relation::EQ) { return reify_normalized(normalize_inequality(constraint), name); }
    // Equality uses two child literals and one final conjunction literal.
    auto lower = constraint;
    lower.relation = Relation::GE;
    auto upper = std::move(constraint);
    upper.relation = Relation::LE;
    const auto lower_literal = reify_normalized(normalize_inequality(lower), name + "__ge");
    const auto upper_literal = reify_normalized(normalize_inequality(upper), name + "__le");
    return reify_and({ lower_literal, upper_literal }, name);
}

FNN_PB::Literal FNN_PB::Formula::reify_and(const std::vector<Literal>& children, const std::string& name) {
    if (children.empty()) { const auto literal = variable(name); add_fixed(literal, true); return literal; }
    std::vector<Term> terms;
    terms.reserve(children.size());
    for (const auto child : children) { terms.push_back({1, child}); }
    return reify({ std::move(terms), Relation::GE, static_cast<long long>(children.size()) }, name);
}

FNN_PB::Literal FNN_PB::Formula::reify_or(const std::vector<Literal>& children, const std::string& name) {
    if (children.empty()) { const auto literal = variable(name); add_fixed(literal, false); return literal; }
    std::vector<Term> terms;
    terms.reserve(children.size());
    for (const auto child : children) { terms.push_back({1, child}); }
    return reify({ std::move(terms), Relation::GE, 1 }, name);
}
