//
// This file is part of the PlaJA code base.
//

#include "policies/fnn/jani_2_fnn.h"

#include <algorithm>
#include <stdexcept>
#include "core/exception/constructor_exception.h"
#include "core/exception/parser_exception.h"
#include "core/option_parser/option_parser.h"
#include "core/option_parser/plaja_options.h"
#include "model/ast/model.h"
#include "policies/fnn/fnn_reader.h"
#include "policies/interface/jani_2_interface_parser.h"
#include "core/utils/utils.h"

#include "policies/fnn/formularized_network.h"
#include "policies/fnn/fnn_policy.h"

/**********************************************************************************************************************/

Jani2FNN::~Jani2FNN() = default;

Jani2FNN::Jani2FNN(const Model& model_): Jani2Interface(model_) {}

std::unique_ptr<Jani2FNN> Jani2FNN::load_from_model(const Model& model) {
    try {
        const Jani2InterfaceParser parser(model);
        std::unique_ptr<Jani2FNN> fnn_interface(new Jani2FNN(model));
        fnn_interface->fnnFile = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::fnn);
        fnn_interface->parse(parser);
        fnn_interface->load_fnn();
        return fnn_interface;
    } catch (ParserException& e) { throw ConstructorException(e.what()); }
}


const Policy& Jani2FNN::load_policy(const PLAJA::Configuration& config) const {
    if (not fnnPolicy) { fnnPolicy = std::make_unique<FNNPolicy>(config, *this); }
    return *fnnPolicy;
}


const PLAJA::FormularizedNetwork& Jani2FNN::load_fnn() const {
    if (fnn) { return *fnn; }
    fnn = PLAJA::FNN_PARSER::read_fnn(fnnFile, *this);
    return *fnn;
}

// further auxiliaries
void Jani2FNN::compute_layer_sizes() {
    // layer sizes
    layerSizes.reserve(2);
    layerSizes.push_back(inputFeatureMapping.size());
    layerSizes.push_back(outputsToLabels.size());
}

#ifndef NDEBUG

#include "core/configuration.h"

ActionLabel_type Jani2FNN::eval_policy(const StateBase& state) const {
    const PLAJA::Configuration config;
    const auto& policy = load_policy(config);
    return policy.evaluate(state);
}

bool Jani2FNN::is_chosen(const StateBase& state, ActionLabel_type action_label) const {
    const PLAJA::Configuration config;
    const auto& policy = load_policy(config);
    return policy.is_chosen(state, action_label, PLAJA_NN::argMaxPrecision);
}

#endif
