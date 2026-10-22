//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "options_marabou.h"
#include "../../../include/marabou_include/options.h"
#include "../../../include/ct_config_const.h"
#include "../../../option_parser/option_parser.h"
#include "../using_marabou.h"
#include <algorithm>
#include <vector>

namespace PLAJA_OPTION {

    extern const std::string marabou_options;
    extern const std::string marabou_solver;
    extern const std::string nn_tightening;
    extern const std::string nn_tightening_per_label;
    extern const std::string per_src_tightening;
    extern const std::string per_src_label_tightening;

}

namespace OPTIONS_MARABOU {

    const std::unordered_map<std::string, SolverType> stringToSolverType { // NOLINT(cert-err58-cpp)
        { "reluplex", OPTIONS_MARABOU::SolverType::RELUPLEX },
        { "soi",      OPTIONS_MARABOU::SolverType::SOI },
        { "milp",     OPTIONS_MARABOU::SolverType::MILP },
    };

    int marabouTimeout; // NOLINT(*-avoid-non-const-global-variables)

    int get_timeout() { return marabouTimeout; }

}

std::string OPTIONS_MARABOU::gurobi_requirement() {
    if constexpr (not PLAJA_GLOBAL::supportGurobi) { return PLAJA_UTILS::emptyString; }
    if (not PLAJA_GLOBAL::optionParser) { return "--lp-solver gurobi"; }
    const auto& option_parser = *PLAJA_GLOBAL::optionParser;

    const auto arguments = option_parser.has_value_option(PLAJA_OPTION::marabou_options) ? PLAJA_UTILS::split(option_parser.get_value_option_string(PLAJA_OPTION::marabou_options), PLAJA_UTILS::spaceString) : std::vector<std::string>();
    const auto argument_value = [&arguments](const std::string& name) -> std::string {
        for (auto it = arguments.begin(); it != arguments.end(); ++it) {
            if (*it == name) { return std::next(it) == arguments.end() ? PLAJA_UTILS::emptyString : *std::next(it); }
            if (it->rfind(name + "=", 0) == 0) { return it->substr(name.size() + 1); }
        }
        return PLAJA_UTILS::emptyString;
    };

    // Marabou uses Gurobi for any LP solver value other than "native" (PlaJA appends "--lp-solver gurobi" by default).
    if (argument_value("--lp-solver") != "native") { return "--lp-solver gurobi"; }
    if (std::find(arguments.begin(), arguments.end(), "--milp") != arguments.end()) { return "--milp"; }
    if (option_parser.get_option(stringToSolverType, PLAJA_OPTION::marabou_solver) == MILP) { return "Marabou MILP solver"; }
    const auto milp_tightening = argument_value("--milp-tightening");
    if (not milp_tightening.empty() and milp_tightening != "none") { return "--milp-tightening " + milp_tightening; }
    for (const auto* option: { &PLAJA_OPTION::nn_tightening, &PLAJA_OPTION::nn_tightening_per_label, &PLAJA_OPTION::per_src_tightening, &PLAJA_OPTION::per_src_label_tightening }) {
        if (option_parser.has_option(*option) and option_parser.get_option(*option) != "none") { return "--" + *option + " " + option_parser.get_option(*option); }
    }
    return PLAJA_UTILS::emptyString;
}

void OPTIONS_MARABOU::parse_marabou_options() {
    static bool done = false;
    if (done) { return; } // parsing already happened once ...
    else { PLAJA_LOG("Parsing options for Marabou ..."); }

    std::string options_str = "marabou "; // dummy program call

    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::marabou_options)) {
        options_str.append(PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::marabou_options));
    }

    if (not PLAJA_UTILS::contains(options_str, "verbosity")) { options_str.append(" --verbosity 0"); }

    const auto marabou_solver = PLAJA_GLOBAL::optionParser->get_option(stringToSolverType, PLAJA_OPTION::marabou_solver);

    if constexpr (PLAJA_GLOBAL::supportGurobi) {
        if (marabou_solver != MILP) {
            if (not PLAJA_UTILS::contains(options_str, "lp-solver")) { options_str.append(" --lp-solver gurobi"); }
        }
    }

    switch (marabou_solver) {
        case RELUPLEX: {
            GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = false;
            break;
        }
        case SOI: {
            GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = true;
            break;
        }
        case MILP: {
            options_str.append(" --milp");
            break;
        }
        default: PLAJA_ABORT
    }

#if 0
    /*
     * MV: This does not work if preprocessorBoundTolerance is smallter than precision of std::string ...
     * Better to just use Marabou's internal default value.
     */
    if (not PLAJA_UTILS::contains(options_str, "preprocessor-bound-tolerance")) {
        options_str.append(" --preprocessor-bound-tolerance "); // for incremental solving
        options_str.append(std::to_string(MARABOU_IN_PLAJA::preprocessorBoundTolerance));
    }
#endif

    // replace "~" by "-" (not required)
    // PLAJA_UTILS::replace_all(options_str, "~", PLAJA_UTILS::dashString);

    // Split at whitespace
    const std::vector<std::string> arguments_strs = PLAJA_UTILS::split(options_str, PLAJA_UTILS::spaceString);

    // arguments to char*
    char** args = PLAJA_UTILS::strings_to_char_array(arguments_strs);

    Options* options = Options::get();
    PLAJA_ASSERT(arguments_strs.size() <= std::numeric_limits<int>::max())
    options->parseOptions(arguments_strs.size(), args); // NOLINT(cppcoreguidelines-narrowing-conversions)
    OPTIONS_MARABOU::marabouTimeout = options->getInt(Options::TIMEOUT);

    // delete
    PLAJA_UTILS::delete_char_array(arguments_strs.size(), args); // NOLINT(cppcoreguidelines-narrowing-conversions)

    done = true; // do not repeat parsing !!!
}