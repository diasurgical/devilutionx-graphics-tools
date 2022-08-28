#include <cel2clx.hpp>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include "endian.hpp"

namespace dvl_gfx {

namespace {

constexpr bool IsCelTransparent(uint8_t control)
{
	constexpr uint8_t CelTransparentMin = 0x80;
	return control >= CelTransparentMin;
}

constexpr uint8_t GetCelTransparentWidth(uint8_t control)
{
	return -static_cast<int8_t>(control);
}

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

void AppendCl2FillRun(uint8_t color, unsigned width, std::vector<uint8_t> &out)
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

void AppendCl2PixelsRun(const uint8_t *src, unsigned width, std::vector<uint8_t> &out)
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

void AppendCl2PixelsOrFillRun(const uint8_t *src, unsigned length, std::vector<uint8_t> &out)
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
			// A tunable parameter that decides at which minimum length we encode a fill run.
			// 3 appears to be optimal for most of our data (much better than 2, rarely very slightly worse than 4).
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

std::optional<IoError> CelToClx(const uint8_t *data, size_t size,
    const uint16_t *widths, size_t numWidths, std::vector<uint8_t> &clxData)
{
	// A CEL file either begins with:
	// 1. A CEL header.
	// 2. A list of offsets to frame groups (each group is a CEL file).
	size_t groupsHeaderSize = 0;
	uint32_t numGroups = 1;
	const uint32_t maybeNumFrames = LoadLE32(data);

	// Most files become smaller with CLX. Allocate exactly enough bytes to avoid reallocation.
	// The only file that becomes larger is data\hf_logo3.CEL, by exactly 4445 bytes.
	clxData.reserve(size + 4445);

	// If it is a number of frames, then the last frame offset will be equal to the size of the file.
	if (LoadLE32(&data[maybeNumFrames * 4 + 4]) != size) {
		// maybeNumFrames is the address of the first group, right after
		// the list of group offsets.
		numGroups = maybeNumFrames / 4;
		groupsHeaderSize = maybeNumFrames;
		data += groupsHeaderSize;
		clxData.resize(groupsHeaderSize);
	}

	for (size_t group = 0; group < numGroups; ++group) {
		uint32_t numFrames;
		if (numGroups == 1) {
			numFrames = maybeNumFrames;
		} else {
			numFrames = LoadLE32(data);
			WriteLE32(&clxData[4 * group], clxData.size());
		}

		// CL2 header: frame count, frame offset for each frame, file size
		const size_t cl2DataOffset = clxData.size();
		clxData.resize(clxData.size() + 4 * (2 + static_cast<size_t>(numFrames)));
		WriteLE32(&clxData[cl2DataOffset], numFrames);

		const uint8_t *srcEnd = &data[LoadLE32(&data[4])];
		for (size_t frame = 1; frame <= numFrames; ++frame) {
			const uint8_t *src = srcEnd;
			srcEnd = &data[LoadLE32(&data[4 * (frame + 1)])];
			WriteLE32(&clxData[cl2DataOffset + 4 * frame], static_cast<uint32_t>(clxData.size() - cl2DataOffset));

			// Skip CEL frame header if there is one.
			constexpr size_t CelFrameHeaderSize = 10;
			const bool celFrameHasHeader = LoadLE16(src) == CelFrameHeaderSize;
			if (celFrameHasHeader)
				src += CelFrameHeaderSize;

			const unsigned frameWidth = numWidths == 1 ? *widths : widths[frame - 1];

			// CLX frame header.
			const size_t frameHeaderPos = clxData.size();
			constexpr size_t FrameHeaderSize = 10;
			clxData.resize(clxData.size() + FrameHeaderSize);
			WriteLE16(&clxData[frameHeaderPos], FrameHeaderSize);
			WriteLE16(&clxData[frameHeaderPos + 2], frameWidth);

			unsigned transparentRunWidth = 0;
			size_t frameHeight = 0;
			while (src != srcEnd) {
				// Process line:
				for (unsigned remainingCelWidth = frameWidth; remainingCelWidth != 0;) {
					uint8_t val = *src++;
					if (IsCelTransparent(val)) {
						val = GetCelTransparentWidth(val);
						transparentRunWidth += val;
					} else {
						AppendCl2TransparentRun(transparentRunWidth, clxData);
						transparentRunWidth = 0;
						AppendCl2PixelsOrFillRun(src, val, clxData);
						src += val;
					}
					remainingCelWidth -= val;
				}
				++frameHeight;
			}
			WriteLE16(&clxData[frameHeaderPos + 4], frameHeight);
			memset(&clxData[frameHeaderPos + 6], 0, 4);
			AppendCl2TransparentRun(transparentRunWidth, clxData);
		}

		WriteLE32(&clxData[cl2DataOffset + 4 * (1 + static_cast<size_t>(numFrames))], static_cast<uint32_t>(clxData.size() - cl2DataOffset));
		data = srcEnd;
	}
	return std::nullopt;
}

std::optional<IoError> CelToClx(const char *inputPath, const char *outputPath,
    const uint16_t *widths, size_t numWidths,
    uintmax_t *inputFileSize,
    uintmax_t *outputFileSize)
{
	std::error_code ec;
	const uintmax_t size = std::filesystem::file_size(inputPath, ec);
	if (ec)
		return IoError { ec.message() };
	if (inputFileSize != nullptr)
		*inputFileSize = size;

	std::ifstream input;
	input.open(inputPath, std::ios::in | std::ios::binary);
	if (input.fail())
		return IoError { std::string("Failed to open input file: ")
			                 .append(std::strerror(errno)) };
	std::unique_ptr<uint8_t[]> ownedData { new uint8_t[size] };
	input.read(reinterpret_cast<char *>(ownedData.get()), static_cast<std::streamsize>(size));
	if (input.fail()) {
		return IoError {
			std::string("Failed to read CEL data: ").append(std::strerror(errno))
		};
	}
	input.close();
	const uint8_t *data = ownedData.get();

	std::ofstream output;
	output.open(outputPath, std::ios::out | std::ios::binary);
	if (output.fail())
		return IoError { std::string("Failed to open output file: ")
			                 .append(std::strerror(errno)) };

	std::vector<uint8_t> clxData;
	const std::optional<IoError> err = CelToClx(ownedData.get(), size, widths, numWidths, clxData);
	if (err.has_value())
		return err;

	if (outputFileSize != nullptr)
		*outputFileSize = clxData.size();
	output.write(reinterpret_cast<const char *>(clxData.data()), static_cast<std::streamsize>(clxData.size()));
	output.close();
	if (output.fail())
		return IoError { std::string("Failed to write to output file: ")
			                 .append(std::strerror(errno)) };

	return std::nullopt;
}

} // namespace dvl_gfx
