#ifndef DVL_GFX_CEL2CLX_H_
#define DVL_GFX_CEL2CLX_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

/**
 * @brief Converts a CEL image to CLX.
 *
 * @param data The CEL buffer.
 * @param size CEL buffer size.
 * @param widths Widths of each frame. If all the frame are the same width, this can be a single number.
 * @param numWidths The number of widths.
 * @param clxData Output CLX buffer.
 * @return std::optional<IoError>
 */
std::optional<IoError> CelToClx(const uint8_t *data, size_t size,
    const uint16_t *widths, size_t numWidths, std::vector<uint8_t> &clxData);

std::optional<IoError> CelToClx(const char *inputPath, const char *outputPath,
    const uint16_t *widths, size_t numWidths,
    uintmax_t *inputFileSize = nullptr,
    uintmax_t *outputFileSize = nullptr);

inline std::optional<IoError> CelToClx(const char *inputPath, const char *outputPath,
    const std::vector<uint16_t> &widths,
    uintmax_t *inputFileSize = nullptr,
    uintmax_t *outputFileSize = nullptr)
{
	return CelToClx(inputPath, outputPath, widths.data(), widths.size(), inputFileSize, outputFileSize);
}

} // namespace dvl_gfx
#endif // DVL_GFX_CEL2CLX_H_
