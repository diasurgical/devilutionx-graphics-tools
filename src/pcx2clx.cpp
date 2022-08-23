#include "pcx2clx.hpp"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include "endian.hpp"
#include "pcx.hpp"

namespace devilution {

namespace {

void AppendCl2TransparentRun(unsigned width, std::vector<uint8_t> &out)
{
	while (width >= 0x7F) {
		out.push_back(0x7F);
		width -= 0x7F;
	}
	if (width == 0)
		return;
	out.push_back(width);
}

void AppendCl2FillRun(uint8_t color, unsigned width,
    std::vector<uint8_t> &out)
{
	while (width >= 0x3F) {
		out.push_back(0x80);
		out.push_back(color);
		width -= 0x3F;
	}
	if (width == 0)
		return;
	out.push_back(0xBF - width);
	out.push_back(color);
}

void AppendCl2PixelsRun(const uint8_t *src, unsigned width,
    std::vector<uint8_t> &out)
{
	while (width >= 0x41) {
		out.push_back(0xBF);
		for (size_t i = 0; i < 0x41; ++i)
			out.push_back(src[i]);
		width -= 0x41;
		src += 0x41;
	}
	if (width == 0)
		return;
	out.push_back(256 - width);
	for (size_t i = 0; i < width; ++i)
		out.push_back(src[i]);
}

void AppendCl2PixelsOrFillRun(const uint8_t *src, unsigned length,
    std::vector<uint8_t> &out)
{
	const uint8_t *begin = src;
	const uint8_t *prevColorBegin = src;
	unsigned prevColorRunLength = 1;
	uint8_t prevColor = *src++;
	while (--length > 0) {
		const uint8_t color = *src;
		if (prevColor == color) {
			++prevColorRunLength;
		} else {
			// A tunable parameter that decides at which minimum length we encode a
			// fill run. 3 appears to be optimal for most of our data (much better
			// than 2, rarely very slightly worse than 4).
			constexpr unsigned MinFillRunLength = 3;
			if (prevColorRunLength >= MinFillRunLength) {
				AppendCl2PixelsRun(begin, prevColorBegin - begin, out);
				AppendCl2FillRun(prevColor, prevColorRunLength, out);
				begin = src;
			}
			prevColorBegin = src;
			prevColorRunLength = 1;
			prevColor = color;
		}
		++src;
	}
	AppendCl2PixelsRun(begin, prevColorBegin - begin, out);
	AppendCl2FillRun(prevColor, prevColorRunLength, out);
}

} // namespace

std::optional<IoError> PcxToClx(const char *inputPath, const char *outputPath,
    int numFramesOrFrameHeight,
    std::optional<uint8_t> transparentColor,
    bool exportPalette,
    uintmax_t &inputFileSize,
    uintmax_t &outputFileSize)
{
	std::ifstream input;
	input.open(inputPath, std::ios::in | std::ios::binary);
	if (input.fail())
		return IoError { std::string("Failed to open input file: ")
			                 .append(std::strerror(errno)) };

	int width;
	int height;
	uint8_t bpp;
	if (std::optional<IoError> error = LoadPcxMeta(input, width, height, bpp);
	    error.has_value()) {
		return error;
	}
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

	std::error_code ec;
	uintmax_t pixelDataSize = std::filesystem::file_size(inputPath, ec);
	if (ec)
		return IoError { ec.message() };
	inputFileSize = pixelDataSize;

	pixelDataSize -= PcxHeaderSize;

	std::unique_ptr<uint8_t[]> fileBuffer { new uint8_t[pixelDataSize] };
	input.read(reinterpret_cast<char *>(fileBuffer.get()), static_cast<std::streamsize>(pixelDataSize));
	if (input.fail()) {
		return IoError {
			std::string("Failed to read PCX data: ").append(std::strerror(errno))
		};
	}
	input.close();

	// CLX header: frame count, frame offset for each frame, file size
	std::vector<uint8_t> cl2Data;
	cl2Data.reserve(pixelDataSize);
	cl2Data.resize(4 * (2 + static_cast<size_t>(numFrames)));
	WriteLE32(cl2Data.data(), numFrames);

	// We process the PCX a whole frame at a time because the lines are reversed
	// in CEL.
	auto frameBuffer = std::unique_ptr<uint8_t[]>(
	    new uint8_t[static_cast<size_t>(frameHeight) * width]);

	const unsigned srcSkip = width % 2;
	uint8_t *dataPtr = fileBuffer.get();
	for (unsigned frame = 1; frame <= numFrames; ++frame) {
		WriteLE32(&cl2Data[4 * static_cast<size_t>(frame)],
		    static_cast<uint32_t>(cl2Data.size()));

		// Frame header: 5 16-bit values:
		// 1. Offset to start of the pixel data.
		// 2. Width
		// 3. Height
		// 4..5. Unused (0)
		const size_t frameHeaderPos = cl2Data.size();
		constexpr size_t FrameHeaderSize = 10;
		cl2Data.resize(cl2Data.size() + FrameHeaderSize);

		// Frame header:
		WriteLE16(&cl2Data[frameHeaderPos], FrameHeaderSize);
		WriteLE16(&cl2Data[frameHeaderPos + 2], static_cast<uint16_t>(width));
		WriteLE16(&cl2Data[frameHeaderPos + 4], static_cast<uint16_t>(frameHeight));
		memset(&cl2Data[frameHeaderPos + 6], 0, 4);

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
				for (const uint8_t *srcEnd = src + width; src != srcEnd; ++src) {
					if (*src == *transparentColor) {
						if (solidRunWidth != 0) {
							AppendCl2PixelsOrFillRun(
							    src - transparentRunWidth - solidRunWidth, solidRunWidth,
							    cl2Data);
							solidRunWidth = 0;
						}
						++transparentRunWidth;
					} else {
						AppendCl2TransparentRun(transparentRunWidth, cl2Data);
						transparentRunWidth = 0;
						++solidRunWidth;
					}
				}
				if (solidRunWidth != 0) {
					AppendCl2PixelsOrFillRun(src - solidRunWidth, solidRunWidth, cl2Data);
				}
			} else {
				AppendCl2PixelsOrFillRun(src, width, cl2Data);
			}
			++line;
		}
		AppendCl2TransparentRun(transparentRunWidth, cl2Data);
	}
	WriteLE32(&cl2Data[4 * (1 + static_cast<size_t>(numFrames))],
	    static_cast<uint32_t>(cl2Data.size()));

	outputFileSize = cl2Data.size();

	if (exportPalette) {
		constexpr unsigned PcxPaletteSeparator = 0x0C;
		if (*dataPtr++ != PcxPaletteSeparator)
			return IoError { std::string("PCX has no palette") };

		std::array<uint8_t, 256 * 3> outPalette;
		uint8_t *out = &outPalette[0];
		for (unsigned i = 0; i < 256; ++i) {
			*out++ = *dataPtr++;
			*out++ = *dataPtr++;
			*out++ = *dataPtr++;
		}

		std::ofstream output;
		output.open(std::filesystem::path(outputPath).replace_extension("pal").c_str(), std::ios::out | std::ios::binary);
		if (output.fail())
			return IoError { std::string("Failed to open palette output file: ")
				                 .append(std::strerror(errno)) };
		output.write(reinterpret_cast<const char *>(outPalette.data()), static_cast<std::streamsize>(outPalette.size()));
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
		output.write(reinterpret_cast<const char *>(cl2Data.data()), static_cast<std::streamsize>(cl2Data.size()));
		output.close();
		if (output.fail())
			return IoError { std::string("Failed to write to output file: ")
				                 .append(std::strerror(errno)) };
	}

	return std::nullopt;
}

} // namespace devilution
