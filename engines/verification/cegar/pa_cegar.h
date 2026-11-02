//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_CEGAR_H
#define PLAJA_PA_CEGAR_H

#include "include/ct_config_const.h"
#include "model/ast/expression/forward_expression.h"
#include "model/ast/forward_ast.h"
#include "core/utils/default_constructors.h"
#include "engines/verification/forward_factories_pa.h"
#include "core/search_engine.h"
#include "core/using_core.h"
#include "engines/verification/cegar/forward_cegar.h"
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>
#include "core/state_values.h"

class StateBase;
class SearchPAPath;

class PACegar final: public SearchEngine {

private:
    // cached:
    std::unique_ptr<Properties> cachedProperties;

    // abstract search
    std::unique_ptr<Property> paProperty;
    std::unique_ptr<PredicatesStructure> predicates;
    std::unique_ptr<PropertyInformation> propInfoSub;
    unsigned int maxNumPreds;
    // sub-engine (have to cache to prohibit dangling pointer in case of signal handling)
    std::unique_ptr<SearchPAPath> cegarSearch;

    // concrete path certification:
    std::unique_ptr<SpuriousnessChecker> spuriousnessChecker;
    std::unique_ptr<SpuriousnessResult> current_spuriousness_result;

    PAExpression* get_pa_expression();
    void cache_pa_property();
    void predicates_to_file() const;

    // override:
    SearchStatus initialize() override;
    SearchStatus step() override;


public:
    explicit PACegar(const SearchEngineConfigPA& config);
    ~PACegar() override;
    DELETE_CONSTRUCTOR(PACegar)

    // stats
    void update_statistics() const override;
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;
    void short_stats_to_csv(std::ofstream& file) const override;
    void short_stat_names_to_csv(std::ofstream& file) const override;
};

#endif //PLAJA_PA_CEGAR_H
