#ifndef DVL_GFX_CL22CLX_H_
#define DVL_GFX_CL22CLX_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

/**
 * @brief Converts a CL2 image to CLX in-place.
 *
 * @param data The CL2 buffer.
 * @param size CL2 buffer size.
 * @param widths Widths of each frame. If all the frame are the same width, this can be a single number.
 * @param numWidths The number of widths.
 * @return std::optional<IoError>
 */
std::optional<IoError> Cl2ToClx(uint8_t *data, size_t size,
    const uint16_t *widths, size_t numWidths);

std::optional<IoError> Cl2ToClx(const char *inputPath, const char *outputPath,
    const uint16_t *widths, size_t numWidths);

inline std::optional<IoError> Cl2ToClx(const char *inputPath, const char *outputPath,
    const std::vector<uint16_t> &widths)
{
	return Cl2ToClx(inputPath, outputPath, widths.data(), widths.size());
}

/**
 * @brief Converts multiple CL2 images to a CLX sheet.
 *
 * @param inputPaths Paths to the input files.
 * @param numFiles The number of `inputPaths`.
 * @param widths Widths of each frame. If all the frame are the same width, this can be a single number.
 * @return std::optional<IoError>
 */
std::optional<IoError> CombineCl2AsClxSheet(
    const char *const *inputPaths, size_t numFiles, const char *outputPath,
    const std::vector<uint16_t> &widths);

} // namespace dvl_gfx
#endif // DVL_GFX_CL22CLX_H_
