//
// Predicate atom parsing helpers for FNN symbolic models.
//
#include "predicate_atom_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <unordered_set>

namespace {
    std::string strip_kind_prefix(const std::string& atom) {
        const auto pos = atom.find(':');
        if (pos == std::string::npos) { return atom; }
        const auto after = atom.substr(pos + 1);
        if (after.find(' ') != std::string::npos) { return after; }
        return atom;
    }
}

bool PLAJA::FNN_PARSER::parse_predicate_op(const std::string& op, PLAJA::PredicateOp& out) {
    if (op == "eq") { out = PLAJA::PredicateOp::EQ; return true; }
    if (op == "neq") { out = PLAJA::PredicateOp::NEQ; return true; }
    if (op == "lt") { out = PLAJA::PredicateOp::LT; return true; }
    if (op == "leq") { out = PLAJA::PredicateOp::LEQ; return true; }
    if (op == "gt") { out = PLAJA::PredicateOp::GT; return true; }
    if (op == "geq") { out = PLAJA::PredicateOp::GEQ; return true; }
    if (op == "==") { out = PLAJA::PredicateOp::EQ; return true; }
    if (op == "!=") { out = PLAJA::PredicateOp::NEQ; return true; }
    if (op == "<") { out = PLAJA::PredicateOp::LT; return true; }
    if (op == "<=") { out = PLAJA::PredicateOp::LEQ; return true; }
    if (op == ">") { out = PLAJA::PredicateOp::GT; return true; }
    if (op == ">=") { out = PLAJA::PredicateOp::GEQ; return true; }
    return false;
}

bool PLAJA::FNN_PARSER::parse_linear_predicate(const std::string& atom, ParsedLinearPredicate& out) {
    std::string s = strip_kind_prefix(atom);
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) { tokens.push_back(tok); }
    if (tokens.size() < 3) { return false; }

    static const std::unordered_set<std::string> ops {
        "eq", "neq", "lt", "leq", "gt", "geq",
        "==", "!=", "<", "<=", ">", ">="
    };
    std::size_t op_idx = tokens.size();
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (ops.count(tokens[i])) { op_idx = i; break; }
    }
    if (op_idx == tokens.size() || op_idx + 1 >= tokens.size()) { return false; }
    if (op_idx < 1) { return false; }

    out.op = tokens[op_idx];
    const std::string& constant_str = tokens[op_idx + 1];
    char* end = nullptr;
    const double val = std::strtod(constant_str.c_str(), &end);
    if (!end || *end != '\0') { return false; }
    out.constant = static_cast<PLAJA::floating>(val);
    out.terms.clear();

    int sign = +1;
    for (std::size_t i = 0; i < op_idx; ++i) {
        const std::string& t = tokens[i];
        if (t == "+") { sign = +1; continue; }
        if (t == "-") { sign = -1; continue; }

        int term_sign = sign;
        sign = +1;

        std::string term = t;
        if (!term.empty() && (term[0] == '+' || term[0] == '-')) {
            if (term[0] == '-') { term_sign *= -1; }
            term = term.substr(1);
        }
        if (term.empty()) { return false; }

        PLAJA::floating coeff = 1.0;
        std::string var;
        const auto star = term.find('*');
        if (star != std::string::npos) {
            const std::string coeff_str = term.substr(0, star);
            var = term.substr(star + 1);
            char* end_coeff = nullptr;
            const double c = std::strtod(coeff_str.c_str(), &end_coeff);
            if (!end_coeff || *end_coeff != '\0') { return false; }
            coeff = static_cast<PLAJA::floating>(c);
        } else {
            var = term;
        }
        if (var.empty()) { return false; }

        coeff *= static_cast<PLAJA::floating>(term_sign);
        out.terms.emplace_back(var, coeff);
    }

    return !out.terms.empty();
}
