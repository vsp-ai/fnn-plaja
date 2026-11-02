//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include "engines/verification/checkers/nn/nn_checker_factory.h"
#include "include/ct_config_const.h"
#include "core/option_parser/option_parser_aux.h"
#include "core/option_parser/option_parser.h"
#include "core/configuration.h"
#include "engines/verification/checkers/nn/nn_checker.h"
#include "engines/verification/smt/model_marabou_pa.h"

namespace PLAJA_OPTION_DEFAULT {

    const std::string nn_sat("marabou"); // NOLINT(cert-err58-cpp)
    const std::string per_src_tightening("none"); // NOLINT(cert-err58-cpp)
    const std::string per_src_label_tightening("none"); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION {

    const std::string nn_sat("nn-sat"); // NOLINT(cert-err58-cpp)
    const std::string per_src_tightening("per-src-tightening"); // NOLINT(cert-err58-cpp)
    const std::string per_src_label_tightening("per-src-label-tightening"); // NOLINT(cert-err58-cpp)

    namespace SMT_SOLVER_MARABOU { extern std::string print_supported_bound_tightening(); }

    namespace NN_CHECKER_PA {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::nn_sat, PLAJA_OPTION_DEFAULT::nn_sat);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::per_src_tightening, PLAJA_OPTION_DEFAULT::per_src_tightening);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::per_src_label_tightening, PLAJA_OPTION_DEFAULT::per_src_label_tightening);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::nn_sat, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::nn_sat, "Set mode for NN-satisfiability check to compute (relaxed) PPA):", true);
            OPTION_PARSER::print_additional_specification(NN_CHECKER_PA::print_supported_nn_sat_modes());

            if constexpr (PLAJA_GLOBAL::supportGurobi) {
                OPTION_PARSER::print_option(PLAJA_OPTION::per_src_tightening, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::per_src_tightening, "Per abstract source expansion tightening:", true);
                OPTION_PARSER::print_additional_specification(PLAJA_OPTION::SMT_SOLVER_MARABOU::print_supported_bound_tightening());
                OPTION_PARSER::print_option(PLAJA_OPTION::per_src_label_tightening, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::per_src_label_tightening, "Per abstract source expansion and label tightening.");
            }
        }

        const std::unordered_map<std::string, NN_CHECKER::NNSatMode> stringToNNSatMode { // NOLINT(cert-err58-cpp)
            { "marabou", NN_CHECKER::NNSatMode::MARABOU } };

        std::string print_supported_nn_sat_modes() { return PLAJA::OptionParser::print_supported_options(stringToNNSatMode); }
    }

}

/**********************************************************************************************************************/

std::unique_ptr<NNChecker> NN_CHECKER::construct_checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config) {
    PLAJA_LOG("Constructing NN-SAT checker ...")

    // Check predicates
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU))
    PLAJA_ASSERT(config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU)->_predicates())
    PolicyChecker::check_linear(*config.get_sharable<ModelMarabou>(PLAJA::SharableKey::MODEL_MARABOU)->_predicates());

    switch (config.get_option(PLAJA_OPTION::NN_CHECKER_PA::stringToNNSatMode, PLAJA_OPTION::nn_sat)) {
        case NN_CHECKER::NNSatMode::MARABOU: { return std::make_unique<NNChecker>(jani_2_nnet, config); }
        default: { PLAJA_ABORT }
    }
    PLAJA_ABORT
}
