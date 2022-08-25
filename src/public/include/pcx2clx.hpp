#ifndef DVL_GFX_PCX2CLX_H_
#define DVL_GFX_PCX2CLX_H_

#include <cstdint>
#include <optional>
#include <string>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

std::optional<IoError> PcxToClx(const char *inputPath, const char *outputPath,
    int numFramesOrFrameHeight = 1,
    std::optional<uint8_t> transparentColor = std::nullopt,
    bool exportPalette = false,
    uintmax_t *inputFileSize = nullptr,
    uintmax_t *outputFileSize = nullptr);

} // namespace dvl_gfx
#endif // DVL_GFX_PCX2CLX_H_
