#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace dvl_gfx {

template <typename T>
constexpr uint16_t LoadLE16(const T *b)
{
	static_assert(sizeof(T) == 1, "invalid argument");
	return (static_cast<uint8_t>(b[1]) << 8) | static_cast<uint8_t>(b[0]);
}

template <typename T>
constexpr uint32_t LoadLE32(const T *b)
{
	static_assert(sizeof(T) == 1, "invalid argument");
	return (static_cast<uint8_t>(b[3]) << 24) | (static_cast<uint8_t>(b[2]) << 16) | (static_cast<uint8_t>(b[1]) << 8) | static_cast<uint8_t>(b[0]);
}

template <typename T>
constexpr uint32_t LoadBE32(const T *b)
{
	static_assert(sizeof(T) == 1, "invalid argument");
	return (static_cast<uint8_t>(b[0]) << 24) | (static_cast<uint8_t>(b[1]) << 16) | (static_cast<uint8_t>(b[2]) << 8) | static_cast<uint8_t>(b[3]);
}

template <typename T>
constexpr T ByteSwap(T value) noexcept
{
	std::array<std::byte, sizeof(T)> bytes;
	std::memcpy(bytes.data(), &value, sizeof(T));
	std::reverse(bytes.begin(), bytes.end());
	std::memcpy(&value, bytes.data(), sizeof(T));
	return value;
}

template <typename T>
constexpr T SwapLE(T value) noexcept
{
	if constexpr (std::endian::native == std::endian::big) {
		return ByteSwap(value);
	}
	return value;
}

inline void WriteLE16(void *out, uint16_t val)
{
	val = SwapLE(val);
	memcpy(out, &val, 2);
}

inline void WriteLE32(void *out, uint32_t val)
{
	val = SwapLE(val);
	memcpy(out, &val, 4);
}

} // namespace dvl_gfx
