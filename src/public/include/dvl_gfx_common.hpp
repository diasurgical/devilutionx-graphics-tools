#ifndef DVL_GFX_COMMON_H_
#define DVL_GFX_COMMON_H_

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

} // namespace dvl_gfx
#endif // DVL_GFX_COMMON_H_
