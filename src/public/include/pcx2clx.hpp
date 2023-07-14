#ifndef DVL_GFX_PCX2CLX_H_
#define DVL_GFX_PCX2CLX_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

/**
 * @brief Converts a PCX image to CLX.
 *
 * @param data The PCX buffer.
 * @param size PCX buffer size.
 * @param numFramesOrFrameHeight Number of vertically-stacked frames if positive, frame height if negative.
 * @param transparentColor Palette index of the transparent color.
 * @param cropWidths If non-empty, the sprites are cropped to the given width(s) by removing the right side of the sprite.
 * @param paletteData If non-null, PCX palette data (256 * 3 bytes).
 * @return std::optional<IoError>
 */
std::optional<IoError> PcxToClx(const uint8_t *data, size_t size,
    int numFramesOrFrameHeight,
    std::optional<uint8_t> transparentColor,
    const std::vector<uint16_t> &cropWidths,
    std::vector<uint8_t> &clxData,
    uint8_t *paletteData = nullptr);

std::optional<IoError> PcxToClx(const char *inputPath, const char *outputPath,
    int numFramesOrFrameHeight = 1,
    std::optional<uint8_t> transparentColor = std::nullopt,
    const std::vector<uint16_t> &cropWidths = {},
    bool exportPalette = false,
    uintmax_t *inputFileSize = nullptr,
    uintmax_t *outputFileSize = nullptr);

} // namespace dvl_gfx
#endif // DVL_GFX_PCX2CLX_H_
