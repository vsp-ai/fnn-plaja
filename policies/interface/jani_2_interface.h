//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_JANI_2_INTERFACE_H
#define PLAJA_JANI_2_INTERFACE_H
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "model/ast/expression/forward_expression.h"
#include "model/ast/forward_ast.h"
#include "assertions.h"
#include "core/forward_factories.h"
#include "policies/forward_policy.h"
#include "core/forward_states.h"
#include "model/forward_information.h"
#include "policies/interface/input_feature_to_jani.h"
#include "policies/interface/input_feature_to_jani_derived.h"
#include "model/model_information.h"
#include "model/ast/model.h"
#include "policies/using_nn.h"
#include "core/using_core.h"

class Jani2InterfaceParser;

/**
 * This class documents the interface between a JANI model and an underlying policy stored in .nnet or.json format.
 * The interface is dependent on the current implementation of JANI models, \ie, the used indexes and ids.
 */
class Jani2Interface {
public:
    const Model* model; // underlying model
    bool doApplicabilityFiltering;
    std::vector<unsigned int> layerSizes;
    std::vector<std::unique_ptr<InputFeatureToJANI>> inputFeatureMapping; // For applicability filtering any variable occurring in a guard is to be considered an input to the policy (inputFeatureMapping.size() >= layerSizes[0]).
    std::vector<ActionLabel_type> outputsToLabels;
    std::unordered_map<ActionLabel_type, OutputIndex_type> labelsToOutputs;

protected:
    virtual void compute_layer_sizes() = 0;

public:
    explicit Jani2Interface(const Model& model);
    virtual ~Jani2Interface() = default;  // Make the destructor virtual

    virtual const Policy& load_policy(const PLAJA::Configuration& config) const = 0;
    
    FCT_IF_DEBUG(virtual ActionLabel_type eval_policy(const StateBase& state) const = 0;)
    FCT_IF_DEBUG(virtual bool is_chosen(const StateBase& state, ActionLabel_type action_label) const = 0;)

    [[nodiscard]] inline const Model& get_model() const {return *model; }

    [[nodiscard]] inline const ModelInformation& get_model_info() const {return model->get_model_information(); }

    [[nodiscard]] inline bool _do_applicability_filtering() const {return doApplicabilityFiltering; }

    void parse(const Jani2InterfaceParser& parser);
    void compute_labels_to_outputs();
    void compute_extended_inputs();

    /* Input: */

    inline std::size_t get_num_input_features() const { 
        return layerSizes[0]; \
    }

    [[nodiscard]] inline const InputFeatureToJANI* get_input_feature(InputIndex_type index) const {
        PLAJA_ASSERT(index < inputFeatureMapping.size())
        return inputFeatureMapping[index].get();
    }

    [[nodiscard]] inline VariableIndex_type get_input_feature_index(InputIndex_type index) const { return get_input_feature(index)->stateIndex; }

    [[nodiscard]] inline const LValueExpression* get_input_feature_expression(InputIndex_type index) const { return get_input_feature(index)->get_expression(); }

    /* Output: */

    [[nodiscard]] inline std::size_t get_num_output_features() const { return outputsToLabels.size(); };

    [[nodiscard]] inline ActionLabel_type get_output_label(OutputIndex_type index) const {
        PLAJA_ASSERT(index < outputsToLabels.size())
        return outputsToLabels[index];
    }

    [[nodiscard]] inline bool is_learned(ActionLabel_type action_label) const { return labelsToOutputs.count(action_label); } // silent-labeled operators should not be learned

    [[nodiscard]] inline OutputIndex_type get_output_index(ActionLabel_type action_label) const {
        PLAJA_ASSERT(is_learned(action_label))
        return labelsToOutputs.at(action_label);
    }

/**********************************************************************************************************************/

class InputFeatureIterator {
        friend Jani2Interface;
    private:
        const Jani2Interface* interface;
        InputIndex_type currentIndex;
        std::size_t numInputs;

        explicit InputFeatureIterator(const Jani2Interface& j_interface, InputIndex_type start = 0, bool extended = false):
            interface(&j_interface)
            , currentIndex(start)
            , numInputs(extended ? interface->inputFeatureMapping.size() : interface->layerSizes[0]) {}

    public:
        ~InputFeatureIterator() = default;
        DELETE_CONSTRUCTOR(InputFeatureIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] inline bool end() const { return currentIndex >= numInputs; }

        [[nodiscard]] inline InputIndex_type _input_index() const { return currentIndex; }

        [[nodiscard]] inline const InputFeatureToJANI* get_input_feature() const { return interface->get_input_feature(currentIndex); }

        [[nodiscard]] inline VariableIndex_type get_input_feature_index() const { return interface->get_input_feature_index(currentIndex); }

        [[nodiscard]] inline const LValueExpression* get_input_feature_expression() const { return interface->get_input_feature_expression(currentIndex); }
    };

    [[nodiscard]] inline InputFeatureIterator init_input_feature_iterator(bool extended = false) const { return InputFeatureIterator(*this, 0, extended); }

    [[nodiscard]] inline InputFeatureIterator init_input_feature_iterator_extended() const { return InputFeatureIterator(*this, get_num_input_features(), true); }

    class OutputFeatureIterator {
        friend Jani2Interface;
    private:
        const Jani2Interface* interface;
        OutputIndex_type currentIndex;

        explicit OutputFeatureIterator(const Jani2Interface& j_interface):
            interface(&j_interface)
            , currentIndex(0) {}

    public:
        ~OutputFeatureIterator() = default;
        DELETE_CONSTRUCTOR(OutputFeatureIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] inline bool end() const { return currentIndex >= interface->get_num_output_features(); }

        [[nodiscard]] inline OutputIndex_type _output_index() const { return currentIndex; }

        [[nodiscard]] inline ActionLabel_type get_output_label() const { return interface->get_output_label(currentIndex); }

        [[nodiscard]] inline bool is_learned() const { return interface->is_learned(get_output_label()); }
    };

    [[nodiscard]] inline OutputFeatureIterator init_output_feature_iterator() const { return OutputFeatureIterator(*this); }

};


#endif //PLAJA_JANI_2_INTERFACE_H
