#include <cel2clx.hpp>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <dvl_gfx_endian.hpp>

#include "clx_encode.hpp"

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

		// CLX header: frame count, frame offset for each frame, file size
		const size_t clxDataOffset = clxData.size();
		clxData.resize(clxData.size() + 4 * (2 + static_cast<size_t>(numFrames)));
		WriteLE32(&clxData[clxDataOffset], numFrames);

		const uint8_t *srcEnd = &data[LoadLE32(&data[4])];
		for (size_t frame = 1; frame <= numFrames; ++frame) {
			const uint8_t *src = srcEnd;
			srcEnd = &data[LoadLE32(&data[4 * (frame + 1)])];
			WriteLE32(&clxData[clxDataOffset + 4 * frame], static_cast<uint32_t>(clxData.size() - clxDataOffset));

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
						AppendClxTransparentRun(transparentRunWidth, clxData);
						transparentRunWidth = 0;
						AppendClxPixelsOrFillRun(src, val, clxData);
						src += val;
					}
					remainingCelWidth -= val;
				}
				++frameHeight;
			}
			AppendClxTransparentRun(transparentRunWidth, clxData);
			WriteLE16(&clxData[frameHeaderPos + 4], frameHeight);
			memset(&clxData[frameHeaderPos + 6], 0, 4);
		}

		WriteLE32(&clxData[clxDataOffset + 4 * (1 + static_cast<size_t>(numFrames))], static_cast<uint32_t>(clxData.size() - clxDataOffset));
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
