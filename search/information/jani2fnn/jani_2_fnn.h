//
// This file is part of the PlaJA code base.
// Copyright ...
//

#ifndef PLAJA_JANI_2_FNN_H
#define PLAJA_JANI_2_FNN_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>

#include "../../../parser/using_parser.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../non_prob_search/policy/forward_policy.h"
#include "../../states/forward_states.h" //StaeBase
#include "../forward_information.h"
#include "../jani_2_interface.h"
class FNNPolicy;

namespace PLAJA { class FormularizedNetwork; }

class Jani2FNN : public Jani2Interface {
    friend class Jani2FNNParser;
    using OutputIndex_type = unsigned int;
    using InputIndex_type  = unsigned int;

protected:
    std::string fnnFile;

public:
    virtual ~Jani2FNN();

    Jani2FNN(const Jani2FNN&) = delete;
    Jani2FNN(Jani2FNN&&) = delete;
    DELETE_ASSIGNMENT(Jani2FNN)

    static std::unique_ptr<Jani2FNN> load_interface(const Model& model);
    static std::unique_ptr<Jani2FNN> load_interface(const Model& model,
                                                    const std::string& jani_2_fnn_file);

    const Policy& load_policy(const PLAJA::Configuration& config) const override;
    void compute_layer_sizes();
    void compute_extended_inputs();
    void compute_labels_to_outputs();
    [[nodiscard]] const PLAJA::FormularizedNetwork& load_fnn() const;
    [[nodiscard]] inline const std::string& get_policy_file() const override { return fnnFile; }
    
    FCT_IF_DEBUG([[nodiscard]] ActionLabel_type eval_policy(const StateBase& state) const override;)
    FCT_IF_DEBUG([[nodiscard]] bool is_chosen(const StateBase& state,
                                              ActionLabel_type action_label) const override;)
    
protected:
    explicit Jani2FNN(const Model& model);

private:
    // loaded structures
    mutable std::unique_ptr<PLAJA::FormularizedNetwork> fnn;
    mutable std::unique_ptr<FNNPolicy> fnnPolicy;
    
    bool initialized=false;
    
    

};

#endif //PLAJA_JANI_2_FNN_H
