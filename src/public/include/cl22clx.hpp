#ifndef DVL_GFX_CL22CLX_H_
#define DVL_GFX_CL22CLX_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

std::optional<IoError> Cl2ToClx(const char *inputPath, const char *outputPath,
    const uint16_t *widths, size_t numWidths);

inline std::optional<IoError> Cl2ToClx(const char *inputPath, const char *outputPath,
    const std::vector<uint16_t> &widths)
{
	return Cl2ToClx(inputPath, outputPath, widths.data(), widths.size());
}

} // namespace dvl_gfx
#endif // DVL_GFX_CL22CLX_H_
