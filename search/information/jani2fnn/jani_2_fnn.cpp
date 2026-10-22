//
// This file is part of the PlaJA code base.
// Copyright ...
//

#include "jani_2_fnn.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <json.hpp>
#include "../../../exception/constructor_exception.h"
#include "../../../exception/parser_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/expression/array_access_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/fnn_parser/sym_model_parser.h"
#include "../../../utils/utils.h"
#include "../../states/state_base.h" //NOTE: DELETE ME LATER 

#include "../../../parser/fnn_parser/formularized_network.h"
#include "../../non_prob_search/policy/fnn_policy.h"

// for pretty printing
#include <iomanip>
#include <algorithm>
#include <unordered_set>

namespace {
    std::unordered_map<std::string, VariableIndex_type> build_var_to_state_index_map(const Jani2FNN& iface) {
        std::unordered_map<std::string, VariableIndex_type> map;
        for (auto it = iface.init_input_feature_iterator(true); !it.end(); ++it) {
            const auto* feature = it.get_input_feature();
            if (feature->is_location()) { continue; }
            const auto* expr = feature->get_expression();
            if (auto* var = dynamic_cast<const VariableExpression*>(expr)) {
                map[var->get_name()] = it.get_input_feature_index();
            } else if (auto* arr = dynamic_cast<const ArrayAccessExpression*>(expr)) {
                const auto* base = dynamic_cast<const VariableExpression*>(arr->get_accessedArray());
                const auto* idx = arr->get_index();
                if (!base || !idx || !idx->is_constant()) { continue; }
                const auto idx_val = idx->evaluateInteger(nullptr);
                map[base->get_name() + "_" + std::to_string(idx_val)] = it.get_input_feature_index();
            }
        }
        return map;
    }
} // namespace

namespace JANI_2_FNN {
    extern std::unique_ptr<Jani2FNN> load_fnn_interface(const Model& model, const std::string& fnn_interface_file);

}

/**********************************************************************************************************************/

Jani2FNN::~Jani2FNN() = default;

Jani2FNN::Jani2FNN(const Model& model_)
    : Jani2Interface() {
    this->model = &model_;
    this->interfaceFile = "";
    this->policyFile = "";
    this->doApplicabilityFiltering = false;
}
std::unique_ptr<Jani2FNN> Jani2FNN::load_interface(const Model& model) {
    std::cout << "Loading FNN interface from option parser: " << PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::fnn_interface) << std::endl;
    return load_interface(model, PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::fnn_interface));
}

std::unique_ptr<Jani2FNN> Jani2FNN::load_interface(const Model& model, const std::string& jani_2_fnn_file) {
    try { 
        auto interface = JANI_2_FNN::load_fnn_interface(model, jani_2_fnn_file); 
        interface->load_fnn();
        return interface;
    }
    catch (ParserException& e) { throw ConstructorException(e.what()); }
}


const Policy& Jani2FNN::load_policy(const PLAJA::Configuration& config) const {
    if (!fnnPolicy) { fnnPolicy = std::make_unique<FNNPolicy>(config, *this); }
    return *fnnPolicy;
}


const PLAJA::FormularizedNetwork& Jani2FNN::load_fnn() const {
    if(fnn) return *fnn; //already loaded)
    try{
        fnn = std::make_unique<PLAJA::FormularizedNetwork>(fnnFile);
        auto& fnnNodes        = fnn->fnnNodes;
        auto& nodeActivations = fnn->nodeActivations;
        auto& linearToOutput  = fnn->linearToOutput;
        auto& initLogicalIdx  = fnn->initLogicalIdx;
        auto& initLinearIdx   = fnn->initLinearIdx;
        auto& inputFeatures   = fnn->inputFeatures;


        // Keep onehot encoding tied to real NN inputs only (extended inputs are for filtering/refinement).
        std::size_t l = this->get_num_input_features();
        inputFeatures.reserve(l);
        for (InputIndex_type i = 0; i < l; ++i) { inputFeatures.push_back(this->get_input_feature(i)->stateIndex); }
        
        
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::fnn));
        auto json_path = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::fnn);
        std::ifstream jsonFile(json_path);
        if (!jsonFile.is_open()) {
            std::cerr << "Failed to open the JSON file: " << json_path << std::endl;
            PLAJA_ASSERT(false);
        }
        using json = nlohmann::json;
        json j;
        jsonFile >> j;

        fnnNodes.clear();
        nodeActivations.clear();
        fnn->binFeatures.clear();
        fnn->topologicallySorted = false;

        std::unordered_map<std::string, int> name_to_idx;
        const auto atoms_spec = PLAJA::FNN_PARSER::read_atoms_spec(j);
        auto pending_rules = PLAJA::FNN_PARSER::read_rules(j);
        std::unordered_map<std::string, int> output_name_to_index;
        output_name_to_index.reserve(outputsToLabels.size());
        for (OutputIndex_type oi = 0; oi < outputsToLabels.size(); ++oi) {
            const auto action_label = outputsToLabels[oi];
            output_name_to_index.emplace(model->get_action_name(action_label), static_cast<int>(oi));
        }

        std::size_t linear_count = 0;
        if (j.contains("linear") && j["linear"].contains("weights") && j["linear"]["weights"].is_object()) {
            linear_count = j["linear"]["weights"].size();
        }
        fnnNodes.reserve(atoms_spec.atoms.size() + pending_rules.size() + linear_count);

        int next_idx = 0;
        // Onehot and predicates atoms share one representation: a linear
        // predicate per BIN node (onehot atoms are the single-term eq/neq
        // special case).
        fnn->binFeatures.reserve(atoms_spec.atoms.size());
        auto var_to_state_index = build_var_to_state_index_map(*this);
        PLAJA::FNN_PARSER::append_bin_features(atoms_spec.atoms, var_to_state_index, fnnNodes, name_to_idx, fnn->binFeatures, next_idx);

        initLogicalIdx = next_idx; // LOGICAL INDEX STARTS HERE
        PLAJA::FNN_PARSER::append_rules_topologically(pending_rules, name_to_idx, fnnNodes, next_idx);
        PLAJA::FNN_PARSER::append_linear_nodes(
            j,
            outputsToLabels.size(),
            output_name_to_index,
            name_to_idx,
            fnnNodes,
            linearToOutput,
            fnn->linearSize,
            initLinearIdx,
            next_idx
        );
        nodeActivations.resize(fnnNodes.size());
        fnn->topologicallySorted = true;
        return *fnn;
    }catch (ParserException& e) { throw ConstructorException("Error while parsing formularized neural network file."); }

}

// further auxiliaries
void Jani2FNN::compute_layer_sizes() {
    // layer sizes
    layerSizes.reserve(2);
    layerSizes.push_back(inputFeatureMapping.size());
    layerSizes.push_back(outputsToLabels.size());
}

void Jani2FNN::compute_extended_inputs() {
    PLAJA_ASSERT(not layerSizes.empty()) // Must be computed first.

    if (not doApplicabilityFiltering) { return; }

    const auto& model_info = get_model_info();

    // Include locations only if they are not fixed.
    bool include_locs = false;
    for (auto loc_it = model_info.locationIndexIterator(); !loc_it.end(); ++loc_it) {
        if (loc_it.range() > 1) {
            include_locs = true;
            break;
        }
    }

    std::unordered_set<VariableIndex_type> actual_input_features;
    for (auto it = init_input_feature_iterator(false); !it.end(); ++it) { actual_input_features.insert(it.get_input_feature_index()); }

    const auto guard_vars = model->collect_guard_variables(include_locs);

    for (auto it = model_info.stateIndexIterator(include_locs); !it.end(); ++it) {
        const auto state_index = it.state_index();

        if (actual_input_features.count(state_index) or not guard_vars.count(state_index)) { continue; }

        if (it.is_location()) {
            inputFeatureMapping.push_back(std::make_unique<InputFeatureToLocation>(it.location_index()));
            continue;
        }

        const auto id = model_info.get_id(state_index);
        if (model_info.is_array(id)) {
            const auto& var_decl = *model->get_variable_by_id(id);
            inputFeatureMapping.push_back(std::make_unique<InputFeatureToArray>(var_decl, state_index - var_decl.get_index()));
        } else {
            inputFeatureMapping.push_back(std::make_unique<InputFeatureToVariable>(*model->get_variable_by_id(id)));
        }
    }
}

void Jani2FNN::compute_labels_to_outputs() {
    for (OutputIndex_type output_index = 0; output_index < outputsToLabels.size(); ++output_index) {
        labelsToOutputs[outputsToLabels[output_index]] = output_index;
    }
    // also cache unlearned labels:
    unlearnedLabels.push_back(ACTION::silentLabel);
    const auto num_actions = model->get_number_actions();
    unlearnedLabels.reserve(num_actions - get_num_output_features() + 1);
    for (ActionLabel_type action_label = 0; action_label < num_actions; ++action_label) {
        if (!is_learned(action_label)) { unlearnedLabels.push_back(action_label); }
    }
}

#ifndef NDEBUG

#include "../../factories/configuration.h"

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
