//
// This file is part of the PlaJA code base.
//

#include "policies/fnn/fnn_reader.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <json.hpp>

#include "core/exception/parser_exception.h"
#include "assertions.h"
#include "model/ast/model.h"
#include "model/ast/expression/array_access_expression.h"
#include "model/ast/expression/variable_expression.h"
#include "policies/interface/jani_2_interface.h"

namespace PLAJA {
    namespace FNN_PARSER {

        namespace {

        using json = nlohmann::json;

        struct RuleTmp {
            std::string name;
            std::string type;               // "AND", "OR" or "NOT"
            std::vector<std::string> preds; // predecessor node names
        };

        std::vector<std::string> read_atoms_spec(const json& j) {
            if (not j.contains("atoms") or not j["atoms"].is_object()) {
                throw ParserException("[FNN] sym_model.json must contain an 'atoms' object with type/len/content.");
            }

            const auto& atoms_obj = j["atoms"];
            if (not atoms_obj.contains("content") or not atoms_obj["content"].is_array()) {
                throw ParserException("[FNN] atoms.content missing or not an array.");
            }
            if (not atoms_obj.contains("type") or not atoms_obj["type"].is_string()) {
                throw ParserException("[FNN] atoms.type missing or not a string (expected 'onehot' or 'predicates').");
            }
            if (not atoms_obj.contains("len") or not atoms_obj["len"].is_number_integer()) {
                throw ParserException("[FNN] atoms.len missing or not an integer.");
            }

            auto atoms = atoms_obj["content"].get<std::vector<std::string>>();
            const auto declared_len = atoms_obj["len"].get<std::size_t>();
            if (declared_len != atoms.size()) {
                throw ParserException("[FNN] atoms.len does not match atoms.content size.");
            }

            const auto atoms_type = atoms_obj["type"].get<std::string>();
            if (atoms_type != "onehot" and atoms_type != "predicates") {
                throw ParserException("[FNN] atoms.type must be 'onehot' or 'predicates'. Got: " + atoms_type);
            }

            return atoms;
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
                if (op.is_object() and op.contains("NOT") and op["NOT"].is_string()) {
                    const auto payload = op["NOT"].get<std::string>();
                    const auto not_name = "NOT(" + payload + ")";
                    if (seen_not_nodes.insert(not_name).second) {
                        RuleTmp aux;
                        aux.name = not_name;
                        aux.type = "NOT";
                        aux.preds = {payload};
                        aux_not_rules.push_back(std::move(aux));
                    }
                    preds.push_back(not_name);
                    continue;
                }
                throw ParserException("[FNN] Rule " + rule_name +
                                      " has an operand that is neither a string nor {\"NOT\": <string>}.");
            }
            return preds;
        }

        std::vector<RuleTmp> read_rules(const json& j) {
            if (not j.contains("rules") or not j["rules"].is_object()) {
                throw ParserException("[FNN] sym_model.json missing 'rules' object.");
            }

            std::vector<RuleTmp> aux_not_rules;
            std::unordered_set<std::string> seen_not_nodes;
            std::vector<RuleTmp> rules;
            for (auto& entry : j["rules"].items()) {
                const std::string& rule_name = entry.key();
                const json& node_json = entry.value();

                RuleTmp r;
                r.name = rule_name;

                if (node_json.contains("AND")) {
                    r.type = "AND";
                    r.preds = read_operands(node_json["AND"], rule_name, aux_not_rules, seen_not_nodes);
                } else if (node_json.contains("OR")) {
                    r.type = "OR";
                    r.preds = read_operands(node_json["OR"], rule_name, aux_not_rules, seen_not_nodes);
                } else {
                    throw ParserException("[FNN] Rule " + rule_name + " has neither AND nor OR.");
                }

                rules.push_back(std::move(r));
            }
            rules.insert(rules.end(),
                         std::make_move_iterator(aux_not_rules.begin()),
                         std::make_move_iterator(aux_not_rules.end()));
            return rules;
        }

        struct ParsedLinearPredicate {
            std::vector<std::pair<std::string, PLAJA::floating>> terms;
            PLAJA::PredicateOp op;
            PLAJA::floating constant;
        };

        std::string strip_kind_prefix(const std::string& atom) {
            const auto pos = atom.find(':');
            if (pos == std::string::npos) { return atom; }
            const auto after = atom.substr(pos + 1);
            if (after.find(' ') != std::string::npos) { return after; }
            return atom;
        }

        bool parse_linear_predicate(const std::string& atom, ParsedLinearPredicate& out) {
            std::string s = strip_kind_prefix(atom);
            std::istringstream iss(s);
            std::vector<std::string> tokens;
            std::string tok;
            while (iss >> tok) { tokens.push_back(tok); }
            if (tokens.size() < 3) { return false; }

            static const std::unordered_map<std::string, PLAJA::PredicateOp> ops {
                {"eq", PLAJA::PredicateOp::EQ}, {"neq", PLAJA::PredicateOp::NEQ}, {"lt", PLAJA::PredicateOp::LT},
                {"leq", PLAJA::PredicateOp::LEQ}, {"gt", PLAJA::PredicateOp::GT}, {"geq", PLAJA::PredicateOp::GEQ},
                {"==", PLAJA::PredicateOp::EQ}, {"!=", PLAJA::PredicateOp::NEQ}, {"<", PLAJA::PredicateOp::LT},
                {"<=", PLAJA::PredicateOp::LEQ}, {">", PLAJA::PredicateOp::GT}, {">=", PLAJA::PredicateOp::GEQ}
            };
            std::size_t op_idx = tokens.size();
            for (std::size_t i = 0; i < tokens.size(); ++i) {
                if (ops.count(tokens[i])) { op_idx = i; break; }
            }
            if (op_idx + 1 >= tokens.size()) { return false; }
            if (op_idx < 1) { return false; }

            out.op = ops.at(tokens[op_idx]);
            const std::string& constant_str = tokens[op_idx + 1];
            char* end = nullptr;
            const double val = std::strtod(constant_str.c_str(), &end);
            if (*end != '\0') { return false; }
            out.constant = static_cast<PLAJA::floating>(val);
            out.terms.clear();

            int sign = +1;
            for (std::size_t i = 0; i < op_idx; ++i) {
                const std::string& t = tokens[i];
                if (t == "+") { sign = +1; continue; }
                if (t == "-") { sign = -1; continue; }

                int term_sign = sign;
                sign = +1;

                std::string term = t;
                if (term[0] == '+' or term[0] == '-') {
                    if (term[0] == '-') { term_sign *= -1; }
                    term = term.substr(1);
                }

                PLAJA::floating coeff = 1.0;
                std::string var;
                const auto star = term.find('*');
                if (star != std::string::npos) {
                    const std::string coeff_str = term.substr(0, star);
                    var = term.substr(star + 1);
                    char* end_coeff = nullptr;
                    const double c = std::strtod(coeff_str.c_str(), &end_coeff);
                    if (*end_coeff != '\0') { return false; }
                    coeff = static_cast<PLAJA::floating>(c);
                } else {
                    var = term;
                }
                if (var.empty()) { return false; }

                coeff *= static_cast<PLAJA::floating>(term_sign);
                out.terms.emplace_back(var, coeff);
            }

            return not out.terms.empty();
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
                node.name = atom_name;

                fnnNodes.push_back(node);
                name_to_idx[node.name] = next_idx++;

                // Negations arrive folded into the operator ("x neq 3"); a
                // leading '~' is a legacy export whose negation
                // strip_kind_prefix would silently swallow - reject it.
                if (not atom_name.empty() and atom_name[0] == '~') {
                    throw ParserException("[FNN] Legacy '~'-prefixed predicate atom '" + node.name +
                                          "' is not supported: re-export the model with the "
                                          "current rrl (negated operator instead of '~').");
                }

                ParsedLinearPredicate parsed;
                if (not parse_linear_predicate(node.name, parsed)) {
                    throw ParserException("[FNN] Bad predicate atom: " + node.name);
                }
                PLAJA::BinFeature bin;
                bin.op = parsed.op;
                bin.constant = parsed.constant;
                for (const auto& term : parsed.terms) {
                    const auto it_state = var_to_state_index.find(term.first);
                    if (it_state == var_to_state_index.end()) {
                        throw ParserException("[FNN] Predicate variable not found in inputs: " + term.first);
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
            while (not pending_rules.empty() and progress) {
                progress = false;

                for (std::size_t i = 0; i < pending_rules.size(); ) {
                    RuleTmp& r = pending_rules[i];
                    std::vector<int> pred_indices;
                    pred_indices.reserve(r.preds.size());
                    bool all_known = true;
                    for (const auto& pred_name : r.preds) {
                        auto pr = name_to_idx.find(pred_name);
                        if (pr == name_to_idx.end()) {
                            all_known = false;
                            break;
                        }
                        pred_indices.push_back(pr->second);
                    }

                    if (not all_known) {
                        ++i;
                        continue;
                    }

                    PLAJA::FNNNode node;
                    node.type = (r.type == "AND" ? PLAJA::NodeType::AND
                                 : r.type == "NOT" ? PLAJA::NodeType::NOT
                                                   : PLAJA::NodeType::OR);
                    node.preds = std::move(pred_indices);
                    node.name = r.name;

                    fnnNodes.push_back(node);
                    name_to_idx[node.name] = next_idx++;
                    pending_rules[i] = std::move(pending_rules.back());
                    pending_rules.pop_back();
                    progress = true;
                }
            }

            if (not pending_rules.empty()) {
                throw ParserException("[FNN] Some logical rules could not be topologically ordered.");
            }
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
            if (not j.contains("linear") or not j["linear"].is_object()) {
                throw ParserException("[FNN] sym_model.json missing 'linear' object.");
            }
            if (not j["linear"].contains("weights") or not j["linear"]["weights"].is_object()) {
                throw ParserException("[FNN] linear.weights missing or not an object.");
            }
            if (not j["linear"].contains("bias") or not j["linear"]["bias"].is_object()) {
                throw ParserException("[FNN] linear.bias missing or not an object.");
            }

            if (j["linear"]["weights"].size() != outputs_size) {
                throw ParserException("[FNN] linear.weights size does not match outputs size.");
            }
            if (j["linear"]["bias"].size() != outputs_size) {
                throw ParserException("[FNN] linear.bias size does not match outputs size.");
            }

            linearSize = static_cast<unsigned int>(outputs_size);
            auto bias = j["linear"]["bias"].get<std::unordered_map<std::string,double>>();
            std::unordered_map<std::string, std::unordered_map<std::string,double>> weights;
            for (auto& cls_entry : j["linear"]["weights"].items()) {
                const std::string& cls_name = cls_entry.key();
                for (auto& w_entry : cls_entry.value().items()) {
                    const std::string& node_name = w_entry.key();
                    double w = w_entry.value().get<double>();
                    weights[cls_name][node_name] = w;
                }
            }

            initLinearIdx = next_idx;
            linearToOutput.assign(linearSize, -1);
            int linear_idx = 0;
            for (const auto& kv : weights) {
                const std::string& cls_name = kv.first;
                const auto& wmap = kv.second;
                PLAJA::FNNNode node;
                node.type = PLAJA::NodeType::LINEAR;
                node.name = cls_name;

                const auto out_idx_it = output_name_to_index.find(node.name);
                if (out_idx_it == output_name_to_index.end()) {
                    throw ParserException("[FNN] Linear output '" + node.name
                                          + "' does not match any action name from the FNN interface output list. "
                                          + "This likely indicates a deprecated sym_model format; please re-export "
                                          + "the symbolic model with action labels matching the policy labels.");
                }
                const int out_idx = out_idx_it->second;
                PLAJA_ASSERT(linear_idx == next_idx - initLinearIdx)
                linearToOutput[linear_idx++] = out_idx;

                auto b_it = bias.find(cls_name);
                if (b_it == bias.end()) {
                    throw ParserException("[FNN] Missing linear bias for class: " + cls_name);
                }
                node.bias = b_it->second;

                for (const auto& wkv : wmap) {
                    const std::string& rule_name = wkv.first;
                    double w = wkv.second;

                    auto it_rule = name_to_idx.find(rule_name);
                    if (it_rule == name_to_idx.end()) {
                        throw ParserException("[FNN] Linear weight references unknown rule: " + rule_name);
                    }
                    node.preds.push_back(it_rule->second);
                    node.weights.push_back(w);
                }
                fnnNodes.push_back(node);
                name_to_idx[cls_name] = next_idx++;
            }
            if (static_cast<std::size_t>(linear_idx) != outputs_size) {
                throw ParserException("[FNN] Linear layer size mismatch after parsing.");
            }
        }

        std::unordered_map<std::string, VariableIndex_type> build_var_to_state_index_map(const Jani2Interface& iface) {
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
                    if (not base or not idx or not idx->is_constant()) { continue; }
                    const auto idx_val = idx->evaluateInteger(nullptr);
                    map[base->get_name() + "_" + std::to_string(idx_val)] = it.get_input_feature_index();
                }
            }
            return map;
        }

        } // namespace

        std::unique_ptr<FormularizedNetwork> read_fnn(const std::string& path, const Jani2Interface& interface) {
            std::cout << "[FNN] Loading FormularizedNetwork from: " << path << std::endl;
            std::ifstream json_file(path);
            if (not json_file.is_open()) { throw ParserException("Failed to open the JSON file: " + path); }
            json j;
            json_file >> j;

            std::vector<FNNNode> nodes;
            std::vector<BinFeature> bin_features;
            std::vector<int> linear_to_output;
            unsigned int linear_size;
            int init_linear_idx;

            std::unordered_map<std::string, int> name_to_idx;
            const auto atoms = read_atoms_spec(j);
            auto pending_rules = read_rules(j);
            const auto num_outputs = interface.get_num_output_features();
            std::unordered_map<std::string, int> output_name_to_index;
            output_name_to_index.reserve(num_outputs);
            for (OutputIndex_type oi = 0; oi < num_outputs; ++oi) {
                output_name_to_index.emplace(interface.get_model().get_action_name(interface.get_output_label(oi)), static_cast<int>(oi));
            }

            nodes.reserve(atoms.size() + pending_rules.size() + num_outputs);

            int next_idx = 0;
            // Onehot and predicates atoms share one representation: a linear
            // predicate per BIN node (onehot atoms are the single-term eq/neq
            // special case).
            bin_features.reserve(atoms.size());
            auto var_to_state_index = build_var_to_state_index_map(interface);
            append_bin_features(atoms, var_to_state_index, nodes, name_to_idx, bin_features, next_idx);

            const int init_logical_idx = next_idx;
            append_rules_topologically(pending_rules, name_to_idx, nodes, next_idx);
            append_linear_nodes(j, num_outputs, output_name_to_index, name_to_idx, nodes, linear_to_output, linear_size, init_linear_idx, next_idx);
            return std::make_unique<FormularizedNetwork>(std::move(nodes), std::move(bin_features), init_logical_idx, init_linear_idx, linear_size, std::move(linear_to_output));
        }

    } // namespace FNN_PARSER
} // namespace PLAJA
