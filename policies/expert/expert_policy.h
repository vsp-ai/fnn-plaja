//
// This file is part of the PlaJA code base.
//

#ifndef PLAJA_EXPERT_POLICY_H
#define PLAJA_EXPERT_POLICY_H

#include <string>
#include <memory>
#include "core/state_base.h"
#include "core/using_core.h"
#include "policies/interface/jani_2_interface.h"
#include "policies/policy.h"
#include "model/ast/model.h"
#include <vector>

/** Hand-written expert policies (POLICY_EVALUATION --expert-policy). */
class ExpertPolicy : public Policy {
public:
    ~ExpertPolicy() override = default;

    ExpertPolicy(const Model* model, const Jani2Interface* interface)
        : model(model), interface(interface) {}

    const Jani2Interface& get_interface() const override { return *interface; }
    std::vector<double> action_values(const StateBase& state_values) const override;
    bool is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance) const override { return evaluate(state) == action_label; }
    bool is_chosen(ActionLabel_type action_label, double tolerance) const override { PLAJA_ABORT }
    bool is_chosen_with_tolerance() const override { return false; }
    Floating_type get_selection_delta() const override { return 0; }
    FCT_IF_DEBUG(void dump_cached_outputs() const override {})

    // Throws std::invalid_argument if the name is unknown.
    static std::unique_ptr<ExpertPolicy> create(const std::string& name, const Model* model, const Jani2Interface* interface);

protected:
    const Model* model;
    const Jani2Interface* interface;
};

class Transport : public ExpertPolicy {
protected:
    static ActionLabel_type transport_action(const StateBase& state_values, int number_of_locations, bool package_behind);

public:
    using ExpertPolicy::ExpertPolicy;

    ActionLabel_type evaluate(const StateBase& state_values) const override;
};

/** Shared by the line-world experts with parking. */
namespace LINE_PARK {
    // Action ordering:
    constexpr int ACTION_PICKUP = 0;
    constexpr int ACTION_DROP = 1;
    constexpr int ACTION_ACCELERATE = 2;
    constexpr int ACTION_DECELERATE = 3;
    constexpr int ACTION_MOVE = 4;
    constexpr int ACTION_PARK = 5;
    constexpr double BAD_ACTION = -1.0;
    constexpr double NEUTRAL_ACTION = 0.0;
    constexpr double BEST_ACTION = 1.0;

    extern int line_location_count(const Model& model);
    extern ActionLabel_type park_or_stall(const Model& model);
    extern void set_score(const Jani2Interface& interface, std::vector<double>& scores, ActionLabel_type label, double score);
}

class OneWayLinePark : public ExpertPolicy {
public:
    using ExpertPolicy::ExpertPolicy;
    ActionLabel_type evaluate(const StateBase& state_values) const override;
    std::vector<double> action_values(const StateBase& state_values) const override;
};

class TwoWayLinePark : public ExpertPolicy {
public:
    using ExpertPolicy::ExpertPolicy;
    ActionLabel_type evaluate(const StateBase& state_values) const override;
    std::vector<double> action_values(const StateBase& state_values) const override;
};

class TransportFeat : public Transport {
public:
    using Transport::Transport;
    ActionLabel_type evaluate(const StateBase& state_values) const override;
};

#endif //PLAJA_EXPERT_POLICY_H
