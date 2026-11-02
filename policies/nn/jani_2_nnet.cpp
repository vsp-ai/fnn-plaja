//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#include <iostream>
#include "policies/nn/jani_2_nnet.h"
#include "policies/interface/jani_2_interface_parser.h"
#include "core/exception/constructor_exception.h"
#include "core/exception/parser_exception.h"
#include "core/option_parser/option_parser.h"
#include "core/option_parser/plaja_options.h"
#include "policies/nn/neural_network.h"
#include "model/ast/type/declaration_type.h"
#include "model/ast/model.h"
#include "model/ast/variable_declaration.h"
#include "policies/nn/nn_policy.h"
#include "model/model_information.h"
#include "policies/interface/input_feature_to_jani_derived.h"

namespace JANI_2_NNET {
    const std::string layerSizeError { "Jani2NNet layer size error" }; // NOLINT(cert-err58-cpp)
}

/**********************************************************************************************************************/

Jani2NNet::~Jani2NNet() = default;

Jani2NNet::Jani2NNet(const Model& model_): Jani2Interface(model_) {}

// The hidden layer sizes are those of the --nn file; expert policies have none.
std::unique_ptr<Jani2NNet> Jani2NNet::load_from_model(const Model& model) {
    try {
        const Jani2InterfaceParser parser(model);
        std::unique_ptr<Jani2NNet> nn_interface(new Jani2NNet(model));
        if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::nn)) {
            nn_interface->nnFile = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::nn);
            const PLAJA::NeuralNetwork nn(nn_interface->nnFile);
            for (unsigned int layer = 1; layer < nn.getNumLayers(); ++layer) { nn_interface->hiddenLayerSizes.push_back(nn.getLayerSize(layer)); }
        }
        nn_interface->parse(parser);
        return nn_interface;
    } catch (ParserException& e) { throw ConstructorException(e.what()); }
}

// loading NN

const PLAJA::NeuralNetwork& Jani2NNet::load_network() const {
    if (nn) { return *nn; }
    std::cout << "Loading NN file ..." << nnFile << std::endl;
    try {
        nn = std::make_unique<PLAJA::NeuralNetwork>(nnFile); // unfortunately parser results in segmentation fault for invalid but existing NN files

        // sanity checks
        auto num_layers = layerSizes.size();
        if (nn->getNumLayers() != num_layers - 1) { throw ConstructorException(JANI_2_NNET::layerSizeError); } // -1, does not count input
        for (LayerIndex_type layer_index = 0; layer_index < num_layers; ++layer_index) {
            if (nn->getLayerSize(layer_index) != layerSizes[layer_index]) { throw ConstructorException(JANI_2_NNET::layerSizeError); }
        }

        // sanity check bounds (NN vs. state variable bounds as per JANI model)
        bool differing_bounds = false;
        for (auto input_it = init_input_feature_iterator(false); !input_it.end(); ++input_it) {
            auto input_index = input_it._input_index();
            auto state_index = input_it.get_input_feature_index();
            if (nn->getInputLowerBound(input_index) != get_model_info().get_lower_bound(state_index) or nn->getInputUpperBound(input_index) != get_model_info().get_upper_bound(state_index)) {
                differing_bounds = true;
            }
            // always use state variable bounds:
            nn->min[input_index] = get_model_info().get_lower_bound(state_index);
            nn->max[input_index] = get_model_info().get_upper_bound(state_index);
        }
        if (differing_bounds) { std::cout << "Warning: NN input bounds and state variable bounds differ (latter will be used)." << std::endl; }
        std::cout << "Successfully loaded nnet" << std::endl;
        PLAJA_ASSERT(nn)
        return *nn;

    } catch (ParserException& e) { throw ConstructorException("Error while parsing NN file."); }
}

const Policy& Jani2NNet::load_policy(const PLAJA::Configuration& config) const {
    if (not nnPolicy) { nnPolicy = std::make_unique<NNPolicy>(config, *this); PLAJA_ASSERT(nnPolicy); }
    return *nnPolicy;
}

#ifndef NDEBUG

#include "core/configuration.h"

ActionLabel_type Jani2NNet::eval_policy(const StateBase& state) const {
    const PLAJA::Configuration config;
    const auto& nn_policy = load_policy(config);
    return nn_policy.evaluate(state);
}

bool Jani2NNet::is_chosen(const StateBase& state, ActionLabel_type action_label) const {
    const PLAJA::Configuration config;
    const auto& nn_policy = load_policy(config);
    return nn_policy.is_chosen(state, action_label, PLAJA_NN::argMaxPrecision);
}

#endif

// further auxiliaries

void Jani2NNet::compute_layer_sizes() {
    // layer sizes
    layerSizes.reserve(hiddenLayerSizes.size() + 2);
    layerSizes.push_back(inputFeatureMapping.size());
    for (const unsigned int size: hiddenLayerSizes) {
        layerSizes.push_back(size);
    }
    layerSizes.push_back(outputsToLabels.size());
}
