#include "modes.h"

#include "validate.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/logger/global/global.h>

#include <kernel/geodb/geodb.h>

#include <util/folder/path.h>
#include <util/stream/input.h>

#include <cstdlib>

namespace {
    struct TArgs {
        TFsPath GeoDBPath;
        int VerbosityLevel{static_cast<int>(INFO_LOG)};
    };
}

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto args = TArgs{};
    auto p = NLastGetopt::TOpts::Default();
    p.SetTitle("Validate GeoDB content (will return non-zero code if content is invalid)");
    p.AddLongOption('i', "input")
     .DefaultValue("-")
     .RequiredArgument("PATH")
     .Help("GeoDB")
     .StoreResult(&args.GeoDBPath);
    p.AddLongOption('v', "verbose")
     .OptionalValue(ToString(static_cast<int>(TLOG_INFO)), "LEVEL")
     .DefaultValue("0")
     .Help("Verbosity level")
     .StoreResult(&args.VerbosityLevel);
    p.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult{&p, argc, argv};
    return args;
}

static int Main(const TArgs&, IInputStream& input) {
    const auto geodb = NGeoDB::TGeoKeeper::LoadToHolder(input);
    if (NGeoDBOps::IsValid(*geodb)) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

static int Main(const TArgs& args) {
    const auto input = OpenInput(args.GeoDBPath);
    return Main(args, *input);
}

int NGeoDBOps::MainValidate(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return ::Main(args);
}
