#include <clx2pixels.hpp>

#include <cstring>
#include <span>

#include "clx_decode.hpp"

namespace dvl_gfx {

namespace {

std::optional<IoError> CheckSupportedClx(std::span<const uint8_t> clxData)
{
	if (GetNumListsFromClxListOrSheetBuffer(clxData) != 0) {
		return IoError { std::string("CLX sprite sheets are not supported") };
	}
	const size_t numSprites = GetNumSpritesFromClxList(clxData.data());

	const std::span<const uint8_t> firstSprite = GetSpriteDataFromClxList(clxData.data(), 0);
	const uint16_t firstSpriteWidth = GetClxSpriteWidth(firstSprite.data());
	const uint16_t firstSpriteHeight = GetClxSpriteHeight(firstSprite.data());
	for (size_t i = 1; i < numSprites; ++i) {
		const std::span<const uint8_t> sprite = GetSpriteDataFromClxList(clxData.data(), i);
		const uint16_t spriteWidth = GetClxSpriteWidth(sprite.data());
		const uint16_t spriteHeight = GetClxSpriteHeight(sprite.data());
		if (spriteWidth != firstSpriteWidth || spriteHeight != firstSpriteHeight) {
			return IoError { std::string("All the sprites in the CLX sprite list must have the same dimensions") };
		}
	}
	return std::nullopt;
}

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

void ConvertClxToPixels(
    const uint8_t *clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch)
{
	const uint32_t numSprites = GetNumSpritesFromClxList(clxData);
	const uint16_t height = GetClxSpriteHeight(GetSpriteDataFromClxList(clxData, 0).data());
	std::memset(pixels, transparentColor, numSprites * pitch * height);
	for (size_t i = 0; i < numSprites; ++i) {
		// CLX sprite data is organized bottom to top.
		// The start of the output is the first pixel of the last line of the sprite.
		uint8_t *dstBegin = &pixels[(i + 1) * pitch * height - pitch];
		BlitClxSprite(GetSpriteDataFromClxList(clxData, i),
		    dstBegin, pitch);
	}
}

} // namespace

std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    uint8_t *pixels,
    unsigned pitch)
{
	if (std::optional<IoError> error = CheckSupportedClx(clxData); error.has_value()) {
		return error;
	}
	ConvertClxToPixels(clxData.data(), transparentColor, pixels, pitch);
	return std::nullopt;
}

std::optional<IoError> Clx2Pixels(
    std::span<const uint8_t> clxData,
    uint8_t transparentColor,
    std::vector<uint8_t> &pixels,
    std::optional<unsigned> pitch)
{
	if (std::optional<IoError> error = CheckSupportedClx(clxData); error.has_value()) {
		return error;
	}

	const std::span<const uint8_t> firstSprite = GetSpriteDataFromClxList(clxData.data(), 0);
	const uint16_t width = GetClxSpriteWidth(firstSprite.data());
	const uint16_t height = GetClxSpriteHeight(firstSprite.data());
	if (!pitch.has_value())
		pitch = width;
	pixels.resize(GetNumSpritesFromClxList(clxData.data()) * (*pitch) * height);
	ConvertClxToPixels(clxData.data(), transparentColor, pixels.data(), *pitch);
	return std::nullopt;
}

} // namespace dvl_gfx
