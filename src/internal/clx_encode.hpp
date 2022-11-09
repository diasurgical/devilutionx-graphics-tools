#ifndef DVL_GFX_CLX_ENCODE_H_
#define DVL_GFX_CLX_ENCODE_H_

#include <cstdint>
#include <vector>

namespace dvl_gfx::internal {

void AppendClxTransparentRun(unsigned width, std::vector<uint8_t> &out);
void AppendClxPixelsOrFillRun(const uint8_t *src, unsigned length, std::vector<uint8_t> &out);

} // namespace dvl_gfx::internal
#endif // DVL_GFX_CLX_ENCODE_H_
