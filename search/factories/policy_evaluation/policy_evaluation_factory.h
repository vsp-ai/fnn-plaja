#ifndef PLAJA_POLICY_EVALUATION_FACTORY_H
#define PLAJA_POLICY_EVALUATION_FACTORY_H

#include "../search_engine_factory.h"
#include "../../../policy_evaluation/policy_evaluation_engine.h"

class PolicyEvaluationFactory : public SearchEngineFactory {
public:
    PolicyEvaluationFactory();
    ~PolicyEvaluationFactory() override;

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;
    
    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
  
    // For now, accept any property type.
    bool supports_property_sub(const PropertyInformation& /*prop_info*/) const override {
        return true;
    }
};


#endif //PLAJA_POLICY_EVALUATION_FACTORY_H