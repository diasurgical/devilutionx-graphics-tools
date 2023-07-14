#include <pcx2clx.hpp>

#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <dvl_gfx_endian.hpp>

#include "clx_encode.hpp"
#include "pcx.hpp"

namespace dvl_gfx {

std::optional<IoError> PcxToClx(const uint8_t *data, size_t size,
    int numFramesOrFrameHeight,
    std::optional<uint8_t> transparentColor,
    const std::vector<uint16_t> &cropWidths,
    std::vector<uint8_t> &clxData,
    uint8_t *paletteData)
{
	int width;
	int height;
	uint8_t bpp;
	if (size < PcxHeaderSize) {
		return IoError { "data too small" };
	}
	const uint8_t *pixelData = LoadPcxMeta(data, width, height, bpp);
	const size_t pixelDataSize = size - PcxHeaderSize;
	assert(bpp == 8);

	unsigned numFrames;
	unsigned frameHeight;
	if (numFramesOrFrameHeight > 0) {
		numFrames = numFramesOrFrameHeight;
		frameHeight = height / numFrames;
	} else {
		frameHeight = -numFramesOrFrameHeight;
		numFrames = height / frameHeight;
	}

	// CLX header: frame count, frame offset for each frame, file size
	clxData.reserve(pixelDataSize);
	clxData.resize(4 * (2 + static_cast<size_t>(numFrames)));
	WriteLE32(clxData.data(), numFrames);

	// We process the PCX a whole frame at a time because the lines are reversed
	// in CEL.
	auto frameBuffer = std::unique_ptr<uint8_t[]>(
	    new uint8_t[static_cast<size_t>(frameHeight) * width]);

	const unsigned srcSkip = width % 2;
	const uint8_t *dataPtr = pixelData;
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
		const uint16_t frameWidth = cropWidths.empty()
		    ? width
		    : cropWidths[std::min<size_t>(cropWidths.size(), frame) - 1];

		WriteLE16(&clxData[frameHeaderPos], FrameHeaderSize);
		WriteLE16(&clxData[frameHeaderPos + 2], static_cast<uint16_t>(frameWidth));
		WriteLE16(&clxData[frameHeaderPos + 4], static_cast<uint16_t>(frameHeight));
		memset(&clxData[frameHeaderPos + 6], 0, 4);

		for (unsigned j = 0; j < frameHeight; ++j) {
			uint8_t *buffer = &frameBuffer[static_cast<size_t>(j) * width];
			for (unsigned x = 0; x < static_cast<unsigned>(width);) {
				constexpr uint8_t PcxMaxSinglePixel = 0xBF;
				const uint8_t byte = *dataPtr++;
				if (byte <= PcxMaxSinglePixel) {
					*buffer++ = byte;
					++x;
					continue;
				}
				constexpr uint8_t PcxRunLengthMask = 0x3F;
				const uint8_t runLength = (byte & PcxRunLengthMask);
				std::memset(buffer, *dataPtr++, runLength);
				buffer += runLength;
				x += runLength;
			}
			dataPtr += srcSkip;
		}

		unsigned transparentRunWidth = 0;
		size_t line = 0;
		while (line != frameHeight) {
			// Process line:
			const uint8_t *src = &frameBuffer[(frameHeight - (line + 1)) * width];
			if (transparentColor) {
				unsigned solidRunWidth = 0;
				for (const uint8_t *srcEnd = src + frameWidth; src != srcEnd; ++src) {
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

	if (paletteData != nullptr) {
		constexpr unsigned PcxPaletteSeparator = 0x0C;
		if (*dataPtr++ != PcxPaletteSeparator)
			return IoError { std::string("PCX has no palette") };

		uint8_t *out = paletteData;
		for (unsigned i = 0; i < 256; ++i) {
			*out++ = *dataPtr++;
			*out++ = *dataPtr++;
			*out++ = *dataPtr++;
		}
	}
	return std::nullopt;
}

std::optional<IoError> PcxToClx(const char *inputPath, const char *outputPath,
    int numFramesOrFrameHeight,
    std::optional<uint8_t> transparentColor,
    const std::vector<uint16_t> &cropWidths,
    bool exportPalette,
    uintmax_t *inputFileSize,
    uintmax_t *outputFileSize)
{
	std::ifstream input;
	input.open(inputPath, std::ios::in | std::ios::binary);
	if (input.fail())
		return IoError { std::string("Failed to open input file: ")
			                 .append(std::strerror(errno)) };

	std::error_code ec;
	uintmax_t pixelDataSize = std::filesystem::file_size(inputPath, ec);
	if (ec)
		return IoError { ec.message() };
	if (inputFileSize != nullptr)
		*inputFileSize = pixelDataSize;

	std::unique_ptr<uint8_t[]> fileBuffer { new uint8_t[pixelDataSize] };
	input.read(reinterpret_cast<char *>(fileBuffer.get()), static_cast<std::streamsize>(pixelDataSize));
	if (input.fail()) {
		return IoError {
			std::string("Failed to read PCX data: ").append(std::strerror(errno))
		};
	}
	input.close();

	std::vector<uint8_t> clxData;
	std::array<uint8_t, 256 * 3> paletteData;
	if (const std::optional<IoError> error = PcxToClx(
	        fileBuffer.get(), pixelDataSize, numFramesOrFrameHeight, transparentColor,
	        cropWidths, clxData, exportPalette ? paletteData.data() : nullptr);
	    error.has_value()) {
		return error;
	}

	if (outputFileSize != nullptr)
		*outputFileSize = clxData.size();

	if (exportPalette) {
		std::ofstream output;
		output.open(std::filesystem::path(outputPath).replace_extension("pal").c_str(), std::ios::out | std::ios::binary);
		if (output.fail())
			return IoError { std::string("Failed to open palette output file: ")
				                 .append(std::strerror(errno)) };
		output.write(reinterpret_cast<const char *>(paletteData.data()), static_cast<std::streamsize>(paletteData.size()));
		output.close();
		if (output.fail())
			return IoError { std::string("Failed to write to palette output file: ")
				                 .append(std::strerror(errno)) };
	}

	{
		std::ofstream output;
		output.open(outputPath, std::ios::out | std::ios::binary);
		if (output.fail())
			return IoError { std::string("Failed to open output file: ")
				                 .append(std::strerror(errno)) };
		output.write(reinterpret_cast<const char *>(clxData.data()), static_cast<std::streamsize>(clxData.size()));
		output.close();
		if (output.fail())
			return IoError { std::string("Failed to write to output file: ")
				                 .append(std::strerror(errno)) };
	}

	return std::nullopt;
}

} // namespace dvl_gfx
