//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "enum_option_values.h"
#include "../utils/string_set.h"
#include "../assertions.h"

namespace PLAJA_ENUM_STR {

    const std::string none("none"); // NOLINT(*-err58-cpp)

#ifdef USE_Z3
    const std::string z3("z3"); // NOLINT(*-err58-cpp)
    const std::string naive("naive"); // NOLINT(*-err58-cpp)
    const std::string inc("inc"); // NOLINT(*-err58-cpp)
    const std::string bs("bs"); // NOLINT(*-err58-cpp)
    const std::string td("td"); // NOLINT(*-err58-cpp)
    const std::string sample("sample"); // NOLINT(*-err58-cpp)
#endif

#ifdef USE_MARABOU
    const std::string marabou("marabou"); // NOLINT(*-err58-cpp)
    const std::string relaxed("relaxed"); // NOLINT(*-err58-cpp)
    const std::string bb("bb"); // NOLINT(*-err58-cpp)
    const std::string enumerate("enumerate"); // NOLINT(*-err58-cpp)
#endif

#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
    const std::string global("global"); // NOLINT(*-err58-cpp)
    const std::string perSrc("per-src"); // NOLINT(*-err58-cpp)
    const std::string perLabel("per-label"); // NOLINT(*-err58-cpp)
    const std::string perOp("per-op"); // NOLINT(*-err58-cpp)
#endif

#endif

}

namespace PLAJA_OPTION {

    std::unique_ptr<EnumOptionValue> str_to_enum_option_value(const std::string& str) {
        static const PLAJA::StringMap<EnumOptionValue> str_to_config {
            { &PLAJA_ENUM_STR::none, EnumOptionValue::None },
#ifdef USE_Z3
            { &PLAJA_ENUM_STR::z3, EnumOptionValue::Z3 },
            { &PLAJA_ENUM_STR::naive, EnumOptionValue::Naive },
            { &PLAJA_ENUM_STR::inc, EnumOptionValue::Inc },
            { &PLAJA_ENUM_STR::bs, EnumOptionValue::Bs },
            { &PLAJA_ENUM_STR::td, EnumOptionValue::Td },
            { &PLAJA_ENUM_STR::sample, EnumOptionValue::Sample },
#endif
#ifdef USE_MARABOU
            { &PLAJA_ENUM_STR::marabou, EnumOptionValue::Marabou },
            { &PLAJA_ENUM_STR::relaxed, EnumOptionValue::Relaxed },
            { &PLAJA_ENUM_STR::bb, EnumOptionValue::BranchAndBound },
            { &PLAJA_ENUM_STR::relaxed, EnumOptionValue::Enumerate },
#endif
#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
            { &PLAJA_ENUM_STR::global, EnumOptionValue::PaReachabilityGlobal },
            { &PLAJA_ENUM_STR::perSrc, EnumOptionValue::PaOnlyReachabilityPerSrc },
            { &PLAJA_ENUM_STR::perLabel, EnumOptionValue::PaOnlyReachabilityPerLabel },
            { &PLAJA_ENUM_STR::perOp, EnumOptionValue::PaOnlyReachabilityPerOp },
#endif

#endif
        };

        const auto it = str_to_config.find(&str);
        return it == str_to_config.cend() ? nullptr : std::make_unique<EnumOptionValue>(it->second);
    }

    const std::string& enum_option_value_to_str(EnumOptionValue value) {
        switch (value) {
            case EnumOptionValue::None: { return PLAJA_ENUM_STR::none; }
#ifdef USE_Z3
            case EnumOptionValue::Z3: { return PLAJA_ENUM_STR::z3; }
            case EnumOptionValue::Naive: { return PLAJA_ENUM_STR::naive; }
            case EnumOptionValue::Inc: { return PLAJA_ENUM_STR::inc; }
            case EnumOptionValue::Bs: { return PLAJA_ENUM_STR::bs; }
            case EnumOptionValue::Td: { return PLAJA_ENUM_STR::td; }
            case EnumOptionValue::Sample: { return PLAJA_ENUM_STR::sample; }
#endif
#ifdef USE_MARABOU
            case EnumOptionValue::Marabou: { return PLAJA_ENUM_STR::marabou; }
            case EnumOptionValue::Relaxed: { return PLAJA_ENUM_STR::relaxed; }
            case EnumOptionValue::BranchAndBound: { return PLAJA_ENUM_STR::bb; }
            case EnumOptionValue::Enumerate: { return PLAJA_ENUM_STR::enumerate; }
#endif
#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
            case EnumOptionValue::PaReachabilityGlobal: { return PLAJA_ENUM_STR::global; }
            case EnumOptionValue::PaOnlyReachabilityPerSrc: { return PLAJA_ENUM_STR::perSrc; }
            case EnumOptionValue::PaOnlyReachabilityPerLabel: { return PLAJA_ENUM_STR::perLabel; }
            case EnumOptionValue::PaOnlyReachabilityPerOp: { return PLAJA_ENUM_STR::perOp; }
#endif

#endif
            default: PLAJA_ABORT
        }
    }

}