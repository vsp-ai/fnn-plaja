//
// Helpers for parsing symbolic FNN (sym_model) JSON files.
//
#ifndef PLAJA_SYM_MODEL_PARSER_H
#define PLAJA_SYM_MODEL_PARSER_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <json.hpp>

#include "formularized_network.h"

namespace PLAJA {
    namespace FNN_PARSER {

        struct AtomSpec {
            std::vector<std::string> atoms;
            bool is_onehot = false;
        };

        struct RuleTmp {
            std::string name;
            std::string type;               // "AND" or "OR"
            std::vector<std::string> preds; // predecessor node names
        };

        AtomSpec read_atoms_spec(const nlohmann::json& j);
        std::vector<RuleTmp> read_rules(const nlohmann::json& j);

        void append_bin_features(
            const std::vector<std::string>& atoms,
            const std::unordered_map<std::string, VariableIndex_type>& var_to_state_index,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::BinFeature>& binFeatures,
            int& next_idx
        );

        void append_rules_topologically(
            std::vector<RuleTmp>& pending_rules,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            int& next_idx
        );

        void append_linear_nodes(
            const nlohmann::json& j,
            std::size_t outputs_size,
            const std::unordered_map<std::string, int>& output_name_to_index,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            std::vector<int>& linearToOutput,
            unsigned int& linearSize,
            int& initLinearIdx,
            int& next_idx
        );

    } // namespace FNN_PARSER
} // namespace PLAJA

#endif // PLAJA_SYM_MODEL_PARSER_H
