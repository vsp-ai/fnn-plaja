#ifndef PLAJA_FNN_POLICY_H
#define PLAJA_FNN_POLICY_H

#include <memory>
#include <vector>
#include "model/ast/forward_ast.h"
#include "model/using_parser.h"
#include "assertions.h"
#include "core/forward_factories.h"
#include "core/forward_states.h"
#include "successor_generation/forward_successor_generation.h"
#include "policies/interface/jani_2_interface.h"
#include "policies/policy.h"

class Jani2FNN;

namespace PLAJA { class FormularizedNetwork; }

class FNNPolicy : public Policy {

    using Floating_type = double;

public:
    explicit FNNPolicy(const PLAJA::Configuration& config, const Jani2FNN& fnn_interface);
    ~FNNPolicy();

    DELETE_CONSTRUCTOR(FNNPolicy)
    // getter:
    [[nodiscard]] inline const Jani2Interface& get_interface() const override { return *fnnInterface; }

    ActionLabel_type evaluate(const StateBase& state) const override;
    std::vector<double> action_values(const StateBase& state) const override;
    [[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;
    [[nodiscard]] bool is_chosen(ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;
    [[nodiscard]] bool is_chosen_with_tolerance() const override { return chosenWithTolerance; }
    [[nodiscard]] Floating_type get_selection_delta() const override {
        PLAJA_ASSERT(selectionDelta >= 0)
        return selectionDelta;
    }
    FCT_IF_DEBUG(void dump_cached_outputs() const override;)


private:
    const PLAJA::FormularizedNetwork* fnn;
    const Jani2Interface* fnnInterface;
    std::shared_ptr<const SuccessorGeneratorC> successorGenerator;

    mutable std::vector<Floating_type> cachedOutputs;
    mutable std::vector<bool>  cachedEnabledLabels;
    mutable ActionLabel_type   cachedLabel;
    mutable Floating_type selectionDelta;
    mutable bool chosenWithTolerance; // last is_chosen answered positively due to tolerance

    ActionLabel_type evaluate_argmax(const std::vector<Floating_type>& outputs) const;
    ActionLabel_type evaluate_app_filtering(const StateBase& state,
                                           const std::vector<Floating_type>& outputs,
                                           std::vector<bool>& enabled_labels) const;
    bool is_chosen_argmax(const std::vector<Floating_type>& outputs,
                          ActionLabel_type action_label,
                          double tolerance) const;
    bool is_chosen_app_filtering(const std::vector<Floating_type>& outputs,
                                 const std::vector<bool>& enabled_labels,
                                 ActionLabel_type action_label,
                                 double tolerance) const;
};

#endif //PLAJA_FNN_POLICY_H
