#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "argument_parser.hpp"
#include "cel2clx.hpp"
#include "io_error.hpp"
#include "tl/expected.hpp"

namespace devilution {
namespace {

constexpr char KHelp[] = R"(Usage: cel2clx [options] files...

Converts CEL sprite(s) to a CLX file.

Options:
  --output-dir <arg>           Output directory. Default: input file directory.
  --width <arg>[,<arg>...]     CEL sprite frame width(s), comma-separated.
  -q, --quiet                  Do not log anything.
)";

struct Options {
	std::vector<const char *> inputPaths;
	std::optional<std::string_view> outputDir;
	std::vector<uint16_t> widths;
	bool quiet = false;
};

void PrintHelp()
{
	std::cerr << KHelp << std::endl;
}

tl::expected<Options, ArgumentError> ParseArguments(int argc, char *argv[])
{
	if (argc == 1) {
		PrintHelp();
		std::exit(64);
	}
	Options options;
	ArgumentParserState state { 1, argc, argv };
	for (; !state.atEnd(); ++state.pos) {
		const std::string_view arg = state.arg();
		if (arg == "-h" || arg == "--help") {
			PrintHelp();
			std::exit(0);
		}
		if (arg == "--output-dir") {
			tl::expected<std::string_view, ArgumentError> value = ParseArgumentValue(state);
			if (!value.has_value())
				return tl::unexpected { std::move(value).error() };
			options.outputDir = *value;
		} else if (arg == "--width") {
			tl::expected<std::vector<uint16_t>, ArgumentError> value = ParseIntListArgument<uint16_t>(state);
			if (!value.has_value())
				return tl::unexpected { std::move(value).error() };
			options.widths = *std::move(value);
		} else if (arg == "-q" || arg == "--quiet") {
			options.quiet = true;
		} else if (arg.empty() || arg[0] == '-') {
			return tl::unexpected { ArgumentError { arg, "unknown argument" } };
		} else {
			break;
		}
	}
	if (std::optional<ArgumentError> error = ParsePositionalArguments(state, "files...", options.inputPaths);
	    error.has_value()) {
		return tl::unexpected { *std::move(error) };
	}
	return options;
}

std::optional<IoError> Run(const Options &options)
{
	if (!options.quiet) {
		std::clog << "file\tCEL\tCLX" << std::endl;
	}

	std::optional<std::filesystem::path> outputDirFs;
	if (options.outputDir.has_value())
		outputDirFs = *options.outputDir;
	for (const char *inputPath : options.inputPaths) {
		std::filesystem::path inputPathFs { inputPath };
		std::string outputPath;
		if (outputDirFs.has_value()) {
			outputPath = *outputDirFs / inputPathFs.filename().replace_extension("clx");
		} else {
			outputPath = inputPathFs.replace_extension("clx");
		}
		uintmax_t inputFileSize;
		uintmax_t outputFileSize;
		if (std::optional<devilution::IoError> error = CelToClx(inputPath, outputPath.c_str(), options.widths, inputFileSize, outputFileSize);
		    error.has_value()) {
			return error;
		}
		if (!options.quiet) {
			std::clog << inputPathFs.stem().c_str() << "\t" << inputFileSize << "\t"
			          << outputFileSize << std::endl;
		}
	}
	return std::nullopt;
}

} // namespace
} // namespace devilution

int main(int argc, char *argv[])
{
	tl::expected<devilution::Options, devilution::ArgumentError> options = devilution::ParseArguments(argc, argv);
	if (!options) {
		std::cerr << options.error().arg << ": " << options.error().error
		          << std::endl;
		return 64;
	}
	if (std::optional<devilution::IoError> error = Run(*options);
	    error.has_value()) {
		std::cerr << error->message << std::endl;
		return 1;
	}
	return 0;
}
