//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "core/exception/plaja_exception.h"

// strings
namespace PLAJA_EXCEPTION {
    const std::string fileNotFoundString("File not found: "); // NOLINT(cert-err58-cpp)
}

PlajaException::PlajaException(std::string information): information(std::move(information)) {}

PlajaException::~PlajaException() noexcept = default;

const char* PlajaException::what() const noexcept { return information.c_str(); }
