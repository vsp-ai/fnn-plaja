#ifndef PLAJA_POLICY_EVALUATION_ENGINE_H
#define PLAJA_POLICY_EVALUATION_ENGINE_H

#include "../search/fd_adaptions/search_engine.h"
#include "../search/fd_adaptions/state.h"
#include "../search/information/jani_2_interface.h"
#include "../search/information/property_information.h"
#include "../search/successor_generation/simulation_environment.h"
#include "../search/non_prob_search/initial_states_enumerator.h"
#include "../search/using_search.h"
#include "../search/non_prob_search/policy/policy_restriction.h"
#include <json.hpp>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include "../domain_specific/domain_specific.h"


class PolicyEvaluationEngine : public SearchEngine {
public:
    explicit PolicyEvaluationEngine(const PLAJA::Configuration& config);
    ~PolicyEvaluationEngine() override;
    SearchStatus initialize() override;
    SearchStatus step() override;
private:
    using Reward_type = float;
    static constexpr Reward_type gamma = 0.99; // Discount factor.

    enum class Outcome {
        Running,
        Goal,
        Avoid,
        DeadEnd,
        Stalling,
        Cycle,
        MaxLen
    };

    int numEpisodes;
    int maxEpisodeLength;
    bool terminateCycles;
    bool regression;
    std::unique_ptr<std::string> tracesFile;
    std::stringstream traces;
    int episodesCounter;
    std::size_t nextStart;
    const Reward_type goalReward;
    const Reward_type avoidReward;
    const Reward_type stallingReward;
    const Reward_type terminalReward;
    double totalScore;
    double totalEpisodeLength;

    const ObjectiveExpression* objective;
    const Expression* avoid;
    std::shared_ptr<SimulationEnvironment> simulatorEnv;
    
    std::unique_ptr<InitialStatesEnumerator> startEnumerator;
    const Jani2Interface* jani2Interface;

    std::vector<StateID_type> startStates;

    std::shared_ptr<PolicyRestriction> policy;
    std::unique_ptr<DomainSpecific> expert_policy;


    void init_start_states();
    std::unique_ptr<State> skip_forced_actions(const State& state);
    Outcome run_episode(std::unique_ptr<State> current);
    void count_outcome(Outcome outcome);
    [[nodiscard]] Reward_type step_reward(const State& current, const State* next) const;
    void store_trace(const State& state, ActionLabel_type chosen_label);
    bool is_goal(const State& state) const;
    bool is_avoid(const State& state) const;
    void write_traces();

    void print_statistics() const override;
    void stat_names_to_csv(std::ofstream& file) const override;
    void stats_to_csv(std::ofstream& file) const override;
};


#endif //PLAJA_POLICY_EVALUATION_ENGINE_H
