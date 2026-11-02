#include <memory>
#include "engines/verification/checkers/fnn/fnn_checker_factory.h"
#include "engines/verification/checkers/fnn/fnn_checker_z3.h"
#include "engines/verification/checkers/fnn/fnn_checker_pb.h"
#ifdef SUPPORT_GUROBI
#include "engines/verification/checkers/fnn/fnn_checker_milp.h"
#endif
#include "core/exception/constructor_exception.h"
#include "include/ct_config_const.h"
#include "include/pa_config_const.h"
#include "core/option_parser/option_parser_aux.h"
#include "core/option_parser/option_parser.h"
#include "core/configuration.h"
#include "model/model_information.h"
#include "engines/verification/smt/model_z3_pa.h"

namespace PLAJA_OPTION_DEFAULT {
    const std::string fnn_sat("z3"); // NOLINT(cert-err58-cpp)
    const std::string incremental_pb_granularity("label"); // NOLINT(cert-err58-cpp)
}

namespace PLAJA_OPTION {

    const std::string fnn_sat("fnn-sat"); // NOLINT(cert-err58-cpp)
    const std::string use_incremental_pb("use-incremental-pb"); // NOLINT(cert-err58-cpp)
    const std::string incremental_pb_granularity("incremental-pb-granularity"); // NOLINT(cert-err58-cpp)

    namespace FNN_CHECKER_PA {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::fnn_sat, PLAJA_OPTION_DEFAULT::fnn_sat);
            OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::use_incremental_pb, PLAJA_OPTION_DEFAULT::use_incremental_pb);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::incremental_pb_granularity, PLAJA_OPTION_DEFAULT::incremental_pb_granularity);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::fnn_sat, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::fnn_sat, "Set mode for FNN-satisfiability check to compute (relaxed) PPA):", true);
            OPTION_PARSER::print_additional_specification(FNN_CHECKER_PA::print_supported_fnn_sat_modes());
            OPTION_PARSER::print_bool_option(PLAJA_OPTION::use_incremental_pb, PLAJA_OPTION_DEFAULT::use_incremental_pb, "Reuse precompiled PB TSAT formulas through IPASIR assumptions. Only complete transition queries are supported.");
            OPTION_PARSER::print_option(PLAJA_OPTION::incremental_pb_granularity, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::incremental_pb_granularity, "Set incremental PB formula granularity to label or operator.");
        }

        const std::unordered_map<std::string, FNN_CHECKER::FnnSatMode> stringToFnnSatMode { // NOLINT(cert-err58-cpp)
            { "pb-constraint", FNN_CHECKER::FnnSatMode::PB_CONSTRAINT },
            { "z3",            FNN_CHECKER::FnnSatMode::Z3 },
#ifdef SUPPORT_GUROBI
            { "milp",          FNN_CHECKER::FnnSatMode::MILP },
#endif
        };

        std::string print_supported_fnn_sat_modes() { return PLAJA::OptionParser::print_supported_options(stringToFnnSatMode); }
    }
}

/**********************************************************************************************************************/

std::unique_ptr<PolicyChecker> FNN_CHECKER::construct_checker(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config) {
    PLAJA_LOG("Constructing FNN-SAT checker ...")

    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    const auto* predicates = config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3)->_predicates();
    if (predicates) { PolicyChecker::check_linear(*predicates); }

    const auto fnn_sat_mode = config.get_option(PLAJA_OPTION::FNN_CHECKER_PA::stringToFnnSatMode, PLAJA_OPTION::fnn_sat);

    const bool use_incremental_pb = config.get_bool_option(PLAJA_OPTION::use_incremental_pb);
    const auto& incremental_pb_granularity = config.get_option(PLAJA_OPTION::incremental_pb_granularity);
    if (incremental_pb_granularity != "label" and incremental_pb_granularity != "operator") {
        throw ConstructorException("--incremental-pb-granularity must be label or operator");
    }
    if (use_incremental_pb and fnn_sat_mode != FNN_CHECKER::FnnSatMode::PB_CONSTRAINT) {
        throw ConstructorException("--use-incremental-pb requires --fnn-sat pb-constraint");
    }
    if (use_incremental_pb and (PLAJA_GLOBAL::doSelectionTest or PLAJA_GLOBAL::doNNApplicabilityTest)) {
        throw ConstructorException("incremental PB supports full transition TSAT queries only; disable compile-time selection and NN-applicability tests");
    }
    if (use_incremental_pb) { // The incremental formula fixes the source location for the successor copy.
        const auto& model_info = config.get_sharable<ModelZ3>(PLAJA::SharableKey::MODEL_Z3)->get_model_info();
        for (auto loc_it = model_info.locationIndexIterator(); !loc_it.end(); ++loc_it) {
            if (loc_it.range() > 1) { throw ConstructorException("incremental PB supports models with one location per automaton only; use --use-incremental-pb false"); }
        }
    }

    switch (fnn_sat_mode) {
        case FNN_CHECKER::FnnSatMode::Z3: { return std::unique_ptr<PolicyChecker>(new FnnCheckerZ3(jani_2_fnn, config)); }
        case FNN_CHECKER::FnnSatMode::PB_CONSTRAINT: { return std::unique_ptr<PolicyChecker>(new FnnCheckerPB(jani_2_fnn, config)); }
#ifdef SUPPORT_GUROBI
        case FNN_CHECKER::FnnSatMode::MILP: { return std::unique_ptr<PolicyChecker>(new FnnCheckerMilp(jani_2_fnn, config)); }
#endif
        default: { PLAJA_ABORT }
    }
    PLAJA_ABORT
}
