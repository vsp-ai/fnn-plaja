#ifndef DOMAIN_SPECIFIC_H
#define DOMAIN_SPECIFIC_H

#include <string>
#include <memory>
#include <stdexcept>
#include "../search/states/state_values.h"
#include "../search/using_search.h"
#include "../search/information/jani_2_interface.h"
#include "../parser/ast/model.h"
#include <iostream>
#include <vector>
#include <unordered_map>

/** Hand-written expert policies (POLICY_EVALUATION --expert-policy). */
class DomainSpecific {
public:
    virtual ~DomainSpecific() = default;

    DomainSpecific(const Model* model, const Jani2Interface* jani)
        : model_(model), jani_(jani) {}

    virtual ActionLabel_type get_action(const StateValues& state_values) = 0;
    virtual std::vector<double> action_values(const StateValues& state_values);

    // Throws std::invalid_argument if the name is unknown.
    static std::unique_ptr<DomainSpecific> create(const std::string& name, const Model* model, const Jani2Interface* jani);

protected:
    const Model* model_;
    const Jani2Interface* jani_;
};

class Transport : public DomainSpecific {
public:
    using DomainSpecific::DomainSpecific;

    ActionLabel_type get_action(const StateValues& state_values) override;
};

class OneWayLinePark : public DomainSpecific {
public:
    using DomainSpecific::DomainSpecific;
    ActionLabel_type get_action(const StateValues& state_values) override;
    std::vector<double> action_values(const StateValues& state_values) override;
};

class TwoWayLinePark : public DomainSpecific {
public:
    using DomainSpecific::DomainSpecific;
    ActionLabel_type get_action(const StateValues& state_values) override;
    std::vector<double> action_values(const StateValues& state_values) override;
};

class Transport_Feat : public DomainSpecific {
public:
    using DomainSpecific::DomainSpecific;
    ActionLabel_type get_action(const StateValues& state_values) override;
};

#endif // DOMAIN_SPECIFIC_H