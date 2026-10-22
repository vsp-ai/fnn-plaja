#include "policy_evaluation_engine.h"
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cctype>
#include <json.hpp>

#include "../globals.h"

#include "../stats/stats_base.h"
#include "../search/factories/configuration.h"
#include "../search/factories/policy_evaluation/policy_evaluation_options.h"
#include "../exception/plaja_exception.h"
#include "../option_parser/plaja_options.h"

#include "../parser/ast/expression/non_standard/objective_expression.h"
#include "../parser/ast/expression/expression.h"  
#include "../parser/ast/expression/lvalue_expression.h"
#include "../parser/ast/model.h"

#include "policy_evaluation_statistics.h"

namespace {
struct Transition {
    StateID_type src;
    ActionLabel_type label;
    StateID_type successor;
};

struct TransitionHash {
    std::size_t operator()(const Transition& transition) const {
        constexpr std::size_t prime = 31;
        std::size_t result = 1;
        result = prime * result + transition.src;
        result = prime * result + transition.label;
        result = prime * result + transition.successor;
        return result;
    }
};

struct TransitionEqual {
    bool operator()(const Transition& lhs, const Transition& rhs) const {
        return lhs.src == rhs.src and lhs.label == rhs.label and lhs.successor == rhs.successor;
    }
};

bool is_regression_mode(const PLAJA::Configuration& config) {
    if (!config.has_value_option(PLAJA_OPTION::store_traces_mode)) {
        return false;
    }

    auto mode = config.get_value_option_string(PLAJA_OPTION::store_traces_mode);
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (mode == "classification") { return false; }
    if (mode == "regression") { return true; }

    throw PlajaException(
        "Unsupported --store-traces-mode value: " + mode +
        ". Supported values: classification, regression."
    );
}

nlohmann::json input_features(const State& state, const Jani2Interface& jani2Interface) {
    nlohmann::json inputs = nlohmann::json::array();
    for (auto it = jani2Interface.init_input_feature_iterator(); !it.end(); ++it) {
        const auto state_index = it.get_input_feature_index();
        if (state_index < state.get_int_state_size()) {
            inputs.push_back(state.get_int(state_index));
        } else {
            inputs.push_back(state.get_float(state_index));
        }
    }
    return inputs;
}
} // namespace

PolicyEvaluationEngine::PolicyEvaluationEngine(const PLAJA::Configuration& config_)
    : SearchEngine(config_)
    , numEpisodes(config_.get_int_option(PLAJA_OPTION::num_episodes_1))
    , maxEpisodeLength(config_.get_int_option(PLAJA_OPTION::max_len_episode))
    , terminateCycles(config_.is_flag_set(PLAJA_OPTION::terminate_cycles_1))
    , regression(is_regression_mode(config_))
    , tracesFile(config_.has_value_option(PLAJA_OPTION::store_traces)
        ? std::make_unique<std::string>(config_.get_value_option_string(PLAJA_OPTION::store_traces))
        : nullptr)
    , episodesCounter(0)
    , nextStart(0)
    , goalReward(PLAJA_UTILS::cast_numeric<Reward_type>(config_.get_double_option(PLAJA_OPTION::goal_reward)))
    , avoidReward(PLAJA_UTILS::cast_numeric<Reward_type>(config_.get_double_option(PLAJA_OPTION::avoid_reward)))
    , stallingReward(PLAJA_UTILS::cast_numeric<Reward_type>(config_.get_double_option(PLAJA_OPTION::stalling_reward)))
    , terminalReward(PLAJA_UTILS::cast_numeric<Reward_type>(config_.get_double_option(PLAJA_OPTION::terminal_reward)))
    , totalScore(0.0)
    , totalEpisodeLength(0.0)
    , startEnumerator(
        propertyInfo->get_start()
            ? std::make_unique<InitialStatesEnumerator>(config_, *propertyInfo->get_start())
            : nullptr)
    , jani2Interface(propertyInfo->get_interface())
{
    PLAJA_ASSERT(jani2Interface);
    POLICY_EVALUATION_STATS::add_basic_stats(*searchStatistics);
    simulatorEnv = std::make_shared<SimulationEnvironment>(config_, *model);
    objective = propertyInfo->get_learning_objective();
    avoid     = propertyInfo->get_reach();
    init_start_states();
    if(config_.has_value_option(PLAJA_OPTION::expert_policy) )
    {
        expert_policy = DomainSpecific::create(config_.get_value_option_string(PLAJA_OPTION::expert_policy), model, jani2Interface);  
        policy= nullptr;
    }else{
        expert_policy = nullptr;
        policy = std::make_shared<PolicyRestriction>(config_, *jani2Interface);
        
    }
    
    if (tracesFile) {
        PLAJA_LOG(std::string("[POLICY EVAL] Dataset store mode: ") + (regression ? "regression" : "classification"));
    }

}

PolicyEvaluationEngine::~PolicyEvaluationEngine() = default;

void PolicyEvaluationEngine::write_traces() {
    if (!tracesFile) { return; }

    auto data = traces.str();
    if (data.empty()) { return; }
    if (data.back() == ',') {
        data.pop_back();
    }

    std::ofstream out(*tracesFile, std::ios::app);
    if (out.fail()) {
        throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + *tracesFile);
    }
    out << "[" << data << "]" << std::endl;
    traces.str(std::string());
    traces.clear();
}

void PolicyEvaluationEngine::init_start_states() {
    startStates.clear();
    if (numEpisodes <= 0) { return; }

    std::vector<StateID_type> starts;
    if (startEnumerator) {
        startEnumerator->initialize();
        if (startEnumerator->samples()) {
            for (int i = 0; i < numEpisodes; ++i) {
                auto start = startEnumerator->sample_state();
                if (!start) {
                    PLAJA_LOG("Warning: startEnumerator->sample_state() returned null. Stopping early.");
                    break;
                }
                starts.push_back(simulatorEnv->get_state(*start).get_id_value());
            }
        } else {
            std::unordered_set<StateID_type> seen;
            for (const auto& s: startEnumerator->enumerate_states()) {
                const StateID_type id = simulatorEnv->get_state(s).get_id_value();
                if (seen.insert(id).second) { starts.push_back(id); }
            }
        }
    } else {
        starts.push_back(simulatorEnv->get_initial_state().get_id_value());
    }

    if (starts.empty()) {
        throw PlajaException("[POLICY EVAL] No start states were sampled.");
    }

    std::unordered_set<StateID_type> distinct;
    for (int i = 0; i < numEpisodes; ++i) {
        const StateID_type id = starts[static_cast<std::size_t>(i) % starts.size()];
        if (distinct.insert(id).second) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::START_STATES);
        }
        startStates.push_back(id);
    }
}

SearchEngine::SearchStatus PolicyEvaluationEngine::initialize() {
    episodesCounter = 0;
    nextStart = 0;
    totalScore = 0.0;
    totalEpisodeLength = 0.0;
    searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_SCORE, 0.0);
    searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_AVERAGE_LENGTH, 0.0);

    return SearchEngine::SearchStatus::IN_PROGRESS;
}

SearchEngine::SearchStatus PolicyEvaluationEngine::step() {
    if (nextStart >= startStates.size()) {
        write_traces();
        return SearchEngine::SearchStatus::FINISHED;
    }

    auto start = skip_forced_actions(simulatorEnv->get_state(startStates[nextStart]));
    ++nextStart;
    if (!start) {
        static bool print_warning(true);
        PLAJA_LOG_IF(print_warning, PLAJA_UTILS::to_red_string("Terminal start state."));
        print_warning = false;
        return SearchEngine::SearchStatus::IN_PROGRESS;
    }

    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EPISODES);
    count_outcome(run_episode(std::move(start)));
    episodesCounter++;
    return SearchEngine::SearchStatus::IN_PROGRESS;
}

// Applies forced actions (single applicable, not learned); nullptr if a goal, avoid or terminal state comes first.
std::unique_ptr<State> PolicyEvaluationEngine::skip_forced_actions(const State& state) {
    std::unique_ptr<State> current = state.to_ptr();
    auto applicable_actions = simulatorEnv->extract_applicable_actions(*current, true);
    while (applicable_actions.size() <= 1) {
        if (applicable_actions.empty()) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            return nullptr;
        }

        const ActionLabel_type action = applicable_actions[0];
        if (jani2Interface->is_learned(action)) { break; }

        current = simulatorEnv->compute_successor(*current, action);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);

        if (is_goal(*current)) {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
            return nullptr;
        }
        if (is_avoid(*current)) {
            return nullptr;
        }

        applicable_actions = simulatorEnv->extract_applicable_actions(*current, true);
    }

    return current;
}

PolicyEvaluationEngine::Outcome PolicyEvaluationEngine::run_episode(std::unique_ptr<State> current) {
    PLAJA_ASSERT(current);
    std::unordered_set<Transition, TransitionHash, TransitionEqual> seenTransitions;
    if (terminateCycles) {
        seenTransitions.reserve(static_cast<std::size_t>(maxEpisodeLength));
    }

    Outcome outcome = Outcome::Running;
    double score = 0.0;

    int it = 0;
    while(it < maxEpisodeLength){
        if (is_goal(*current)){
            outcome = Outcome::Goal;
            break;
        }else if(is_avoid(*current)){
            outcome = Outcome::Avoid;
            break;
        }
        // get applicable actions
        auto applicable_actions = simulatorEnv->extract_applicable_actions(*current, /*filter*/ true);
        if (applicable_actions.empty()) {
            // terminal dead-end
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
            outcome = Outcome::DeadEnd;
            break;
        }
        
        ActionLabel_type a;
        if(expert_policy)
        {
            a = expert_policy->get_action(current->to_state_values());
            if(std::find(applicable_actions.begin(), applicable_actions.end(), a) == applicable_actions.end())
            {
                PLAJA_LOG("Warning: expert policy chose an inapplicable action.");
            }
        }
        else
        {
            a = policy->get_action(*current);
        }
        

        if (tracesFile) {
            store_trace(*current, a);
        }
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::POLICY_ACTS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_LABELS);
        searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::EXPANDED_STATES);

        Reward_type reward = 0;
        auto succ_state = simulatorEnv->compute_successor_if_applicable(*current, a);
        if (!succ_state) {
            reward = stallingReward;
            outcome = Outcome::Stalling;
        } else {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);

            while (true) {
                if (is_goal(*succ_state)) {
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GOAL_STATES);
                    reward = step_reward(*current, succ_state.get()) + goalReward;
                    outcome = Outcome::Goal;
                    break;
                }
                if (is_avoid(*succ_state)) {
                    reward = step_reward(*current, succ_state.get()) + avoidReward;
                    outcome = Outcome::Avoid;
                    break;
                }

                auto successor_applicable_actions = simulatorEnv->extract_applicable_actions(*succ_state, true);
                if (successor_applicable_actions.empty()) {
                    searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TERMINAL_STATES);
                    reward = step_reward(*current, succ_state.get()) + terminalReward;
                    outcome = Outcome::DeadEnd;
                    break;
                }

                if (successor_applicable_actions.size() > 1
                    or jani2Interface->is_learned(successor_applicable_actions[0])) {
                    reward = step_reward(*current, succ_state.get());
                    break;
                }

                succ_state = simulatorEnv->compute_successor(*succ_state, successor_applicable_actions[0]);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::GENERATED_STATES);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::ACTION_OPS);
                searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::TRANSITIONS);
            }
        }

        score += static_cast<double>(reward) * std::pow(static_cast<double>(gamma), it);

        if (terminateCycles) {
            const Transition transition{current->get_id_value(), a, succ_state ? succ_state->get_id_value() : STATE::none};
            if (!seenTransitions.insert(transition).second) {
                if (succ_state) {
                    current = std::move(succ_state);
                }
                ++it;
                outcome = Outcome::Cycle;
                break;
            }
        }

        if (!succ_state || outcome != Outcome::Running) {
            if (succ_state) {
                current = std::move(succ_state);
            }
            ++it;
            break;
        }

        current = std::move(succ_state);
        ++it;
    }
    if (it >= maxEpisodeLength && outcome == Outcome::Running) {
        if (is_goal(*current)) {
            outcome = Outcome::Goal;
        } else if (is_avoid(*current)) {
            outcome = Outcome::Avoid;
        } else {
            outcome = Outcome::MaxLen;
        }
    }

    totalScore += score;
    totalEpisodeLength += static_cast<double>(it);
    const auto completed_episodes = static_cast<double>(episodesCounter + 1);
    searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_SCORE, totalScore / completed_episodes);
    searchStatistics->set_attr_double(PLAJA::StatsDouble::LAST_AVERAGE_LENGTH, totalEpisodeLength / completed_episodes);

    return outcome;
}

void PolicyEvaluationEngine::count_outcome(Outcome outcome) {
    switch (outcome) {
        case Outcome::Goal: {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_GOAL);
            break;
        }
        case Outcome::Avoid: {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_AVOID);
            break;
        }
        case Outcome::DeadEnd:
        case Outcome::Stalling: {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DONE_STALLING);
            break;
        }
        case Outcome::Cycle: {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::DETECTED_CYCLES);
            break;
        }
        case Outcome::MaxLen: {
            searchStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::UNDONE);
            break;
        }
        case Outcome::Running: {
            PLAJA_ABORT;
        }
    }
}

void PolicyEvaluationEngine::store_trace(const State& state, ActionLabel_type chosen_label) {
    nlohmann::json sample;
    sample["state"] = input_features(state, *jani2Interface);

    if (regression) {
        sample["action"] = expert_policy ? expert_policy->action_values(state.to_state_values()) : policy->action_values(state);
    } else {
        sample["action"] = chosen_label;
    }

    traces << sample.dump() << ",";
}

PolicyEvaluationEngine::Reward_type PolicyEvaluationEngine::step_reward(const State& current, const State* next) const {
    if (!next || !objective) { return 0; }

    return static_cast<Reward_type>(objective->compute_step_reward(&current, next));
}

bool PolicyEvaluationEngine::is_goal(const State& state) const {
    auto* goal = objective ? objective->get_goal() : nullptr;
    return goal && goal->evaluateInteger(&state);
}

bool PolicyEvaluationEngine::is_avoid(const State& state) const {
    return avoid && avoid->evaluateInteger(&state);
}

void PolicyEvaluationEngine::print_statistics() const {
    searchStatistics->print_statistics();
    PLAJA_LOG(PLAJA_UTILS::string_f(
        "AverageScore: %f",
        searchStatistics->get_attr_double(PLAJA::StatsDouble::LAST_SCORE)
    ));
    PLAJA_LOG(PLAJA_UTILS::string_f(
        "AverageLength: %f",
        searchStatistics->get_attr_double(PLAJA::StatsDouble::LAST_AVERAGE_LENGTH)
    ));
}

void PolicyEvaluationEngine::stat_names_to_csv(std::ofstream& file) const {
    searchStatistics->stat_names_to_csv(file);
    file << PLAJA_UTILS::commaString << "AverageScore"
         << PLAJA_UTILS::commaString << "AverageLength";
}

void PolicyEvaluationEngine::stats_to_csv(std::ofstream& file) const {
    searchStatistics->stats_to_csv(file);
    file << PLAJA_UTILS::commaString << searchStatistics->get_attr_double(PLAJA::StatsDouble::LAST_SCORE)
         << PLAJA_UTILS::commaString << searchStatistics->get_attr_double(PLAJA::StatsDouble::LAST_AVERAGE_LENGTH);
}

namespace POLICY_EVALUATION_ENGINE {
    std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) {
        return std::make_unique<PolicyEvaluationEngine>(config);
    }

} // namespace POLICY_EVALUATION_ENGINE
