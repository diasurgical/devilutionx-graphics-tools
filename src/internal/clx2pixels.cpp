#include <clx2pixels.hpp>

#include <cstring>

#include <algorithm>
#include <span>

#include "clx_decode.hpp"

namespace dvl_gfx {

namespace {

void BlitFillDirect(uint8_t *dst, unsigned length, uint8_t color)
{
	std::memset(dst, color, length);
}

void BlitPixelsDirect(uint8_t *dst, const uint8_t *src, unsigned length)
{
	std::memcpy(dst, src, length);
}

void BlitClxCommand(ClxBlitCommand cmd, uint8_t *dst, const uint8_t *src)
{
	switch (cmd.type) {
	case ClxBlitType::Fill:
		BlitFillDirect(dst, cmd.length, cmd.color);
		return;
	case ClxBlitType::Pixels:
		BlitPixelsDirect(dst, src, cmd.length);
		return;
	case ClxBlitType::Transparent:
		return;
	}
}

struct SkipSize {
	int_fast16_t wholeLines;
	int_fast16_t xOffset;
};
SkipSize GetSkipSize(int_fast16_t overrun, int_fast16_t srcWidth)
{
	SkipSize result;
	result.wholeLines = overrun / srcWidth;
	result.xOffset = overrun - srcWidth * result.wholeLines;
	return result;
}

void BlitClxSprite(std::span<const uint8_t> clxSprite, uint8_t *dstBegin, unsigned dstPitch)
{
	int_fast16_t xOffset = 0;

	const uint16_t srcWidth = GetClxSpriteWidth(clxSprite.data());
	const uint8_t *srcBegin = GetClxSpritePixelsData(clxSprite.data());
	const uint8_t *srcEnd = clxSprite.data() + clxSprite.size();

	uint8_t *dst = dstBegin;
	while (srcBegin != srcEnd) {
		auto remainingWidth = static_cast<int_fast16_t>(srcWidth) - xOffset;
		dst += xOffset;
		while (remainingWidth > 0) {
			ClxBlitCommand cmd = ClxGetBlitCommand(srcBegin);
			BlitClxCommand(cmd, dst, srcBegin + 1);
			srcBegin = cmd.srcEnd;
			dst += cmd.length;
			remainingWidth -= cmd.length;
		}
		dst -= dstPitch + srcWidth - remainingWidth;

		if (remainingWidth < 0) {
			const auto skipSize = GetSkipSize(-remainingWidth, static_cast<int_fast16_t>(srcWidth));
			xOffset = skipSize.xOffset;
			dst -= skipSize.wholeLines * dstPitch;
		} else {
			xOffset = 0;
		}
	}
}

Size BlitClxSpriteList(
    std::span<const uint8_t> clxList,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch,
    uint32_t x)
{
	const uint32_t numSprites = GetNumSpritesFromClxList(clxList.data());
	uint32_t y = 0;
	Size size { 0, 0 };
	for (size_t i = 0; i < numSprites; ++i) {
		// CLX sprite data is organized bottom to top.
		// The start of the output is the first pixel of the last line of the sprite.
		const std::span<const uint8_t> clxSprite = GetSpriteDataFromClxList(clxList.data(), i);
		const uint16_t height = GetClxSpriteHeight(clxSprite.data());
		uint8_t *dstBegin = &pixels[(y + height - 1) * pitch + x];
		BlitClxSprite(clxSprite, dstBegin, pitch);
		y += height;
		size.width = std::max<uint32_t>(size.width, GetClxSpriteWidth(clxSprite.data()));
	}
	size.height = y;
	return size;
}

void ConvertClxToPixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch,
    Size *outDimensions)
{
	const uint32_t numLists = GetNumListsFromClxListOrSheetBuffer(clxData);
	if (numLists == 0) {
		const Size size = BlitClxSpriteList(
		    clxData, transparentColor, pixels, pitch, /*x=*/0);
		if (outDimensions != nullptr) {
			*outDimensions = size;
		}
		return;
	}
	uint32_t height = 0;
	uint32_t x = 0;
	for (size_t i = 0; i < numLists; ++i) {
		const Size size = BlitClxSpriteList(GetClxListFromClxSheetBuffer(clxData, i),
		    transparentColor, pixels, pitch, x);
		x += size.width;
		height = std::max(height, size.height);
	}
	if (outDimensions != nullptr) {
		*outDimensions = Size { x, height };
	}
}

} // namespace

Size MeasureVerticallyStackedClxListSize(std::span<const uint8_t> clxList)
{
	Size result { 0, 0 };
	const size_t numSprites = GetNumSpritesFromClxList(clxList.data());
	for (size_t i = 0; i < numSprites; ++i) {
		const std::span<const uint8_t> clxSprite = GetSpriteDataFromClxList(clxList.data(), i);
		result.width = std::max<uint32_t>(result.width, GetClxSpriteWidth(clxSprite.data()));
		result.height += GetClxSpriteHeight(clxSprite.data());
	}
	return result;
}

Size MeasureHorizontallyStackedClxListOrSheetSize(std::span<const uint8_t> clxData)
{
	const uint32_t numLists = GetNumListsFromClxListOrSheetBuffer(clxData);
	if (numLists == 0) {
		return MeasureVerticallyStackedClxListSize(clxData);
	}
	Size result { 0, 0 };
	for (size_t i = 0; i < numLists; ++i) {
		const Size listSize = MeasureVerticallyStackedClxListSize(GetClxListFromClxSheetBuffer(clxData, i));
		result.width += listSize.width;
		result.height = std::max(result.height, listSize.height);
	}
	return result;
}

std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch,
    Size *outDimensions)
{
	ConvertClxToPixels(clxData, transparentColor, pixels, pitch, outDimensions);
	return std::nullopt;
}

std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    std::vector<uint8_t> &pixels,
    std::optional<unsigned> pitch,
    Size *outDimensions)
{
	const Size measuredSize = MeasureHorizontallyStackedClxListOrSheetSize(clxData);
	if (!pitch.has_value())
		pitch = measuredSize.width;
	const size_t size = measuredSize.height * (*pitch);
	if (pixels.size() < size) {
		pixels.resize(size, transparentColor);
		std::fill(pixels.begin(), pixels.end(), transparentColor);
	} else {
		std::fill(pixels.begin(), pixels.begin() + size, transparentColor);
	}
	ConvertClxToPixels(clxData, transparentColor, pixels.data(), *pitch, outDimensions);
	return std::nullopt;
}

} // namespace dvl_gfx
