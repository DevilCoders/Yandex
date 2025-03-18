#include "modes.h"
#include "parse_json_export.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/stream/format.h>

#include <cstdlib>

namespace {
    struct TArgs {
        NGeoDBBuilder::TBuilderConfig BuilderConfig = {};
        TFsPath OutputPath = {};
        TFsPath JsonDumpFile = {};
        TString HostName = {};

        int VerbosityLevel = {};
    };
}  // namespace

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto args = TArgs{};
    auto parser = args.BuilderConfig.GetLastGetopt();
    parser.SetTitle("Fetch geobase JSON dump and build GeoDB");
    parser.AddLongOption('o', "output")
        .Required()
        .RequiredArgument("FILE")
        .Help("Where to save GeoDB")
        .StoreResult(&args.OutputPath);
    parser.AddLongOption('s', "server")
        .RequiredArgument("HOST")
        .DefaultValue("geoexport.yandex.ru")
        .Help("Geo export host")
        .StoreResult(&args.HostName);
    parser.AddLongOption("as-json")
        .RequiredArgument("FILE")
        .Help("Save geobase JSON dump")
        .StoreResult(&args.JsonDumpFile);
    parser
        .AddLongOption('v', "verbose")
        .OptionalValue(ToString(static_cast<int>(TLOG_INFO)), "LEVEL")
        .DefaultValue("0")
        .Help("Verbosity level")
        .StoreResult(&args.VerbosityLevel);
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult{&parser, argc, argv};
    return args;
}

static int Main(const TArgs& args, IOutputStream& output) {
    InitGlobalLog2Console(args.VerbosityLevel);

    const TString request = NGeoDBBuilder::BuildURLForJSONExport(args.HostName);

    INFO_LOG << "Fetching geobase JSON dump from " << request << Endl;
    const NNeh::TResponseRef response = NNeh::Request(request)->Wait();
    if (response->IsError()) {
        ERROR_LOG << response->GetErrorText() << Endl;
        return EXIT_FAILURE;
    }
    INFO_LOG << "Fetching successful, " << response->Data.size() <<  " bytes ("
             <<  HumanReadableSize(response->Data.size(), SF_BYTES) << ") received" << Endl;

    if (args.JsonDumpFile) {
        INFO_LOG << "Dumping response to " << args.JsonDumpFile << Endl;
        const auto jsonDumpOutput = OpenOutput(args.JsonDumpFile);
        jsonDumpOutput->Write(response->Data);
        INFO_LOG << "Dumping successful" << Endl;
    }

    TStringInput input(response->Data);
    NGeoDBBuilder::ParseGeobaseJSONExport(args.BuilderConfig, input, output);

    return EXIT_SUCCESS;
}

static int Main(const TArgs& args) {
    const auto output = OpenOutput(args.OutputPath);
    return Main(args, *output);
}

int NGeoDBOps::MainBuildRemote(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return ::Main(args);
}
