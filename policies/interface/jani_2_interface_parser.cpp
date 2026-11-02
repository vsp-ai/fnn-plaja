//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include <unordered_set>
#include <json.hpp>
#include "policies/interface/jani_2_interface_parser.h"
#include "core/exception/parser_exception.h"
#include "include/ct_config_const.h"
#include "core/option_parser/option_parser.h"
#include "core/option_parser/plaja_options.h"
#include "model/ast/iterators/model_iterator.h"
#include "model/ast/action.h"
#include "model/ast/automaton.h"
#include "model/ast/variable_declaration.h"
#include "model/jani_words.h"
#include "model/parser_utils.h"
#include "policies/interface/input_feature_to_jani_derived.h"

Jani2InterfaceParser::Jani2InterfaceParser(const Model& model_):
    model(model_)
    , loadedJSON(std::make_unique<nlohmann::json>()) {
    if (not model.has_policy()) { throw ParserException("The model has no policy field."); }
    (*loadedJSON)[JANI::POLICY][JANI::INPUTS] = model.get_policy_inputs();
    (*loadedJSON)[JANI::POLICY][JANI::LABELS] = model.get_policy_labels();
}

Jani2InterfaceParser::~Jani2InterfaceParser() = default;

/**********************************************************************************************************************/

#define PARSE_VARIABLE(REF, NAME)\
    const std::size_t l = (REF).get_number_variables();\
    for (std::size_t i = 0; i < l; ++i) {\
        const VariableDeclaration* var_decl = (REF).get_variable(i);\
        if (var_decl->get_name() == (NAME)) {\
            std::unique_ptr<InputFeatureToVariable> input_feature_to(new InputFeatureToVariable(*var_decl));\
            return input_feature_to;\
        }\
    }\
    PARSER::throw_parser_exception(input, (*loadedJSON), PARSER::invalid);

// "variable", or "automaton.variable" for a local one
std::unique_ptr<InputFeatureToJANI> Jani2InterfaceParser::parse_InputFeatureToName(const nlohmann::json& input) const {
    const std::string name = input;
    const auto dot = name.find('.');
    if (dot == std::string::npos) { PARSE_VARIABLE(model, name) }
    for (AutomatonIndex_type index = 0; index < model.get_number_automataInstances(); ++index) {
        const Automaton& automaton = *model.get_automatonInstance(index);
        if (automaton.get_name() == name.substr(0, dot)) { PARSE_VARIABLE(automaton, name.substr(dot + 1)) }
    }
    PARSER::throw_parser_exception(input, (*loadedJSON), PARSER::invalid);
}

/**********************************************************************************************************************/

bool Jani2InterfaceParser::parse_applicability_filtering() const {
    return PLAJA_GLOBAL::enableApplicabilityFiltering and PLAJA_GLOBAL::optionParser->get_int_option(PLAJA_OPTION::applicability_filtering) > 0;
}

std::vector<std::unique_ptr<InputFeatureToJANI>> Jani2InterfaceParser::parse_input_interface() const {
    const nlohmann::json& input = loadedJSON->at(JANI::POLICY).at(JANI::INPUTS);
    PARSER::check_array(input, (*loadedJSON));
    PARSER::throw_parser_exception_if(input.empty(), input, (*loadedJSON), PARSER::at_least_one_element);

    std::vector<std::unique_ptr<InputFeatureToJANI>> input_feature_mapping;
    input_feature_mapping.reserve(input.size());
    for (const auto& input_feature_to: input) {
        input_feature_mapping.push_back(parse_InputFeatureToName(input_feature_to));
    }

    return input_feature_mapping;
}

std::vector<ActionLabel_type> Jani2InterfaceParser::parse_output_interface() const {
    const nlohmann::json& output = loadedJSON->at(JANI::POLICY).at(JANI::LABELS);
    PARSER::check_array(output, (*loadedJSON));
    PARSER::throw_parser_exception_if(output.empty(), output, (*loadedJSON), PARSER::at_least_one_element);

    // cache actions
    std::unordered_map<std::string, ActionID_type> action_name_to_id;
    for (auto it = ModelIterator::actionIterator(model); !it.end(); ++it) { action_name_to_id[it->get_name()] = it->get_id(); }

    std::vector<ActionLabel_type> output_to_labels;
    output_to_labels.reserve(output.size());
    std::unordered_set<ActionLabel_type> output_to_labels_cache; // for duplicate detection
    for (const auto& action: output) {
        PARSER::check_string(action, (*loadedJSON));
        PARSER::throw_parser_exception_if(not action_name_to_id.count(action), action, (*loadedJSON), PARSER::unknown_name);
        output_to_labels.push_back(action_name_to_id.at(action));
        PARSER::throw_parser_exception_if(output_to_labels_cache.count(output_to_labels.back()), action, (*loadedJSON), PARSER::duplicate_name);
        output_to_labels_cache.insert(output_to_labels.back());
    }

    return output_to_labels;
}
