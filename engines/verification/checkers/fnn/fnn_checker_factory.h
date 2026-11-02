#ifndef PLAJA_FNN_CHECKER_FACTORY_H
#define PLAJA_FNN_CHECKER_FACTORY_H

#include <memory>
#include "core/forward_factories.h"

class Jani2FNN;
class PolicyChecker;

namespace FNN_CHECKER {

    enum class FnnSatMode {
        PB_CONSTRAINT,
        Z3,
        MILP
    };

    extern std::unique_ptr<PolicyChecker> construct_checker(const Jani2FNN& jani_2_fnn, const PLAJA::Configuration& config);

}

namespace PLAJA_OPTION {
    extern const std::string fnn_sat;
    extern const std::string use_incremental_pb;
    extern const std::string incremental_pb_granularity;

    namespace FNN_CHECKER_PA {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();

        extern std::string print_supported_fnn_sat_modes();
    }
}

namespace PLAJA_OPTION_DEFAULT {

    extern const std::string fnn_sat;
    constexpr bool use_incremental_pb = false;
    extern const std::string incremental_pb_granularity;

}

#endif //PLAJA_FNN_CHECKER_FACTORY_H
