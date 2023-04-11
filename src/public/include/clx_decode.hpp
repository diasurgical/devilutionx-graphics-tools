#ifndef DVL_GFX_CLX_DECODE_H_
#define DVL_GFX_CLX_DECODE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include <dvl_gfx_endian.hpp>

namespace dvl_gfx {

[[nodiscard]] constexpr uint16_t GetNumListsFromClxListOrSheetBuffer(std::span<const uint8_t> clxData)
{
	const uint32_t maybeNumFrames = LoadLE32(clxData.data());

	// If it is a number of frames, then the last frame offset will be equal to the size of the file.
	if (LoadLE32(&clxData[maybeNumFrames * 4 + 4]) != clxData.size())
		return maybeNumFrames / 4;

	// Not a sprite sheet.
	return 0;
}

[[nodiscard]] constexpr std::span<const uint8_t> GetClxListFromClxSheetBuffer(
    std::span<const uint8_t> clxSheet, size_t listIndex)
{
	const uint32_t beginOffset = LoadLE32(&clxSheet[4 * listIndex]);
	const uint32_t endOffset = LoadLE32(&clxSheet[4 * (listIndex + 1)]);
	return std::span(&clxSheet[beginOffset], endOffset - beginOffset);
}

[[nodiscard]] constexpr uint32_t GetNumSpritesFromClxList(const uint8_t *clxList)
{
	return LoadLE32(clxList);
}

[[nodiscard]] constexpr uint32_t GetSpriteOffsetFromClxList(const uint8_t *clxList, size_t spriteIndex)
{
	return LoadLE32(&clxList[4 + spriteIndex * 4]);
}

[[nodiscard]] constexpr std::span<const uint8_t> GetSpriteDataFromClxList(const uint8_t *clxList, size_t spriteIndex)
{
	const uint32_t begin = GetSpriteOffsetFromClxList(clxList, spriteIndex);
	const uint32_t end = GetSpriteOffsetFromClxList(clxList, spriteIndex + 1);
	return std::span<const uint8_t> { &clxList[begin], end - begin };
}

[[nodiscard]] constexpr uint16_t GetClxSpriteWidth(const uint8_t *clxSprite)
{
	return LoadLE16(&clxSprite[2]);
}

[[nodiscard]] constexpr uint16_t GetClxSpriteHeight(const uint8_t *clxSprite)
{
	return LoadLE16(&clxSprite[4]);
}

[[nodiscard]] constexpr const uint8_t *GetClxSpritePixelsData(const uint8_t *clxSprite)
{
	return &clxSprite[10];
}

[[nodiscard]] constexpr bool IsClxOpaque(uint8_t control)
{
	constexpr uint8_t ClxOpaqueMin = 0x80;
	return control >= ClxOpaqueMin;
}

[[nodiscard]] constexpr uint8_t GetClxOpaquePixelsWidth(uint8_t control)
{
	return -static_cast<std::int8_t>(control);
}

[[nodiscard]] constexpr bool IsClxOpaqueFill(uint8_t control)
{
	constexpr uint8_t ClxFillMax = 0xBE;
	return control <= ClxFillMax;
}

[[nodiscard]] constexpr uint8_t GetClxOpaqueFillWidth(uint8_t control)
{
	constexpr uint8_t ClxFillEnd = 0xBF;
	return static_cast<int_fast16_t>(ClxFillEnd - control);
}

enum class ClxBlitType : uint8_t {
	Transparent,
	Pixels,
	Fill
};

struct ClxBlitCommand {
	ClxBlitType type;
	const uint8_t *srcEnd; // Pointer past the end of the command.
	unsigned length;       // Number of pixels this command will write.
	uint8_t color;         // For `BlitType::Pixel` and `BlitType::Fill` only.
};

[[nodiscard]] constexpr ClxBlitCommand ClxGetBlitCommand(const uint8_t *src)
{
	const uint8_t control = *src++;
	if (!IsClxOpaque(control))
		return ClxBlitCommand { ClxBlitType::Transparent, src, control, 0 };
	if (IsClxOpaqueFill(control)) {
		const uint8_t width = GetClxOpaqueFillWidth(control);
		const uint8_t color = *src++;
		return ClxBlitCommand { ClxBlitType::Fill, src, width, color };
	}
	const uint8_t width = GetClxOpaquePixelsWidth(control);
	return ClxBlitCommand { ClxBlitType::Pixels, src + width, width, 0 };
}

} // namespace dvl_gfx
#endif // DVL_GFX_CLX_DECODE_H_
