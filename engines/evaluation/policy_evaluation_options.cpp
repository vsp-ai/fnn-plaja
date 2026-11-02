#include "engines/evaluation/policy_evaluation_options.h"

namespace PLAJA_OPTION {
    const std::string num_episodes("num-episodes");
    const std::string max_len_episode("max-len-episode");
    const std::string terminate_cycles("terminate-cycles");
    const std::string store_traces("store-traces");
    const std::string store_traces_mode("store-traces-mode");
    const std::string goal_reward("goal-reward"); // NOLINT(cert-err58-cpp)
    const std::string avoid_reward("avoid-reward"); // NOLINT(cert-err58-cpp)
    const std::string stalling_reward("stalling-reward"); // NOLINT(cert-err58-cpp)
    const std::string terminal_reward("terminal-reward"); // NOLINT(cert-err58-cpp)
}
