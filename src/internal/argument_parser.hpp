#pragma once

#include <charconv>
#include <limits>
#include <optional>
#include <string>

#include "tl/expected.hpp"

namespace dvl_gfx {

struct ArgumentError {
	std::string_view arg;
	std::string error;
};

struct ArgumentParserState {
	int pos;
	int argc;
	char **argv;

	[[nodiscard]] bool atEnd() const
	{
		return pos == argc;
	}
	[[nodiscard]] const char *arg() const
	{
		return argv[pos];
	}

	ArgumentParserState &operator++()
	{
		++pos;
		return *this;
	}
};

inline tl::expected<std::string_view, ArgumentError>
ParseArgumentValue(ArgumentParserState &state)
{
	if ((++state).atEnd())
		return tl::unexpected { ArgumentError { state.arg(), "requires a value" } };
	return state.arg();
}

template <typename IntT>
tl::expected<IntT, std::string> ParseInt(
    std::string_view str, IntT min = std::numeric_limits<IntT>::min(),
    IntT max = std::numeric_limits<IntT>::max())
{
	IntT result;
	auto [ptr, ec] { std::from_chars(str.begin(), str.end(), result) };
	if (ec == std::errc::invalid_argument)
		return tl::unexpected { std::string { "must be a number" } };
	if (ec == std::errc::result_out_of_range || result < min || result > max) {
		return tl::unexpected { std::string { "must be between " }.append(std::to_string(min)).append(" and ").append(std::to_string(max)) };
	}
	if (ec != std::errc())
		return tl::unexpected { std::string { "parse error" } };
	return result;
}

template <typename IntT>
tl::expected<std::vector<IntT>, ArgumentError> ParseIntListArgument(
    ArgumentParserState &state, IntT min = std::numeric_limits<IntT>::min(),
    IntT max = std::numeric_limits<IntT>::max())
{
	const std::string_view arg = state.arg();
	tl::expected<std::string_view, ArgumentError> str = ParseArgumentValue(state);
	if (!str.has_value())
		return tl::make_unexpected(std::move(str).error());
	std::vector<IntT> result;
	std::string_view remaining = *str;
	std::string_view::size_type commaPos;
	while (true) {
		commaPos = remaining.find(',');
		const std::string_view part = remaining.substr(0, commaPos);

		tl::expected<IntT, std::string> value = ParseInt(part, min, max);
		if (!value.has_value())
			return tl::unexpected { ArgumentError { arg, std::move(value).error() } };

		result.push_back(*value);
		if (commaPos == std::string_view::npos)
			break;
		remaining.remove_prefix(commaPos + 1);
	}
	return result;
}

template <typename IntT>
tl::expected<IntT, ArgumentError> ParseIntArgument(
    ArgumentParserState &state, IntT min = std::numeric_limits<IntT>::min(),
    IntT max = std::numeric_limits<IntT>::max())
{
	const std::string_view arg = state.arg();
	tl::expected<std::string_view, ArgumentError> str = ParseArgumentValue(state);
	if (!str.has_value())
		return tl::make_unexpected(std::move(str).error());
	tl::expected<IntT, std::string> result = ParseInt<IntT>(*str, min, max);
	if (!result.has_value())
		return tl::unexpected { ArgumentError { arg, std::move(result).error() } };
	return result.value();
}

inline std::optional<ArgumentError>
ParsePositionalArguments(ArgumentParserState &state, std::string_view listName,
    std::vector<const char *> &list)
{
	for (; !state.atEnd(); ++state)
		list.emplace_back(state.arg());
	if (list.empty())
		return ArgumentError { listName, " are required" };
	return std::nullopt;
}

} // namespace dvl_gfx
