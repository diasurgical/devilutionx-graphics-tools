#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "io_error.hpp"

namespace devilution {

std::optional<IoError> Cl2ToClx(const char *inputPath, const char *outputPath,
    const std::vector<uint16_t> &widths);

} // namespace devilution
