//
// Helpers for parsing symbolic FNN (sym_model) JSON files.
//
#include "sym_model_parser.h"

#include <iostream>
#include <iterator>
#include <unordered_set>

#include "../../exception/parser_exception.h"
#include "../../assertions.h"
#include "predicate_atom_parser.h"

namespace PLAJA {
    namespace FNN_PARSER {

        using json = nlohmann::json;

        [[noreturn]] void fail(const std::string& msg) {
            std::cerr << msg << "\n";
            throw ParserException(msg);
        }

        AtomSpec read_atoms_spec(const json& j) {
            if (!j.contains("atoms") || !j["atoms"].is_object()) {
                fail("[FNN] sym_model.json must contain an 'atoms' object with type/len/content.");
            }

            const auto& atoms_obj = j["atoms"];
            if (!atoms_obj.contains("content") || !atoms_obj["content"].is_array()) {
                fail("[FNN] atoms.content missing or not an array.");
            }
            if (!atoms_obj.contains("type") || !atoms_obj["type"].is_string()) {
                fail("[FNN] atoms.type missing or not a string (expected 'onehot' or 'predicates').");
            }
            if (!atoms_obj.contains("len") || !atoms_obj["len"].is_number_integer()) {
                fail("[FNN] atoms.len missing or not an integer.");
            }

            AtomSpec spec;
            spec.atoms = atoms_obj["content"].get<std::vector<std::string>>();
            const auto declared_len = atoms_obj["len"].get<std::size_t>();
            if (declared_len != spec.atoms.size()) {
                fail("[FNN] atoms.len does not match atoms.content size.");
            }
            PLAJA_ASSERT(declared_len == spec.atoms.size());

            const auto atoms_type = atoms_obj["type"].get<std::string>();
            if (atoms_type == "onehot") {
                spec.is_onehot = true;
            } else if (atoms_type == "predicates") {
                spec.is_onehot = false;
            } else {
                fail("[FNN] atoms.type must be 'onehot' or 'predicates'. Got: " + atoms_type);
            }

            return spec;
        }

        // Rule operands are atom/rule name strings, or {"NOT": "<name>"} objects
        // for negative-polarity connections between logical layers. Each NOT
        // becomes a reference to an auxiliary single-predecessor NOT node
        // "NOT(<name>)" (created once, reused everywhere); atom-level negations
        // are expressed by neq atoms instead.
        std::vector<std::string> read_operands(
            const json& arr,
            const std::string& rule_name,
            std::vector<RuleTmp>& aux_not_rules,
            std::unordered_set<std::string>& seen_not_nodes
        ) {
            std::vector<std::string> preds;
            preds.reserve(arr.size());
            for (const auto& op : arr) {
                if (op.is_string()) {
                    preds.push_back(op.get<std::string>());
                    continue;
                }
                if (op.is_object() && op.contains("NOT") && op["NOT"].is_string()) {
                    const auto payload = op["NOT"].get<std::string>();
                    const auto not_name = "NOT(" + payload + ")";
                    if (seen_not_nodes.insert(not_name).second) {
                        RuleTmp aux;
                        aux.name  = not_name;
                        aux.type  = "NOT";
                        aux.preds = {payload};
                        aux_not_rules.push_back(std::move(aux));
                    }
                    preds.push_back(not_name);
                    continue;
                }
                fail("[FNN] Rule " + rule_name +
                     " has an operand that is neither a string nor {\"NOT\": <string>}.");
            }
            return preds;
        }

        std::vector<RuleTmp> read_rules(const json& j) {
            if (!j.contains("rules") || !j["rules"].is_object()) {
                fail("[FNN] sym_model.json missing 'rules' object.");
            }

            std::vector<RuleTmp> aux_not_rules;
            std::unordered_set<std::string> seen_not_nodes;
            std::vector<RuleTmp> rules;
            for (auto &entry : j["rules"].items()) {
                const std::string &rule_name = entry.key();
                const json &node_json = entry.value();

                RuleTmp r;
                r.name = rule_name;

                if (node_json.contains("AND")) {
                    r.type  = "AND";
                    r.preds = read_operands(node_json["AND"], rule_name, aux_not_rules, seen_not_nodes);
                } else if (node_json.contains("OR")) {
                    r.type  = "OR";
                    r.preds = read_operands(node_json["OR"], rule_name, aux_not_rules, seen_not_nodes);
            } else {
                fail("[FNN] Rule " + rule_name + " has neither AND nor OR.");
            }

                rules.push_back(std::move(r));
            }
            rules.insert(rules.end(),
                         std::make_move_iterator(aux_not_rules.begin()),
                         std::make_move_iterator(aux_not_rules.end()));
            return rules;
        }

        void append_bin_features(
            const std::vector<std::string>& atoms,
            const std::unordered_map<std::string, VariableIndex_type>& var_to_state_index,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::BinFeature>& binFeatures,
            int& next_idx
        ) {
            for (const auto& atom_name : atoms) {
                PLAJA::FNNNode node;
                node.type = PLAJA::NodeType::BIN;
                node.preds.clear();
                node.weights.clear();
                node.name  = atom_name;

                fnnNodes.push_back(node);
                name_to_idx[node.name] = next_idx++;

                // Negations arrive folded into the operator ("x neq 3"); a
                // leading '~' is a legacy export whose negation
                // strip_kind_prefix would silently swallow - reject it.
                if (!atom_name.empty() && atom_name[0] == '~') {
                    fail("[FNN] Legacy '~'-prefixed predicate atom '" + node.name +
                         "' is not supported: re-export the model with the "
                         "current rrl (negated operator instead of '~').");
                }

                ParsedLinearPredicate parsed;
                if (!parse_linear_predicate(node.name, parsed)) {
                    fail("[FNN] Bad predicate atom: " + node.name);
                }
                PLAJA::PredicateOp op;
                if (!parse_predicate_op(parsed.op, op)) {
                    fail("[FNN] Unsupported predicate op: " + parsed.op);
                }

                PLAJA::BinFeature bin;
                bin.op = op;
                bin.constant = parsed.constant;
                for (const auto& term : parsed.terms) {
                    const auto it_state = var_to_state_index.find(term.first);
                    if (it_state == var_to_state_index.end()) {
                        fail("[FNN] Predicate variable not found in inputs: " + term.first);
                    }
                    bin.terms.push_back({it_state->second, term.second});
                }
                binFeatures.push_back(std::move(bin));
            }
        }

        void append_rules_topologically(
            std::vector<RuleTmp>& pending_rules,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            int& next_idx
        ) {
            bool progress = true;
            while (!pending_rules.empty() && progress) {
                progress = false;

                for (std::size_t i = 0; i < pending_rules.size(); ) {
                    RuleTmp &r = pending_rules[i];
                    std::vector<int> pred_indices;
                    pred_indices.reserve(r.preds.size());
                    bool all_known = true;
                    for (const auto &pred_name : r.preds) {
                        auto pr = name_to_idx.find(pred_name);
                        if (pr == name_to_idx.end()) {
                            all_known = false;
                            break;
                        }
                        pred_indices.push_back(pr->second);
                    }

                    if (!all_known) {
                        ++i;
                        continue;
                    }

                    PLAJA::FNNNode node;
                    node.type   = (r.type == "AND" ? PLAJA::NodeType::AND
                                   : r.type == "NOT" ? PLAJA::NodeType::NOT
                                                     : PLAJA::NodeType::OR);
                    node.preds.clear();
                    node.weights.clear();
                    node.preds  = std::move(pred_indices);
                    node.name   = r.name;

                    fnnNodes.push_back(node);
                    name_to_idx[node.name] = next_idx++;
                    pending_rules[i] = std::move(pending_rules.back());
                    pending_rules.pop_back();
                    progress = true;
                }
            }

            if (!pending_rules.empty()) {
                fail("[FNN] Some logical rules could not be topologically ordered.");
            }
            PLAJA_ASSERT(pending_rules.empty());
        }

        void append_linear_nodes(
            const json& j,
            std::size_t outputs_size,
            const std::unordered_map<std::string, int>& output_name_to_index,
            std::unordered_map<std::string, int>& name_to_idx,
            std::vector<PLAJA::FNNNode>& fnnNodes,
            std::vector<int>& linearToOutput,
            unsigned int& linearSize,
            int& initLinearIdx,
            int& next_idx
        ) {
            if (!j.contains("linear") || !j["linear"].is_object()) {
                fail("[FNN] sym_model.json missing 'linear' object.");
            }
            if (!j["linear"].contains("weights") || !j["linear"]["weights"].is_object()) {
                fail("[FNN] linear.weights missing or not an object.");
            }
            if (!j["linear"].contains("bias") || !j["linear"]["bias"].is_object()) {
                fail("[FNN] linear.bias missing or not an object.");
            }

            if (j["linear"]["weights"].size() != outputs_size) {
                fail("[FNN] linear.weights size does not match outputs size.");
            }
            if (j["linear"]["bias"].size() != outputs_size) {
                fail("[FNN] linear.bias size does not match outputs size.");
            }
            PLAJA_ASSERT(j["linear"]["weights"].size() == outputs_size);
            PLAJA_ASSERT(j["linear"]["bias"].size()    == outputs_size);

            linearSize = static_cast<unsigned int>(outputs_size);
            auto bias = j["linear"]["bias"].get<std::unordered_map<std::string,double>>();
            std::unordered_map<std::string, std::unordered_map<std::string,double>> weights;
            for (auto &cls_entry : j["linear"]["weights"].items()) {
                const std::string &cls_name = cls_entry.key();
                for (auto &w_entry : cls_entry.value().items()) {
                    const std::string &node_name = w_entry.key();
                    double w = w_entry.value().get<double>();
                    weights[cls_name][node_name] = w;
                }
            }

            initLinearIdx = next_idx;
            linearToOutput.assign(linearSize, -1);
            int linear_idx = 0;
            for (const auto &kv : weights) {
                const std::string &cls_name = kv.first;
                const auto &wmap = kv.second;
                PLAJA::FNNNode node;
                node.type = PLAJA::NodeType::LINEAR;
                node.preds.clear();
                node.weights.clear();
                node.name  = cls_name;

                const auto out_idx_it = output_name_to_index.find(node.name);
                if (out_idx_it == output_name_to_index.end()) {
                    fail("[FNN] Linear output '" + node.name
                         + "' does not match any action name from the FNN interface output list. "
                         + "This likely indicates a deprecated sym_model format; please re-export "
                         + "the symbolic model with action labels matching jani2nnet outputs.");
                }
                const int out_idx = out_idx_it->second;
                PLAJA_ASSERT(linear_idx == next_idx - initLinearIdx);
                linearToOutput[linear_idx++] = out_idx;

                auto b_it = bias.find(cls_name);
                if (b_it == bias.end()) {
                    fail("[FNN] Missing linear bias for class: " + cls_name);
                }
                PLAJA_ASSERT(b_it != bias.end());
                node.bias = static_cast<float>(b_it->second);
                
                for (const auto &wkv : wmap) {
                    const std::string &rule_name = wkv.first;
                    double w = wkv.second;

                    auto it_rule = name_to_idx.find(rule_name);
                    if (it_rule == name_to_idx.end()) {
                        fail("[FNN] Linear weight references unknown rule: " + rule_name);
                    }
                    node.preds.push_back(it_rule->second);
                    node.weights.push_back(static_cast<float>(w));
                }
                fnnNodes.push_back(node);
                name_to_idx[cls_name] = next_idx++;
            }
            if (linearSize != outputs_size || static_cast<std::size_t>(linear_idx) != outputs_size) {
                fail("[FNN] Linear layer size mismatch after parsing.");
            }
            PLAJA_ASSERT(linearSize == outputs_size);
            PLAJA_ASSERT(static_cast<std::size_t>(linear_idx) == outputs_size);
        }

    } // namespace FNN_PARSER
} // namespace PLAJA
