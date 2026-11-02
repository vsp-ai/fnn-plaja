//
// This file is part of the PlaJA code base.
//

#ifndef PLAJA_FNN_READER_H
#define PLAJA_FNN_READER_H

#include <memory>
#include <string>

#include "policies/fnn/formularized_network.h"

class Jani2Interface;

namespace PLAJA {
    namespace FNN_PARSER {

        std::unique_ptr<FormularizedNetwork> read_fnn(const std::string& path, const Jani2Interface& interface);

    } // namespace FNN_PARSER
} // namespace PLAJA

#endif // PLAJA_FNN_READER_H
