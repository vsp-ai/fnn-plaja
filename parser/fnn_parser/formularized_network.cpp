#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <iostream>

#include "formularized_network.h"
#include "../../exception/parser_exception.h"
#include "../../utils/utils.h"
#include "../../utils/floating_utils.h"
#include "../../search/states/state_base.h"

PLAJA::FormularizedNetwork::FormularizedNetwork(const std::string& path) {
    std::cout << "[FNN] Loading FormularizedNetwork from: " << path << std::endl;
}

PLAJA::FormularizedNetwork::~FormularizedNetwork() = default;

std::vector<double> PLAJA::FormularizedNetwork::forward_pass() const {
    std::vector<double> outputs(this->linearSize, 0.0f);

    std::fill(outputs.begin(), outputs.end(), -std::numeric_limits<float>::infinity());
    for (int i = initLogicalIdx; i < static_cast<int>(fnnNodes.size()); ++i) {
        const PLAJA::FNNNode &node = fnnNodes[i];
        switch (node.type) {
            case PLAJA::NodeType::AND: {
                float val = 1.0f;
                for (int p : node.preds) {
                    if (nodeActivations[p] <= 0.0f) {
                        val = 0.0f;
                        break;
                    }
                }
                nodeActivations[i] = val;
                break;
            }

            case PLAJA::NodeType::OR: {
                float val = 0.0f;
                for (int p : node.preds) {
                    if (nodeActivations[p] > 0.0f) {
                        val = 1.0f;
                        break;
                    }
                }
                nodeActivations[i] = val;
                break;
            }

            case PLAJA::NodeType::NOT: {
                PLAJA_ASSERT(node.preds.size() == 1);
                nodeActivations[i] = (nodeActivations[node.preds[0]] > 0.0f) ? 0.0f : 1.0f;
                break;
            }
            case PLAJA::NodeType::LINEAR:{
                nodeActivations[i] = 0.0f; //reset it first.
                float acc = node.bias;     // bias term
                for (int k = 0; k < node.preds.size(); ++k) {
                    int p = node.preds[k];
                    acc += node.weights[k] * nodeActivations[p];
                }
                nodeActivations[i] = acc;
                // IMPORTANT NOTE: linearToOutput maps linear node index (i - initLinearIdx) to output index 
                //                 which can be used to access outputsToLabels
                int linear_idx = i - initLinearIdx;
                outputs[linearToOutput[linear_idx]] = nodeActivations[i];
                break;
            }
            default:
                PLAJA_ASSERT(false);
        }
    }
    return outputs;
}

void PLAJA::FormularizedNetwork::binarize_features(const StateBase& state) const {
    PLAJA_ASSERT(binFeatures.size() == static_cast<std::size_t>(initLogicalIdx));

    std::fill(nodeActivations.begin(), nodeActivations.end(), 0.0f);
    for (std::size_t i = 0; i < binFeatures.size(); ++i) {
        const auto& p = binFeatures[i];
        PLAJA::floating val = 0.0;
        for (const auto& term : p.terms) {
            val += static_cast<PLAJA::floating>(state[term.stateIndex]) * term.coeff;
        }
        bool truth = false;
        switch (p.op) {
            case PLAJA::PredicateOp::EQ:  truth = PLAJA_FLOATS::equal(val, p.constant, PLAJA::floatingPrecision); break;
            case PLAJA::PredicateOp::NEQ: truth = PLAJA_FLOATS::non_equal(val, p.constant, PLAJA::floatingPrecision); break;
            case PLAJA::PredicateOp::LT:  truth = PLAJA_FLOATS::lt(val, p.constant, PLAJA::floatingPrecision); break;
            case PLAJA::PredicateOp::LEQ: truth = PLAJA_FLOATS::lte(val, p.constant, PLAJA::floatingPrecision); break;
            case PLAJA::PredicateOp::GT:  truth = PLAJA_FLOATS::gt(val, p.constant, PLAJA::floatingPrecision); break;
            case PLAJA::PredicateOp::GEQ: truth = PLAJA_FLOATS::gte(val, p.constant, PLAJA::floatingPrecision); break;
            default: PLAJA_ASSERT(false);
        }
        nodeActivations[i] = truth ? 1.0f : 0.0f;
    }
}

std::vector<double> PLAJA::FormularizedNetwork::evaluate(const StateBase& state) const {
    binarize_features(state);
    return forward_pass();
}
