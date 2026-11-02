//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OUTPUT_CONSTRAINTS_IN_MARABOU_H
#define PLAJA_OUTPUT_CONSTRAINTS_IN_MARABOU_H

#include <list>
#include <memory>
#include <vector>
#include "include/marabou_include/forward_marabou.h"
#include "include/ct_config_const.h"
#include "core/utils/default_constructors.h"
#include "policies/nn/forward_jani_2_nnet.h"
#include "policies/using_nn.h"
#include "core/using_core.h"
#include "marabou/forward_marabou_in_plaja.h"
#include "marabou/using_marabou.h"

namespace MARABOU_IN_PLAJA {

    class OutputConstraints {

    protected:
        const ModelMarabou* modelMarabou;

        /* Auxiliary to compute output constraint equation. */
        [[nodiscard]] static Equation compute_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var);
        [[nodiscard]] static Equation compute_negation_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var);

        [[nodiscard]] static Equation compute_aux_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var, MarabouVarIndex_type aux_var);
        [[nodiscard]] static Tightening compute_aux_var_tightening(MarabouVarIndex_type aux_var);
        [[nodiscard]] static Tightening compute_aux_var_tightening_negation(MarabouVarIndex_type aux_var);

    public:
        explicit OutputConstraints(const ModelMarabou& model_marabou);
        virtual ~OutputConstraints() = 0;
        DELETE_CONSTRUCTOR(OutputConstraints)

        [[nodiscard]] virtual std::unique_ptr<ConjunctionInMarabou> compute(OutputIndex_type output_index, StepIndex_type step) const = 0;
        [[nodiscard]] virtual std::unique_ptr<MarabouConstraint> compute_negation(OutputIndex_type output_index, StepIndex_type step) const = 0;

        virtual void add(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const = 0;
        virtual void add_negation(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const = 0;

#ifdef ENABLE_APPLICABILITY_FILTERING
        [[nodiscard]] static std::unique_ptr<OutputConstraints> construct(const ModelMarabou& model_marabou);
#else
        [[nodiscard]] static std::unique_ptr<class OutputConstraintsNoFilter> construct(const ModelMarabou& model_marabou);
#endif

        inline void generate_steps() {}

    };

    class OutputConstraintsNoFilter final: public OutputConstraints {

    private:
        /* Cache for step=0. */
        std::vector<std::unique_ptr<ConjunctionInMarabou>> constraintsPerLabel;
        std::vector<std::unique_ptr<MarabouConstraint>> exclusionConstraintPerLabel; // Standard negation.

        /* Internal. */
        [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> compute_internal(OutputIndex_type output_index, StepIndex_type step) const;
        [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation_internal(OutputIndex_type output_index, StepIndex_type step) const;

    public:
        explicit OutputConstraintsNoFilter(const ModelMarabou& model);
        ~OutputConstraintsNoFilter() override;
        DELETE_CONSTRUCTOR(OutputConstraintsNoFilter)

        [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> compute(OutputIndex_type output_index, StepIndex_type step) const override;
        [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation(OutputIndex_type output_index, StepIndex_type step) const override;

        void add(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const override;
        void add_negation(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const override;

    };

#ifdef ENABLE_APPLICABILITY_FILTERING

    class OutputConstraintsAppFilter final: public OutputConstraints {

    private:
        /* Cache for step=0. */
        // std::vector<std::vector<std::unique_ptr<PiecewiseLinearCaseSplit>>> maxConstraintsPerLabelPair; // o >= o'
        // std::vector<std::vector<std::unique_ptr<Equation>>> maxConstraintsNegPerLabelPair; // not (o >= o')
        // std::vector<std::vector<std::unique_ptr<Equation>>> maxConstraintsOtherPerLabelPair; // o' >= o

        mutable QueryConstructable* queryConstruct; // Hack to enable optimizations.
        mutable const OpApplicability* opApp; // Quick fix to share operator applicability. See MARABOU_IN_PLAJA::QueryConstructable.

        void pre_optimization(MARABOU_IN_PLAJA::QueryConstructable* query) const;
        void post_optimization() const;

    public:
        explicit OutputConstraintsAppFilter(const ModelMarabou& model);
        ~OutputConstraintsAppFilter() override;
        DELETE_CONSTRUCTOR(OutputConstraintsAppFilter)

        [[nodiscard]] std::unique_ptr<ConjunctionInMarabou> compute(OutputIndex_type output_index, StepIndex_type step) const override;
        [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation(OutputIndex_type output_index, StepIndex_type step) const override;

        void add(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const override;
        void add_negation(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const override;

    };

#endif

}

#endif //PLAJA_OUTPUT_CONSTRAINTS_IN_MARABOU_H
