#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "io_error.hpp"

namespace devilution {

std::optional<IoError> PcxToClx(const char *inputPath, const char *outputPath,
    int numFramesOrFrameHeight,
    std::optional<uint8_t> transparentColor,
    bool exportPalette,
    uintmax_t &inputFileSize,
    uintmax_t &outputFileSize);

} // namespace devilution
