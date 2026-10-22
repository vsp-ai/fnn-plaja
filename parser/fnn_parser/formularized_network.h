
#ifndef PLAJA_FORMULARIZED_NETWORK_H
#define PLAJA_FORMULARIZED_NETWORK_H


#include <string>
#include <utility>
#include <vector>

#include "../using_parser.h"
#include "predicate_atom_parser.h"
#include "../../search/states/forward_states.h" //StateBase



class Jani2FNN;
namespace PLAJA {
    // NOT: auxiliary negation node with exactly one predecessor,
    //      value = 1 - predecessor. Created for {"NOT": <rule>} operands.
    enum class NodeType { BIN, AND, OR, NOT, LINEAR };
    struct FNNNode {
        NodeType type;
        std::vector<int> preds;
        std::vector<float> weights; // info for LINEAR nodes
        float bias = 0.0f;          // info for LINEAR nodes
        std::string name;
    };
    struct BinFeatureTerm {
        VariableIndex_type stateIndex;
        PLAJA::floating coeff;
    };
    // One BIN node of the binarization layer: a linear predicate over state
    // variables. Onehot atoms ("x eq 3", "x neq 3") are the single-term
    // integer special case of the same shape.
    struct BinFeature {
        std::vector<BinFeatureTerm> terms;
        PredicateOp op;
        PLAJA::floating constant;
    };
    class FormularizedNetwork {
        friend ::Jani2FNN;
    private:
        std::vector<FNNNode> fnnNodes; // structure info
        int initLogicalIdx; // structure info: index of first logical (AND/OR) node (separates from BIN nodes)
        int initLinearIdx;  // structure info: index of first linear node (separates from logical nodes)
        unsigned int linearSize;
        // IMPORTANT NOTE:  linearToOutput maps linear node index (i - initLinearIdx) to output index 
        //                  which can be used to access outputsToLabels
        // IMPORTANT NOTE2: this is a hacky temporary solution, in the future the class name in the FNN json should directly map to action labels.
        std::vector<int> linearToOutput; // linear_idx --> output index (in this->outputsToLabels)
        std::vector<int> inputFeatures; // map which features in the state are relevant to the FNN
        std::vector<BinFeature> binFeatures; // one entry per BIN node
        bool topologicallySorted = false;
        mutable std::vector<float> nodeActivations;
        void binarize_features(const StateBase& state) const;
        std::vector<double> forward_pass() const;
    public:
        explicit FormularizedNetwork(const std::string& path);
        ~FormularizedNetwork();
        std::vector<double> evaluate(const StateBase& state) const;

        // Accessors for SMT encoding / analysis.
        [[nodiscard]] inline const std::vector<FNNNode>& get_nodes() const { return fnnNodes; }
        [[nodiscard]] inline int get_init_logical_idx() const { return initLogicalIdx; }
        [[nodiscard]] inline int get_init_linear_idx() const { return initLinearIdx; }
        [[nodiscard]] inline unsigned int get_linear_size() const { return linearSize; }
        [[nodiscard]] inline const std::vector<int>& get_linear_to_output() const { return linearToOutput; }
        [[nodiscard]] inline const std::vector<BinFeature>& get_bin_features() const { return binFeatures; }
        [[nodiscard]] inline bool is_topologically_sorted_() const { return topologicallySorted; }
        
    };

}

#endif //PLAJA_FORMULARIZED_NETWORK_H
