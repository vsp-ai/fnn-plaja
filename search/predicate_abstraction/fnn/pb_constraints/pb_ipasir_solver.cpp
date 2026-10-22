#include "pb_ipasir_solver.h"
#include "../../../../include/factory_config.h"

#ifdef USE_ROUNDINGSAT
#include "ipasirpb.h"
#endif

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

#ifdef USE_ROUNDINGSAT

const char* result_name(ipasirpb_return result) {
    switch (result) {
        case IPASIRPB_UNKNOWN: return "IPASIRPB_UNKNOWN_OR_OK";
        case IPASIRPB_SAT: return "IPASIRPB_SAT";
        case IPASIRPB_UNSAT: return "IPASIRPB_UNSAT";
        case IPASIRPB_INCONSISTENT: return "IPASIRPB_INCONSISTENT";
        case IPASIRPB_OPT: return "IPASIRPB_OPT";
        case IPASIRPB_NULL: return "IPASIRPB_NULL";
        case IPASIRPB_INVALID: return "IPASIRPB_INVALID";
        case IPASIRPB_NOT_SUPPORTED: return "IPASIRPB_NOT_SUPPORTED";
        case IPASIRPB_OVERFLOW: return "IPASIRPB_OVERFLOW";
    }
    return "IPASIRPB_UNRECOGNIZED";
}

std::string signature() {
    const auto* value = ipasirpb_signature();
    return value ? value : "<null>";
}

void require_ok(ipasirpb_return result, const char* operation) {
    if (result != IPASIRPB_OK) {
        throw std::runtime_error(std::string("PB ") + operation + " returned " + result_name(result)
                                 + " (code=" + std::to_string(static_cast<int>(result))
                                 + ", solver=" + signature() + ")");
    }
}

#endif

} // namespace

struct FNN_PB::IpasirSolver::Handle final {
#ifdef USE_ROUNDINGSAT
    void* value = ipasirpb_init();
    Handle() {
        if (not value) { throw std::runtime_error("PB ipasirpb_init returned null"); }
    }
    ~Handle() { if (value) { ipasirpb_release(value); } }
#else
    Handle() { throw std::runtime_error("PB was built without RoundingSat IPASIR support"); }
#endif
};

FNN_PB::IpasirSolver::IpasirSolver(const Formula& formula)
    : handle(nullptr), installedFormula(&formula) {
#ifdef USE_ROUNDINGSAT
    handle = std::make_unique<Handle>();
    if (formula.is_inconsistent()) { return; }
    for (const auto& constraint : formula.normalized_constraints()) {
        validate_normalized(constraint);
        std::vector<int64_t> coefficients;
        std::vector<int64_t> literals;
        coefficients.reserve(constraint.terms.size());
        literals.reserve(constraint.terms.size());
        for (const auto& term : constraint.terms) {
            coefficients.push_back(static_cast<int64_t>(term.coefficient));
            literals.push_back(static_cast<int64_t>(term.literal));
        }
        ipasirpb_terms64 input {
            coefficients.data(), literals.data(), static_cast<int64_t>(coefficients.size()),
            IPASIRPB_GEQ, static_cast<int64_t>(constraint.rhs)
        };
        require_ok(ipasirpb_add64(handle->value, input), "ipasirpb_add64");
    }
#else
    (void) formula;
#endif
}

FNN_PB::IpasirSolver::~IpasirSolver() = default;

bool FNN_PB::IpasirSolver::solve(const std::vector<Literal>& assumptions) {
    hasModel = false;
    modelValues.clear();
#ifdef USE_ROUNDINGSAT
    if (installedFormula->is_inconsistent()) { return false; }
    std::vector<int64_t> api_assumptions;
    api_assumptions.reserve(assumptions.size());
    for (const auto literal : assumptions) {
        if (literal == 0) { throw std::invalid_argument("PB assumption contains literal 0"); }
        api_assumptions.push_back(literal);
    }
    const auto result = ipasirpb_solve(handle->value,
                                       api_assumptions.empty() ? nullptr : api_assumptions.data(),
                                       static_cast<int64_t>(api_assumptions.size()));
    if (result == IPASIRPB_UNSAT or result == IPASIRPB_INCONSISTENT) { return false; }
    if (result != IPASIRPB_SAT) {
        throw std::runtime_error(std::string("PB ipasirpb_solve returned ") + result_name(result)
                                 + " (code=" + std::to_string(static_cast<int>(result))
                                 + ", solver=" + signature() + ")");
    }

    std::unordered_map<int, bool> values;
    const auto value_of = [&](Literal literal) {
        const auto variable = literal > 0 ? literal : -literal;
        auto [it, inserted] = values.emplace(variable, false);
        if (inserted) { require_ok(ipasirpb_litval(handle->value, variable, &it->second), "ipasirpb_litval"); }
        return literal > 0 ? it->second : not it->second;
    };

    for (const auto literal : assumptions) {
        if (not value_of(literal)) {
            throw std::runtime_error("PB SAT model violates assumption " + std::to_string(literal));
        }
    }
    std::size_t constraint_index = 0;
    for (const auto& constraint : installedFormula->normalized_constraints()) {
        long long lhs = 0;
        for (const auto& term : constraint.terms) {
            if (value_of(term.literal)) {
                if (lhs > std::numeric_limits<long long>::max() - term.coefficient) {
                    throw std::overflow_error("PB model validation overflow");
                }
                lhs += term.coefficient;
            }
        }
        if (lhs < constraint.rhs) {
            throw std::runtime_error("PB SAT model violates normalized constraint "
                                     + std::to_string(constraint_index));
        }
        ++constraint_index;
    }
    modelValues = std::move(values);
    hasModel = true;
    return true;
#else
    (void) assumptions;
    throw std::runtime_error("PB was built without RoundingSat IPASIR support");
#endif
}

bool FNN_PB::IpasirSolver::model_value(Literal literal) const {
    if (not hasModel) { throw std::logic_error("PB model requested without a SAT result"); }
#ifdef USE_ROUNDINGSAT
    const auto variable = literal > 0 ? literal : -literal;
    bool value = false;
    if (const auto it = modelValues.find(variable); it != modelValues.end()) { value = it->second; }
    else { require_ok(ipasirpb_litval(handle->value, variable, &value), "ipasirpb_litval"); }
    return literal > 0 ? value : not value;
#else
    (void) literal;
    throw std::runtime_error("PB was built without RoundingSat IPASIR support");
#endif
}

bool FNN_PB::solve_fresh(const Formula& formula, const std::vector<Literal>& assumptions) {
    IpasirSolver solver(formula);
    return solver.solve(assumptions);
}
