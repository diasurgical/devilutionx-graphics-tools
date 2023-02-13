#ifndef DVL_GFX_CLX_ENCODE_H_
#define DVL_GFX_CLX_ENCODE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include <dvl_gfx_endian.hpp>

namespace dvl_gfx {

/**
 * @return Size of the CLX sheet header for a CLX sheet with the given number of lists.
 */
inline uint32_t ClxSheetHeaderSize(uint32_t numLists)
{
	return numLists * 4;
}

/**
 * @brief Sets the byte offset for the given list index in the CLX sheet header.
 */
inline void ClxSheetHeaderSetListOffset(size_t listIndex, uint32_t offset, uint8_t *clxSheetHeader)
{
	WriteLE32(&clxSheetHeader[listIndex * 4], offset);
}

void AppendClxTransparentRun(unsigned width, std::vector<uint8_t> &out);
void AppendClxPixelsOrFillRun(const uint8_t *src, unsigned length, std::vector<uint8_t> &out);

} // namespace dvl_gfx
#endif // DVL_GFX_CLX_ENCODE_H_
