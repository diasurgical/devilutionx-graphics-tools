#ifndef DVL_GFX_COMMON_H_
#define DVL_GFX_COMMON_H_

#include <cstddef>
#include <cstdint>

#include <string>

namespace dvl_gfx {

struct IoError {
	std::string message;
};

struct Size {
	uint32_t width;
	uint32_t height;
};

/**
 * CLX frame header is 6 bytes:
 *
 *  Bytes |   Type   | Value
 * :-----:|:--------:|-------------
 *  0..2  | uint16_t | header size
 *  2..4  | uint16_t | width
 *  4..6  | uint16_t | height
 */
constexpr size_t ClxFrameHeaderSize = 6;

} // namespace dvl_gfx
#endif // DVL_GFX_COMMON_H_
