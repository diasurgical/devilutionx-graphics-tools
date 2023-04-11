#ifndef DVL_GFX_CLX2PIXELS_H_
#define DVL_GFX_CLX2PIXELS_H_

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

/**
 * @brief Measures the total dimensions of a CLX list if its sprites were
 * to be stacked vertically.
 */
Size MeasureVerticallyStackedClxListSize(std::span<const uint8_t> clxList);

/**
 * @brief Measures the total dimensions of a CLX sheet if its lists were
 * to be stacked horizontally with sprites within each list stacked vertically.
 */
Size MeasureHorizontallyStackedClxListOrSheetSize(std::span<const uint8_t> clxData);

/**
 * @brief Converts a CLX to an 8-bit color-indexed pixel buffer.
 *
 * Does not clear the buffer before drawing.
 *
 * @param clxData The CLX buffer.
 * @param transparentColor Palette index of the transparent color.
 * @param pixels Output pixel buffer.
 *     Individual frames are stacked vertically.
 * @param pitch The width of the line in the pixel buffer including padding.
 *     If `std::nullopt`, assumes no padding.
 * @param outDimensions If non-null, set to the dimensions of the resulting image.
 */
std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    std::vector<uint8_t> &pixels,
    std::optional<unsigned> pitch = std::nullopt,
    Size *outDimensions = nullptr);

/**
 * @brief Converts a CLX to an 8-bit color-indexed pixel buffer.
 *
 * @param clxData The CLX buffer.
 * @param transparentColor Palette index of the transparent color.
 * @param pixels Output pixel buffer.
 *     Must be large enough to fit all the frames of the CLX.
 *     The frames are stacked vertically.
 * @param pitch The width of the line in the pixel buffer including padding.
 * @param outDimensions If non-null, set to the dimensions of the resulting image.
 */
std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch,
    Size *outDimensions = nullptr);

} // namespace dvl_gfx

#endif // DVL_GFX_CLX2PIXELS_H_
