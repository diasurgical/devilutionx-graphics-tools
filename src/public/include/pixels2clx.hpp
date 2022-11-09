#ifndef DVL_GFX_PIXELS2CLX_H_
#define DVL_GFX_PIXELS2CLX_H_

#include <cstdint>
#include <optional>
#include <vector>

namespace dvl_gfx {

/**
 * @brief Converts an 8-bit color-indexed pixel buffer to a CLX sprite (list).
 *
 * The frames in the pixel buffer must be stacked vertically.
 * All frames must be the same size.
 *
 * @param pixels The pixel buffer.
 * @param pitch Pixel buffer pitch, i.e. the width of each line including padding.
 * @param width Frame (sprite) width.
 * @param frameHeight Frame (sprite) height.
 * @param numFrames The number of frames in the pixel buffer.
 * @param transparentColor Palette index of the transparent color.
 * @param clxData Output CLX buffer.
 */
void Pixels2Clx(
    const uint8_t *pixels,
    unsigned pitch, unsigned width, unsigned frameHeight, unsigned numFrames,
    std::optional<uint8_t> transparentColor, std::vector<uint8_t> &clxData);

} // namespace dvl_gfx

#endif // DVL_GFX_PIXELS2CLX_H_
