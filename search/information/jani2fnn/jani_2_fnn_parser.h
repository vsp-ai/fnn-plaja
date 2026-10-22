#ifndef PLAJA_JANI_2_FNN_PARSER_H
#define PLAJA_JANI_2_FNN_PARSER_H
#include <memory>
#include <string>
#include <json_fwd.hpp>
#include "../../../parser/using_parser.h"
#include "../../using_search.h"
class Model;
class Jani2FNN;
struct InputFeatureToJANI;

class Jani2FNNParser {
private:
    const Model& model;
    std::string jani2FNNFile; // file loaded
    std::unique_ptr<nlohmann::json> loadedJSON; // may be an invalid json document if load_file fails  
    
    [[nodiscard]] AutomatonIndex_type parse_automaton_instance(const nlohmann::json& input) const;
    [[nodiscard]] std::pair<AutomatonIndex_type, EdgeID_type> parse_edge(const nlohmann::json& input) const;
    [[nodiscard]] ActionOpID_type parse_action_operator_structure(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToJANI(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToLocation(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToValueVariable(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToArrayVariable(const nlohmann::json& input) const;
    [[nodiscard]] bool applicability_filtering_enabled() const;
    [[nodiscard]] std::vector<unsigned int> parse_hidden_layer_structure() const;
public:
    explicit Jani2FNNParser(const Model& model_);
    ~Jani2FNNParser();
    std::unique_ptr<Jani2FNN> load_file(const std::string& fnn_interface_file);
    [[nodiscard]] std::vector<std::unique_ptr<InputFeatureToJANI>> parse_input_interface() const;
    [[nodiscard]] std::vector<ActionLabel_type> parse_output_interface() const;
    
    
};

namespace JANI_2_FNN {
    extern std::unique_ptr<Jani2FNN> load_fnn_interface(const Model& model, const std::string& fnn_interface_file);
}

#endif //PLAJA_JANI_2_FNN_PARSER_H
