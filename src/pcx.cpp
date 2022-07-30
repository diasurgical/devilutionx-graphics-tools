#include "pcx.hpp"

#include <cerrno>
#include <cstring>

#include "endian.hpp"

namespace devilution {

std::optional<IoError> LoadPcxMeta(std::ifstream &file, int &width, int &height, uint8_t &bpp)
{
	PCXHeader pcxhdr;
	file.read(reinterpret_cast<char *>(&pcxhdr), PcxHeaderSize);
	if (file.fail()) {
		return IoError { std::string("Failed to read PCX header: ")
			                 .append(std::strerror(errno)) };
	}
	width = SwapLE(pcxhdr.xmax) - SwapLE(pcxhdr.xmin) + 1;
	height = SwapLE(pcxhdr.ymax) - SwapLE(pcxhdr.ymin) + 1;
	bpp = pcxhdr.bitsPerPixel;
	return std::nullopt;
}

} // namespace devilution
