#ifndef DVL_GFX_CEL2CLX_H_
#define DVL_GFX_CEL2CLX_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <dvl_gfx_common.hpp> // IWYU pragma: export

namespace dvl_gfx {

std::optional<IoError> CelToClx(const char *inputPath, const char *outputPath,
    const std::vector<uint16_t> &widths,
    uintmax_t *inputFileSize = nullptr,
    uintmax_t *outputFileSize = nullptr);

} // namespace dvl_gfx
#endif // DVL_GFX_CEL2CLX_H_
