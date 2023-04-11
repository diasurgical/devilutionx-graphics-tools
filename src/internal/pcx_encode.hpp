#pragma once

#include <cstdint>

#include <optional>
#include <ostream>
#include <span>

#include <dvl_gfx_common.hpp>

namespace dvl_gfx {

std::optional<IoError> PcxEncode(
    std::span<const uint8_t> pixels, Size size, uint32_t pitch,
    std::span<const uint8_t> palette, std::ostream *out);

} // namespace dvl_gfx
