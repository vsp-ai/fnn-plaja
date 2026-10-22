//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FACTORY_CONFIG_CONST_H
#define PLAJA_FACTORY_CONFIG_CONST_H

#include "factory_config.h"

namespace PLAJA_GLOBAL {

//

#ifdef USE_Z3
    constexpr bool useZ3 = true;
#else
    constexpr bool useZ3 = false;
#endif


}

#endif //PLAJA_FACTORY_CONFIG_CONST_H
