//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "search_engine_factory.h"
#include "../../exception/option_parser_exception.h"
#include "../../exception/property_analysis_exception.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../fd_adaptions/search_engine.h"
#include "../information/property_information.h"
#include "configuration.h"

#include <iostream>
#include <string>
#include <unordered_map>

#include "predicate_abstraction/pa_factory.h"

namespace SEARCH_ENGINE_FACTORY {

    const std::string searchEngineTypeToString[] {
        "PA_CEGAR",
        "POLICY_EVALUATION"}; // NOLINT(cert-err58-cpp)

    const std::unordered_map<std::string, SearchEngineFactory::SearchEngineType> stringToSearchEngineType { // NOLINT(cert-err58-cpp)
        {"PA_CEGAR", SearchEngineFactory::PaCegarType},
        {"POLICY_EVALUATION", SearchEngineFactory::PolicyEvaluationType}
    };

    void print_supported_engines() {
        std::cout << "\tSupported engines:";
        for (const auto& engine_type_str: searchEngineTypeToString) {
            if (stringToSearchEngineType.count(engine_type_str)) {
                std::cout << PLAJA_UTILS::spaceString << engine_type_str;
            }
        }
        std::cout << PLAJA_UTILS::dotString << std::endl;
    }

} // namespace SEARCH_ENGINE_FACTORY

SearchEngineFactory::SearchEngineFactory(SearchEngineType engine_type):
    engineType(engine_type) {}

SearchEngineFactory::~SearchEngineFactory() = default;

/* */

SearchEngineFactory::SearchEngineType SearchEngineFactory::determine_search_engine_type(
    const std::string& search_engine_type) {
    if (SEARCH_ENGINE_FACTORY::stringToSearchEngineType.count(search_engine_type) > 0) {
        return SEARCH_ENGINE_FACTORY::stringToSearchEngineType.at(search_engine_type);
    } else {
        throw OptionParserException("unknown search engine: " + search_engine_type);
    }
}

SearchEngineFactory::SearchEngineType SearchEngineFactory::determine_search_engine_type(
    const PLAJA::OptionParser& option_parser) {
    if (option_parser.has_value_option(PLAJA_OPTION::engine)) {
        return determine_search_engine_type(option_parser.get_value_option_string(PLAJA_OPTION::engine));
    } else {
        return SearchEngineType::PaCegarType;
    }
}

void SearchEngineFactory::print_construction_info() const {
    std::cout << "Constructing " << this->to_string() << " ..." << std::endl;
}

void SearchEngineFactory::add_options(PLAJA::OptionParser& /*option_parser*/) const {}

void SearchEngineFactory::print_options() const {}

const std::string& SearchEngineFactory::to_string() const {
    return SEARCH_ENGINE_FACTORY::searchEngineTypeToString[engineType];
}

//

std::unique_ptr<SearchEngineFactory> SearchEngineFactory::construct_factory(SearchEngineType engine_type) {

    switch (engine_type) {
        case SearchEngineType::PaCegarType: return std::make_unique<PACegarFactory>();
        case SearchEngineType::PolicyEvaluationType: return SEARCH_ENGINE_FACTORY::construct_policy_evaluation_factory();
        default: PLAJA_ABORT
    }
}

std::unique_ptr<SearchEngineFactory> SearchEngineFactory::construct_factory(const PLAJA::OptionParser& option_parser) {
    return construct_factory(determine_search_engine_type(option_parser));
}

void SearchEngineFactory::adapt_configuration(PLAJA::Configuration& config, const PropertyInformation* property_info)
    const {
    if (property_info) {
        config.set_sharable_const(PLAJA::SharableKey::PROP_INFO, property_info);
        config.set_sharable_const(PLAJA::SharableKey::MODEL, property_info->get_model());
    }
}

std::unique_ptr<PLAJA::Configuration> SearchEngineFactory::init_configuration(
    const PLAJA::OptionParser& option_parser,
    const PropertyInformation* property_info) const {
    auto config = std::make_unique<PLAJA::Configuration>(option_parser);
    adapt_configuration(*config, property_info);
    return config;
}

std::unique_ptr<SearchEngine> SearchEngineFactory::construct_engine(const PLAJA::Configuration& configuration) const {
    print_construction_info();
    return construct(configuration);
}

void SearchEngineFactory::supports_model(const Model& /*model*/) const { /* NOTHING */ }

void SearchEngineFactory::supports_property(const PropertyInformation& property_info) const {
    if (property_info.is_non_property()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (property_info.is_reach_property() and not supports_reachability_property()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (property_info.is_prob_property() and not supports_probabilistic_property()) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
    if (requires_policy() and not(property_info.get_interface())) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), "Policy required.");
    }
    if (not supports_property_sub(property_info)) {
        throw PropertyAnalysisException(property_info.get_property_name(), to_string(), PLAJA_UTILS::emptyString);
    }
}

// internal auxiliaries

bool SearchEngineFactory::supports_reachability_property() const {
    switch (engineType) {
        case SearchEngineType::PaCegarType: //  cegar will use the goal for predicate generation
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::supports_probabilistic_property() const {
    switch (engineType) {
        case SearchEngineType::PaCegarType: // cegar will use the goal of FRET/LRTDP-properties we consider for predicate generation
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::requires_policy() const {
    switch (engineType) {
        case SearchEngineType::PolicyEvaluationType:
            return true;
        default: {
            return false;
        }
    }
}

bool SearchEngineFactory::supports_property_sub(const PropertyInformation& /*prop_info*/) const { return false; }