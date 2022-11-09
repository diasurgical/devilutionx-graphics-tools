#ifndef DVL_GFX_CLX2PIXELS_H_
#define DVL_GFX_CLX2PIXELS_H_

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

/**
 * @brief Converts a CLX to an 8-bit color-indexed pixel buffer.
 *
 * Does not support CLX sprite sheets.
 *
 * @param clxData The CLX buffer.
 * @param transparentColor Palette index of the transparent color.
 * @param pixels Output pixel buffer.
 *     Individual frames are stacked vertically.
 * @param pitch The width of the line in the pixel buffer including padding.
 *     If `std::nullopt`, assumes no padding.
 */
std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    std::vector<uint8_t> &pixels,
    std::optional<unsigned> pitch = std::nullopt);

/**
 * @brief Converts a CLX to an 8-bit color-indexed pixel buffer.
 *
 * Does not support CLX sprite sheets.
 *
 * @param clxData The CLX buffer.
 * @param transparentColor Palette index of the transparent color.
 * @param pixels Output pixel buffer.
 *     Must be large enough to fit all the frames of the CLX.
 *     The frames are stacked vertically.
 * @param pitch The width of the line in the pixel buffer including padding.
 */
std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch);

} // namespace dvl_gfx

#endif // DVL_GFX_CLX2PIXELS_H_
