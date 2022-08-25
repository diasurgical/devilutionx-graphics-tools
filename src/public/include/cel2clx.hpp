#ifndef DVL_GFX_CEL2CLX_H_
#define DVL_GFX_CEL2CLX_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

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
