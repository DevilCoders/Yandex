#include "modes.h"
#include "parse_json_export.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

#include <cstdlib>

namespace {
    struct TArgs {
        NGeoDBBuilder::TBuilderConfig BuilderConfig = {};
        TFsPath InputPath = {};
        TFsPath OutputPath = {};

        int VerbosityLevel = {};
    };
}  // namespace

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto args = TArgs{};
    auto parser = args.BuilderConfig.GetLastGetopt();
    parser.SetTitle("Build GeoDB from geobase JSON dump");
    parser.AddLongOption('i', "input")
        .RequiredArgument("FILE")
        .DefaultValue("-")
        .Help("Input file")
        .StoreResult(&args.InputPath);
    parser.AddLongOption('o', "output")
        .Required()
        .RequiredArgument("FILE")
        .Help("Where to save GeoDB")
        .StoreResult(&args.OutputPath);
    parser.AddLongOption('v', "verbose")
        .OptionalValue(ToString(static_cast<int>(TLOG_INFO)), "LEVEL")
        .DefaultValue("0")
        .Help("Verbosity level")
        .StoreResult(&args.VerbosityLevel);
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult{&parser, argc, argv};
    return args;
}

static int Main(const TArgs& args, IInputStream& input, IOutputStream& output) {
    InitGlobalLog2Console(args.VerbosityLevel);
    NGeoDBBuilder::ParseGeobaseJSONExport(args.BuilderConfig, input, output);
    return EXIT_SUCCESS;
}

static int Main(const TArgs& args) {
    const auto input = OpenInput(args.InputPath);
    const auto output = OpenOutput(args.OutputPath);
    return Main(args, *input, *output);
}

int NGeoDBOps::MainBuildFile(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return ::Main(args);
}
