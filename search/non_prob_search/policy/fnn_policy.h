#ifndef PLAJA_FNN_POLICY_H
#define PLAJA_FNN_POLICY_H

#include <memory>
#include <vector>
#include "../../../parser/ast/forward_ast.h"
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../information/jani_2_interface.h"
#include "policy.h"

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


private:
    const PLAJA::FormularizedNetwork* formularized_network;
    const Jani2Interface* fnnInterface;
    std::shared_ptr<const SuccessorGeneratorC> successorGenerator;

    std::vector<int> inputFeatures;

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
