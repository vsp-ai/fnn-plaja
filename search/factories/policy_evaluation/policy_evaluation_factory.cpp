#include "policy_evaluation_factory.h"

#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../information/property_information.h"
#include "../configuration.h"
#include "policy_evaluation_options.h"

#ifdef USE_Z3
namespace PLAJA_OPTION::SMT_SOLVER_Z3 {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}
#endif

namespace POLICY_EVALUATION_ENGINE { extern std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config); }

PolicyEvaluationFactory::PolicyEvaluationFactory():
    SearchEngineFactory(SearchEngineType::PolicyEvaluationType) {}

PolicyEvaluationFactory::~PolicyEvaluationFactory() = default;

void PolicyEvaluationFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SearchEngineFactory::add_options(option_parser);

    OPTION_PARSER::add_int_option(
        option_parser,
        PLAJA_OPTION::num_episodes_1,
        PLAJA_OPTION_DEFAULT::num_episodes_1
    );

    OPTION_PARSER::add_int_option(
        option_parser,
        PLAJA_OPTION::max_len_episode,
        PLAJA_OPTION_DEFAULT::max_len_episode
    );

    OPTION_PARSER::add_flag(
        option_parser,
        PLAJA_OPTION::terminate_cycles_1
    );

    OPTION_PARSER::add_double_option(
        option_parser,
        PLAJA_OPTION::goal_reward,
        PLAJA_OPTION_DEFAULT::goalReward
    );

    OPTION_PARSER::add_double_option(
        option_parser,
        PLAJA_OPTION::avoid_reward,
        PLAJA_OPTION_DEFAULT::avoidReward
    );

    OPTION_PARSER::add_double_option(
        option_parser,
        PLAJA_OPTION::stalling_reward,
        PLAJA_OPTION_DEFAULT::stallingReward
    );

    OPTION_PARSER::add_double_option(
        option_parser,
        PLAJA_OPTION::terminal_reward,
        PLAJA_OPTION_DEFAULT::terminalReward
    );

    OPTION_PARSER::add_value_option(
        option_parser,
        PLAJA_OPTION::store_traces
    );

    OPTION_PARSER::add_value_option(
        option_parser,
        PLAJA_OPTION::store_traces_mode
    );

    PLAJA_OPTION::add_initial_state_enum(option_parser);

    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::expert_policy);

#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::add_options(option_parser);
#endif
}

void PolicyEvaluationFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Policy evaluation");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_episodes_1, PLAJA_OPTION_DEFAULT::num_episodes_1, "Number of episodes.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_len_episode, PLAJA_OPTION_DEFAULT::max_len_episode, "Maximal number of policy steps per episode.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::terminate_cycles_1, "End an episode when it repeats a transition.");
    PLAJA_OPTION::print_initial_state_enum();
    OPTION_PARSER::print_double_option(PLAJA_OPTION::goal_reward, PLAJA_OPTION_DEFAULT::goalReward, "Reward for reaching the goal.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::avoid_reward, PLAJA_OPTION_DEFAULT::avoidReward, "Reward for reaching an avoid state.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::stalling_reward, PLAJA_OPTION_DEFAULT::stallingReward, "Reward for choosing an inapplicable action.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::terminal_reward, PLAJA_OPTION_DEFAULT::terminalReward, "Reward for reaching a terminal (dead-end) state.");
    OPTION_PARSER::print_option(PLAJA_OPTION::store_traces, OPTION_PARSER::fileStr, "Append the visited states and chosen actions (training data) to <file>.");
    OPTION_PARSER::print_option(PLAJA_OPTION::store_traces_mode, "classification|regression", "Store the chosen action label (classification, default) or the policy's action values (regression).");
    OPTION_PARSER::print_option(PLAJA_OPTION::expert_policy, OPTION_PARSER::valueStr, "Use a built-in expert policy instead of a learned one: Transport, Transport_Feat, OneWayLinePark or TwoWayLinePark.");
    OPTION_PARSER::print_options_footline();
}

std::unique_ptr<SearchEngine> PolicyEvaluationFactory::construct(const PLAJA::Configuration& config) const {
    return POLICY_EVALUATION_ENGINE::construct(config);
}

namespace SEARCH_ENGINE_FACTORY {
    std::unique_ptr<SearchEngineFactory> construct_policy_evaluation_factory() { return std::make_unique<PolicyEvaluationFactory>(); }
} // namespace SEARCH_ENGINE_FACTORY
