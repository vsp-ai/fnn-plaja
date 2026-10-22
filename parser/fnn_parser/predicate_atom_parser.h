//
// Predicate atom parsing helpers for FNN symbolic models.
//
#ifndef PLAJA_PREDICATE_ATOM_PARSER_H
#define PLAJA_PREDICATE_ATOM_PARSER_H

#include <string>
#include <utility>
#include <vector>
#include "../using_parser.h"

namespace PLAJA {

    enum class PredicateOp { EQ, NEQ, LT, LEQ, GT, GEQ };

    namespace FNN_PARSER {

        struct ParsedLinearPredicate {
            std::vector<std::pair<std::string, PLAJA::floating>> terms;
            std::string op;
            PLAJA::floating constant;
        };

        bool parse_predicate_op(const std::string& op, PredicateOp& out);
        bool parse_linear_predicate(const std::string& atom, ParsedLinearPredicate& out);

    } // namespace FNN_PARSER

} // namespace PLAJA

#endif // PLAJA_PREDICATE_ATOM_PARSER_H
