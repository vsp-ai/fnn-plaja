//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_CHECKER_FACTORY_H
#define PLAJA_NN_CHECKER_FACTORY_H

#include <memory>
#include "core/forward_factories.h"
#include "policies/nn/forward_jani_2_nnet.h"

class NNChecker;

namespace NN_CHECKER {

    enum class NNSatMode {
        MARABOU
    };

    extern std::unique_ptr<NNChecker> construct_checker(const Jani2NNet& jani_2_nnet, const PLAJA::Configuration& config);

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    extern const std::string nn_sat;
    extern const std::string per_src_tightening;
    extern const std::string per_src_label_tightening;

    namespace NN_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();

        extern std::string print_supported_nn_sat_modes();
    }
}

namespace PLAJA_OPTION_DEFAULT {

    extern const std::string nn_sat;

}

#endif //PLAJA_NN_CHECKER_FACTORY_H
