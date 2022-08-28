#include "pcx.hpp"

#include <cerrno>
#include <cstring>

#include "endian.hpp"

namespace dvl_gfx {

const uint8_t *LoadPcxMeta(const uint8_t *data, int &width, int &height, uint8_t &bpp)
{
	PCXHeader pcxhdr;
	std::memcpy(&pcxhdr, data, PcxHeaderSize);
	width = SwapLE(pcxhdr.xmax) - SwapLE(pcxhdr.xmin) + 1;
	height = SwapLE(pcxhdr.ymax) - SwapLE(pcxhdr.ymin) + 1;
	bpp = pcxhdr.bitsPerPixel;
	return data + PcxHeaderSize;
}

} // namespace dvl_gfx
