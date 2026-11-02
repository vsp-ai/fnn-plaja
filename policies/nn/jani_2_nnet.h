//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_JANI_2_NNET_H
#define PLAJA_JANI_2_NNET_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "model/ast/expression/forward_expression.h"
#include "model/ast/forward_ast.h"
#include "assertions.h"
#include "core/forward_factories.h"
#include "policies/forward_policy.h"
#include "core/forward_states.h"
#include "model/forward_information.h"
#include "policies/nn/forward_jani_2_nnet.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "policies/interface/jani_2_interface.h"

namespace PLAJA { class NeuralNetwork; }

/**
 * This class documents the interface between a JANI model and an underlying neural network stored in .nnet format.
 * The interface is dependent on the current implementation of JANI models, \ie, the used indexes and ids.
 */
class Jani2NNet : public Jani2Interface {
protected:
    std::string nnFile;
    std::vector<unsigned int> hiddenLayerSizes;

private:
    // loaded structures
    mutable std::unique_ptr<PLAJA::NeuralNetwork> nn;
    mutable std::unique_ptr<NNPolicy> nnPolicy;

protected:
    void compute_layer_sizes() override;

    explicit Jani2NNet(const Model& model_);
public:
    virtual ~Jani2NNet();

    Jani2NNet(Jani2NNet&& jani_2_nnet) = delete;
    DELETE_ASSIGNMENT(Jani2NNet)

    static std::unique_ptr<Jani2NNet> load_from_model(const Model& model);

    [[nodiscard]] const PLAJA::NeuralNetwork& load_network() const;
    [[nodiscard]] const Policy& load_policy(const PLAJA::Configuration& config) const override;
    // For debugging only, load policy directly for runtime usage.
    FCT_IF_DEBUG([[nodiscard]] ActionLabel_type eval_policy(const StateBase& state) const override;)
    FCT_IF_DEBUG([[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label) const override;)


    /* Layers: */

    [[nodiscard]] inline std::size_t get_num_layers() const { return layerSizes.size(); }

    [[nodiscard]] inline std::size_t get_num_hidden_layers() const { return layerSizes.size() - 2; }

/**********************************************************************************************************************/
};

#endif //PLAJA_JANI_2_NNET_H
