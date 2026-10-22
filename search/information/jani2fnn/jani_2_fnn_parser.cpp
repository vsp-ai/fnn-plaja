#include <memory>
#include <unordered_set>
#include <json.hpp>
#include "jani_2_fnn_parser.h"
#include "../../../exception/parser_exception.h"
#include "../../../include/ct_config_const.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../parser/ast/iterators/model_iterator.h"
#include "../../../parser/ast/action.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/composition.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../../parser/jani_words.h"
#include "../../../parser/parser_utils.h"
#include "../../../utils/utils.h"
#include "../input_feature_to_jani_derived.h"
#include "jani_2_fnn.h"


namespace JANI_2_FNN { const std::string error { "Jani2FNN error" }; } // NOLINT(cert-err58-cpp)

Jani2FNNParser::Jani2FNNParser(const Model& model_):
    model(model_)
    , jani2FNNFile()
    , loadedJSON(nullptr) {
}

Jani2FNNParser::~Jani2FNNParser() = default;

std::unique_ptr<Jani2FNN> Jani2FNNParser::load_file(const std::string& fnn_interface_file) {
    jani2FNNFile = fnn_interface_file;
    loadedJSON = std::make_unique<nlohmann::json>();
    if (!PARSER::parse_json_file(fnn_interface_file, *loadedJSON)) { throw ParserException(JANI_2_FNN::error); }
    std::unique_ptr<Jani2FNN> fnn_interface(new Jani2FNN(model));
    fnn_interface->interfaceFile = fnn_interface_file;
    // SYM_MODEL:
    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::fnn)) {
        fnn_interface->fnnFile = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::fnn);
    } else {
        PARSER::check_contains((*loadedJSON), JANI::FILE);
        const nlohmann::json& file = loadedJSON->at(JANI::FILE);
        PARSER::check_string(file, (*loadedJSON));
        fnn_interface->fnnFile = PLAJA_UTILS::extract_directory_path(jani2FNNFile).append(file);
    }
    
    // applicability filtering ?
    const auto app_filter_option =
        PLAJA_GLOBAL::enableApplicabilityFiltering
            ? PLAJA_GLOBAL::optionParser->get_int_option(PLAJA_OPTION::applicability_filtering)
            : 0;

    if (app_filter_option > 0) {
        fnn_interface->doApplicabilityFiltering = true;
    } else if (app_filter_option < 0) {
        fnn_interface->doApplicabilityFiltering = false;
    } else {
        PLAJA_ASSERT(app_filter_option == 0)
        fnn_interface->doApplicabilityFiltering = applicability_filtering_enabled();
    }
    // inputs:
    fnn_interface->inputFeatureMapping = parse_input_interface();
    // outputs:
    fnn_interface->outputsToLabels = parse_output_interface();
    // internals:
    fnn_interface->compute_labels_to_outputs();
    fnn_interface->compute_layer_sizes();
    fnn_interface->compute_extended_inputs();

    return fnn_interface;
}

bool Jani2FNNParser::applicability_filtering_enabled() const {
    PLAJA_ASSERT(loadedJSON); // you set it in load_file()
    if (loadedJSON->contains(JANI::FILTER)) {
        const nlohmann::json& filter = loadedJSON->at(JANI::FILTER);
        PARSER::throw_parser_exception_if(!filter.is_boolean(), filter, (*loadedJSON), PARSER::invalid);
        const bool filter_val = filter.get<bool>();
        PARSER::throw_parser_exception_if(
            !PLAJA_GLOBAL::enableApplicabilityFiltering && filter_val,
            PARSER::to_json(JANI::FILTER, filter),
            (*loadedJSON),
            PARSER::invalid
        );

        return filter_val;
    }

    return false;
}


std::vector<ActionLabel_type> Jani2FNNParser::parse_output_interface() const {
    PARSER::check_contains((*loadedJSON), JANI::OUTPUT);
    const nlohmann::json& output = loadedJSON->at(JANI::OUTPUT);
    PARSER::check_array(output, (*loadedJSON));
    PARSER::throw_parser_exception_if(output.empty(), output, (*loadedJSON), PARSER::at_least_one_element);

    // cache actions
    std::unordered_map<std::string, ActionID_type> action_name_to_id;
    for (auto it = ModelIterator::actionIterator(model); !it.end(); ++it) { action_name_to_id[it->get_name()] = it->get_id(); }

    std::vector<ActionLabel_type> output_to_labels;
    output_to_labels.reserve(output.size());
    std::unordered_set<ActionLabel_type> output_to_labels_cache; // for duplicate detection
    for (const auto& action : output) {
        PARSER::check_string(action, output);
        const std::string action_name = action.get<std::string>();
        PARSER::throw_parser_exception_if(not action_name_to_id.count(action_name), action, output, PARSER::unknown_name);
        output_to_labels.push_back(action_name_to_id.at(action_name));
        PARSER::throw_parser_exception_if(output_to_labels_cache.count(output_to_labels.back()), action, output, PARSER::duplicate_name);
        output_to_labels_cache.insert(output_to_labels.back());
    }

    return output_to_labels;
}

std::vector<std::unique_ptr<InputFeatureToJANI>> Jani2FNNParser::parse_input_interface() const {
    PARSER::check_contains((*loadedJSON), JANI::INPUT);
    const nlohmann::json& input = loadedJSON->at(JANI::INPUT);
    PARSER::check_array(input, (*loadedJSON));
    PARSER::throw_parser_exception_if(input.empty(), input, (*loadedJSON), PARSER::at_least_one_element);

    std::vector<std::unique_ptr<InputFeatureToJANI>> input_feature_mapping;
    for (const auto& input_feature_to: input) {
        input_feature_mapping.push_back(parse_InputFeatureToJANI(input_feature_to));
    }
    return input_feature_mapping;
}


std::unique_ptr<InputFeatureToJANI> Jani2FNNParser::parse_InputFeatureToJANI(const nlohmann::json& input) const {
    PARSER::check_object(input);
    if (input.contains(JANI::NAME) and input.contains(JANI::AUTOMATON)) {
        if (input.contains(JANI::INDEX)) { // ArrayVar
            return parse_InputFeatureToArrayVariable(input);
        } else { // ValVar
            return parse_InputFeatureToValueVariable(input);
        }
    } else { // Location
        return parse_InputFeatureToLocation(input);
    }
}

std::unique_ptr<InputFeatureToJANI> Jani2FNNParser::parse_InputFeatureToLocation(const nlohmann::json& input) const {
    std::unique_ptr<InputFeatureToLocation> input_feature_to(new InputFeatureToLocation(parse_automaton_instance(input))); // TODO actually problematic, model info should store the mapping invariant
    return input_feature_to;
}

// auxiliary:

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

#define PARSE_ARRAY_VARIABLE(REF, NAME, INDEX)\
    const std::size_t l = (REF).get_number_variables();\
    for (std::size_t i = 0; i < l; ++i) {\
        const VariableDeclaration* var_decl = (REF).get_variable(i);\
        if (var_decl->get_name() == (NAME)) {\
            std::unique_ptr<InputFeatureToArray> input_feature_to(new InputFeatureToArray(*var_decl, (INDEX)));\
            PARSER::throw_parser_exception_if(var_decl->get_size() < (INDEX), input, (*loadedJSON), PARSER::invalid);\
            return input_feature_to;\
        }\
    }\
    PARSER::throw_parser_exception(input, (*loadedJSON), PARSER::invalid);
//

std::unique_ptr<InputFeatureToJANI> Jani2FNNParser::parse_InputFeatureToValueVariable(const nlohmann::json& input) const {
    PARSER::check_object(input);
    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);
    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_str = input.at(JANI::AUTOMATON);
    if (automaton_str.is_null()) { // global
        PARSE_VARIABLE(model, name)
    } else {
        const Automaton& automaton = *model.get_automatonInstance(parse_automaton_instance(automaton_str));
        PARSE_VARIABLE(automaton, name)
    }

}

std::unique_ptr<InputFeatureToJANI> Jani2FNNParser::parse_InputFeatureToArrayVariable(const nlohmann::json& input) const {
    PARSER::check_object(input);
    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);
    // index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_integer() or index < 0, index, input, PARSER::invalid);
    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_str = input.at(JANI::AUTOMATON);
    if (automaton_str.is_null()) { // global
        PARSE_ARRAY_VARIABLE(model, name, index)
    } else {
        const Automaton& automaton = *model.get_automatonInstance(parse_automaton_instance(automaton_str));
        PARSE_ARRAY_VARIABLE(automaton, name, index)
    }
}

AutomatonIndex_type Jani2FNNParser::parse_automaton_instance(const nlohmann::json& input) const {
    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);

    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);

    // index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_integer() or index < 0 or model.get_number_automataInstances() <= index, index, input, PARSER::invalid);

    // check name matches index
    PARSER::throw_parser_exception_if(model.get_automatonInstance(index)->get_name() != name, input, (*loadedJSON), PARSER::invalid);

    return index;
}

std::pair<AutomatonIndex_type, EdgeID_type> Jani2FNNParser::parse_edge(const nlohmann::json& input) const {
    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);

    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const AutomatonIndex_type automaton_index = parse_automaton_instance(input.at(JANI::AUTOMATON));
    const Automaton* automaton = model.get_automatonInstance(automaton_index);

    // edge
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_unsigned() or index < 0 or automaton->get_number_edges() <= index, index, input, PARSER::invalid);

    return { automaton_index, automaton->get_edge(index)->get_id() };

}

#include "../model_information.h"

ActionOpID_type Jani2FNNParser::parse_action_operator_structure(const nlohmann::json& input) const {

    if (true) { PLAJA_ABORT }

    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);
    ActionOpStructure op_str;

    // sync index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    if (index.is_null()) {
        op_str.syncID = SYNCHRONISATION::nonSyncID;
    } else {
        PARSER::throw_parser_exception_if(not index.is_number_unsigned() or index < 0 or model.get_system()->get_number_syncs() <= index, index, input, PARSER::invalid);
        op_str.syncID = ModelInformation::sync_index_to_id(index);
    }

    // edges
    PARSER::check_contains(input, JANI::EDGES);
    const nlohmann::json& edges = input.at(JANI::EDGES);
    PARSER::check_array(edges, input);
    PARSER::throw_parser_exception_if(edges.empty(), edges, input, PARSER::at_least_one_element);
    for (const auto& edge: edges) {
        op_str.participatingEdges.push_back(parse_edge(edge));
    }

    return model.get_model_information().compute_action_op_id(op_str);
}

namespace JANI_2_FNN {
    std::unique_ptr<Jani2FNN> load_fnn_interface(const Model& model, const std::string& fnn_interface) {
        Jani2FNNParser parser(model);
        return parser.load_file(fnn_interface);
    }
}
