#ifndef POLICY_EVALUATION_OPTIONS_H
#define POLICY_EVALUATION_OPTIONS_H

#include <string>

namespace PLAJA_OPTION {
    extern const std::string num_episodes_1;
    extern const std::string max_len_episode;
    extern const std::string terminate_cycles_1;
    extern const std::string store_traces;
    extern const std::string store_traces_mode;
    extern const std::string expert_policy;
    extern const std::string goal_reward;
    extern const std::string avoid_reward;
    extern const std::string stalling_reward;
    extern const std::string terminal_reward;
}

namespace PLAJA_OPTION_DEFAULT {
    constexpr const int num_episodes_1 = 1000;
    constexpr const int max_len_episode = 1000;
    constexpr double goalReward = 200;
    constexpr double avoidReward = -100;
    constexpr double stallingReward = -100;
    constexpr double terminalReward = -50;
}

#endif // POLICY_EVALUATION_OPTIONS_H
