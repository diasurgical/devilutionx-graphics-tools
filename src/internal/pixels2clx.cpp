#include <pixels2clx.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <dvl_gfx_endian.hpp>

#include "clx_encode.hpp"

namespace dvl_gfx {

void Pixels2Clx(
    const uint8_t *pixels,
    unsigned pitch, unsigned width, unsigned frameHeight, unsigned numFrames,
    std::optional<uint8_t> transparentColor, std::vector<uint8_t> &clxData)
{
	// CL2 header: frame count, frame offset for each frame, file size
	clxData.resize(4 * (2 + static_cast<size_t>(numFrames)));
	WriteLE32(clxData.data(), numFrames);

	const unsigned height = frameHeight * numFrames;

	// We process the surface a whole frame at a time because the lines are reversed in CEL.
	for (unsigned frame = 1; frame <= numFrames; ++frame) {
		WriteLE32(&clxData[4 * static_cast<size_t>(frame)],
		    static_cast<uint32_t>(clxData.size()));

		// Frame header: 5 16-bit values:
		// 1. Offset to start of the pixel data.
		// 2. Width
		// 3. Height
		// 4..5. Unused (0)
		const size_t frameHeaderPos = clxData.size();
		constexpr size_t FrameHeaderSize = 10;
		clxData.resize(clxData.size() + FrameHeaderSize);

		// Frame header:
		WriteLE16(&clxData[frameHeaderPos], FrameHeaderSize);
		WriteLE16(&clxData[frameHeaderPos + 2], static_cast<uint16_t>(width));
		WriteLE16(&clxData[frameHeaderPos + 4], static_cast<uint16_t>(frameHeight));
		memset(&clxData[frameHeaderPos + 6], 0, 4);

		const uint8_t *frameBuffer = &pixels[static_cast<size_t>((frame - 1) * pitch * frameHeight)];
		unsigned transparentRunWidth = 0;
		size_t line = 0;
		while (line != frameHeight) {
			// Process line:
			const uint8_t *src = &frameBuffer[(frameHeight - (line + 1)) * pitch];
			if (transparentColor) {
				unsigned solidRunWidth = 0;
				for (const uint8_t *srcEnd = src + width; src != srcEnd; ++src) {
					if (*src == *transparentColor) {
						if (solidRunWidth != 0) {
							AppendClxPixelsOrFillRun(
							    src - transparentRunWidth - solidRunWidth, solidRunWidth,
							    clxData);
							solidRunWidth = 0;
						}
						++transparentRunWidth;
					} else {
						AppendClxTransparentRun(transparentRunWidth, clxData);
						transparentRunWidth = 0;
						++solidRunWidth;
					}
				}
				if (solidRunWidth != 0) {
					AppendClxPixelsOrFillRun(src - solidRunWidth, solidRunWidth, clxData);
				}
			} else {
				AppendClxPixelsOrFillRun(src, width, clxData);
			}
			++line;
		}
		AppendClxTransparentRun(transparentRunWidth, clxData);
	}

	WriteLE32(&clxData[4 * (1 + static_cast<size_t>(numFrames))],
	    static_cast<uint32_t>(clxData.size()));
}

} // namespace dvl_gfx
