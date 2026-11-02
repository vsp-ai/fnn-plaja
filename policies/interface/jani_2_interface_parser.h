//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_JANI_2_INTERFACE_PARSER_H
#define PLAJA_JANI_2_INTERFACE_PARSER_H

#include <memory>
#include <string>
#include <vector>
#include <json_fwd.hpp>
#include "policies/using_nn.h"
#include "core/using_core.h"

class Model;

struct InputFeatureToJANI;

class Jani2InterfaceParser {
private:
    const Model& model;
    std::unique_ptr<nlohmann::json> loadedJSON;

    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToName(const nlohmann::json& input) const;

public:
    explicit Jani2InterfaceParser(const Model& model_);
    ~Jani2InterfaceParser();

    [[nodiscard]] bool parse_applicability_filtering() const;
    [[nodiscard]] std::vector<std::unique_ptr<InputFeatureToJANI>> parse_input_interface() const;
    [[nodiscard]] std::vector<ActionLabel_type> parse_output_interface() const;

};

#endif //PLAJA_JANI_2_INTERFACE_PARSER_H
